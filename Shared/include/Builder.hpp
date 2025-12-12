#pragma once
#include <ProgramStructure.hpp>
#include <Generator.hpp>
#include <BuiltInGenerators/JsonGenerator.hpp>
#include <BuiltInGenerators/SqliteGenerator.hpp>
#include <BuiltInGenerators/MySqlGenerator.hpp>
// #include <BuiltInGenerators/LuaGenerator.hpp>
// #include <BuiltInGenerators/JavaGenerator.hpp>
#include <BuiltInGenerators/CppGenerator.hpp>
#include <ArgParser/ArgParser.hpp>
#include <boost/dll.hpp>
#include <boost/function.hpp>
class session;

class Builder
{
public:

    void addGenerator(std::shared_ptr<Generator> generator)
    {
        generators[generator->name] = generator;

        for (auto& [name, gen] : generators)
        {
            if (gen != generator)
            {
                generator->add_generator(gen);
                gen->add_generator(generator);
            }
        }

        // if (generator->type == Generator::GeneratorType::Language)
        // {
        //     auto non_language_generators = generators | std::views::values | std::views::filter([](std::shared_ptr<Generator> gen)
        //                                                                                         { return gen->type != Generator::GeneratorType::Language; });
        //     for (auto &nl_gen : non_language_generators)
        //     {
        //         generator->add_generator(nl_gen);
        //     }
        // }
        // else
        // {
        //     auto language_generators = generators | std::views::values | std::views::filter([](std::shared_ptr<Generator> gen)
        //                                                                                     { return gen->type == Generator::GeneratorType::Language; });
        //     for (auto &lang_gen : language_generators)
        //     {
        //         if (lang_gen != generator)
        //         {
        //             lang_gen->add_generator(generator);
        //         }
        //     }
        // }
    }

    void removeGenerator(std::string name)
    {
        if (generators.find(name) != generators.end())
        {
            generators.erase(name);
        }
    }

    void enableGenerator(std::string name)
    {
        if (generators.find(name) != generators.end())
        {
            generators[name]->enabled = true;
        }
    }

    long getUniqueSubsetCount(){
        return program_structure_->getUniqueSubsetCount();
    }

    void disableGenerator(std::string name)
    {
        if (generators.find(name) != generators.end())
        {
            generators[name]->enabled = false;
        }
    }

    std::shared_ptr<Generator> getGenerator(std::string name)
    {
        if (generators.find(name) != generators.end())
        {
            return generators[name];
        }
        return nullptr;
    }

    void enableExponentialOperations(bool enable)
    {
        EnableExponentialOperations = enable;
    }

    bool isExponentialOperationsEnabled() const
    {
        return EnableExponentialOperations;
    }

    void setRecursive(bool rec)
    {
        recursive = rec;
    }

    void loadAdditionalGenerators(std::filesystem::path directory, argumentParser & ap){

			// Load dynamic generators from the specified directory
			if (!std::filesystem::exists(directory))
			{
				PLOGE << "Additional generators directory does not exist: " << directory << std::endl;
				return;
			}

			for (const auto& entry : std::filesystem::directory_iterator(directory))
			{
				if (entry.is_regular_file())
				{
					std::string extension = entry.path().extension().string();
					if (extension == ".dll" || extension == ".so")
					{
						try
						{
							boost::dll::shared_library lib(entry.path().string());
							
							if (lib.has("getGeneratorInstance"))
							{
								auto getGeneratorInstance = lib.get<std::shared_ptr<Generator>()>("getGeneratorInstance");
								std::shared_ptr<Generator> generator = getGeneratorInstance();
								
								if (generator != nullptr)
								{
                                    
                                    addGenerator(generator);
                                    PLOGI << "Loaded generator '" << generator->name << "' from: " << entry.path().string() << std::endl;
                                    
                                    // Check if the generator has a registerArguments function
                                    if (lib.has("registerArguments"))
                                    {
                                        auto registerArguments = lib.get<void(argumentParser*)>("registerArguments");
                                        registerArguments(&ap);
                                        PLOGI << "Registered arguments for generator '" << generator->name << "' from: " << entry.path().string() << std::endl;
                                    }
								}
							}
							else
							{
								PLOGW << "Warning: " << entry.path().string() << " does not have getGeneratorInstance function" << std::endl;
							}
						}
						catch (const std::exception& e)
						{
							PLOGE << "Error loading generator from " << entry.path().string() << ": " << e.what() << std::endl;
						}
					}
				}
			}
    }

    void loadBuiltInGenerators(){
        addGenerator(std::make_shared<JsonGenerator>());
        addGenerator(std::make_shared<SqliteGenerator>());
        addGenerator(std::make_shared<MysqlGenerator>());
        //addGenerator(std::make_shared<LuaGenerator>());
        //addGenerator(std::make_shared<JavaGenerator>());
        addGenerator(std::make_shared<CppGenerator>());
    }

    Builder()
    {
        program_structure_ = std::make_shared<ProgramStructure>();
        loadBuiltInGenerators();
    }

    void setOutputDirectory(std::filesystem::path out_dir)
    {
        outputDirectory = out_dir;
    }

    void setSchema(std::filesystem::path schema_dir)
    {
        schema = schema_dir;
    }

    bool run()
    {
        if(debug_session_){
            program_structure_->debug_server = debug_session_;
        }


        if(!std::filesystem::exists(schema)){
            PLOGE << "Schema path does not exist: " << schema.string() << std::endl;
            return false;
        }

        // check if schema is directory or file
        if (std::filesystem::is_regular_file(schema))
        {
            // read single schema file
            if (!schema.has_extension())
            {
                printf("File has no extension: %s\n", schema.string().c_str());
                return false;
            }
            if (!(schema.extension().compare(".schema") || schema.extension().compare(".schemaLang")))
            {
                printf("File is not a schema file: %s\n", schema.string().c_str());
                return false;
            }
            if (!program_structure_->readFile(schema.string()))
            {
                PLOGE << "Failed to read file: " << schema.string() << std::endl;
                exit(1);
            }
            else
            {
                PLOGI << "Read file: " << schema.string() << std::endl;
            }
        }
        else
        {
            if (recursive)
            {
                for (const auto &entry : std::filesystem::recursive_directory_iterator(schema))
                {
                    if (entry.is_directory())
                    {
                        printf("File is a directory entering: %s\n", entry.path().string().c_str());
                        continue;
                    }
                    if (!entry.path().has_extension())
                    {
                        printf("File has no extension: %s\n", entry.path().string().c_str());
                        continue;
                    }
                    if (!(entry.path().extension().compare(".schema") || entry.path().extension().compare(".schemaLang")))
                    {
                        printf("File is not a schema file: %s\n", entry.path().string().c_str());
                        continue;
                    }
                    if (!program_structure_->readFile(entry.path().string()))
                    {
                        PLOGE << "Failed to read file: " << entry.path().string() << std::endl;
                        exit(1);
                    }
                    else
                    {
                        PLOGI << "Read file: " << entry.path().string() << std::endl;
                    }
                }
            }
            else
            {
                for (const auto &entry : std::filesystem::directory_iterator(schema))
                {
                    if (entry.is_directory())
                    {
                        printf("File is a directory entering: %s\n", entry.path().string().c_str());
                        continue;
                    }
                    if (!entry.path().has_extension())
                    {
                        printf("File has no extension: %s\n", entry.path().string().c_str());
                        continue;
                    }
                    if (!(entry.path().extension().compare(".schema") || entry.path().extension().compare(".schemaLang")))
                    {
                        printf("File is not a schema file: %s\n", entry.path().string().c_str());
                        continue;
                    }
                    if (!program_structure_->readFile(entry.path().string()))
                    {
                        PLOGE << "Failed to read file: " << entry.path().string() << std::endl;
                        exit(1);
                    }
                    else
                    {
                        PLOGI << "Read file: " << entry.path().string() << std::endl;
                    }
                }
            }
        }

        // Load migration files if migrations path is set
        if(migrations && !migrationsPath.empty()){
            if(std::filesystem::exists(migrationsPath)){
                PLOGI << "Loading migration files from: " << migrationsPath.string() << std::endl;
                if(recursive){
                    for(const auto& entry : std::filesystem::recursive_directory_iterator(migrationsPath)){
                        if(entry.is_regular_file() && entry.path().extension() == ".migration"){
                            if(!program_structure_->readMigrationFile(entry.path().string())){
                                PLOGE << "Failed to read migration file: " << entry.path().string() << std::endl;
                                return false;
                            }
                            PLOGI << "Loaded migration file: " << entry.path().string() << std::endl;
                        }
                    }
                } else {
                    for(const auto& entry : std::filesystem::directory_iterator(migrationsPath)){
                        if(entry.is_regular_file() && entry.path().extension() == ".migration"){
                            if(!program_structure_->readMigrationFile(entry.path().string())){
                                PLOGE << "Failed to read migration file: " << entry.path().string() << std::endl;
                                return false;
                            }
                            PLOGI << "Loaded migration file: " << entry.path().string() << std::endl;
                        }
                    }
                }
            } else {
                PLOGW << "Migrations path does not exist: " << migrationsPath.string() << std::endl;
            }
            
            // Auto-generate new migrations if schema versions have changed
            PLOGI << "Checking for schema version changes and auto-generating migrations..." << std::endl;
            program_structure_->autoGenerateMigrations(migrationsPath.string());
        }

        //Generate files for each generator
        for (auto &generator : generators)
        {
            if(!generator.second->enabled){
                continue;
            }
            printf("Generating files for generator '%s'\n", generator.first.c_str());
            if (!program_structure_->generate_files(generator.second, (outputDirectory / generator.first).string()))
            {
                PLOGE << "Failed to generate files for generator '" << generator.first << "'" << std::endl;
                return false;
            }
            if(migrations){
                PLOGI << "Generating migration files for generator '" << generator.first << "'" << std::endl;
                // Note: generate_migration_files returns false if migrations are not supported by this generator
                // This is not an error, so we don't fail the build
                program_structure_->generate_migration_files(generator.second, (outputDirectory / "migrations" / generator.first).string());
            }
        }

        return true;
    }

    void setSession(std::shared_ptr<session> debug_session)
    {
        debug_session_ = debug_session;
    }

    void setMigrationsPath(std::filesystem::path migrations_dir)
    {
        migrationsPath = migrations_dir;
        migrations = true;
    }

private:
    std::shared_ptr<session> debug_session_;
    std::filesystem::path schema;
    std::filesystem::path outputDirectory;
    std::filesystem::path migrationsPath;
    bool EnableExponentialOperations = false;
    bool recursive = false;
    bool migrations = false;
    std::map<std::string, std::shared_ptr<Generator>> generators;
    std::shared_ptr<ProgramStructure> program_structure_;
};