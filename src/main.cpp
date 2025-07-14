#include <iostream>
#include <filesystem>
#include <vector>
#include <ArgParser/ArgParser.hpp>
#include <ProgramStructure.hpp>
#include <BuiltInGenerators/CppGenerator.hpp>
#include <BuiltInGenerators/JavaGenerator.hpp>
#include <BuiltInGenerators/JsonGenerator.hpp>
#include <BuiltInGenerators/SqliteGenerator.hpp>
#include <BuiltInGenerators/MySqlGenerator.hpp>
#include <BuiltInGenerators/LuaGenerator.hpp>
#include <boost/dll.hpp>
#include <boost/function.hpp>
#include <EmbeddedResources/EmbeddedResourcesEmbeddedVFS.hpp>

int main(int argc, char *argv[])
{
	if(!initEmbeddedResourcesEmbeddedVFS(argv[0])){
		std::cerr << "Failed to initialize embedded resources VFS." << std::endl;
		return 1;
	}
	if(!mountEmbeddedResourcesEmbeddedVFS()){
		std::cerr << "Failed to mount embedded resources VFS." << std::endl;
		return 1;
	}

	std::filesystem::path schemaDirectory;
	std::filesystem::path outputDirectory;
	std::filesystem::path additionalGeneratorsDirectory;
	bool EnableExponentialOperations = false;
	bool recursive = false;

	ProgramStructure ps;
	std::vector<Generator*> dynamicGenerators;
	std::vector<std::string> dynamicGeneratorNames;

	JsonGenerator *jsonGenerator = new JsonGenerator();
	SqliteGenerator *sqliteGenerator = new SqliteGenerator();
	MysqlGenerator *mysqlGenerator = new MysqlGenerator();
	LuaGenerator *luaGenerator = new LuaGenerator();
	JavaGenerator *javaGenerator = new JavaGenerator();

	CppGenerator *cppGenerator = new CppGenerator();

	// parse arguments
	argumentParser ap;
	Flag helpFlag("help", false, [&]()
				  { ap.printUsage(); });
	ap.addFlag(&helpFlag);

	Parameter additionalGeneratorsParameter("additionalGenerators", false, [&](std::string value)
		{
			additionalGeneratorsDirectory = value;
			
			// Load dynamic generators from the specified directory
			if (!std::filesystem::exists(additionalGeneratorsDirectory))
			{
				std::cout << "Additional generators directory does not exist: " << additionalGeneratorsDirectory << std::endl;
				return;
			}

			for (const auto& entry : std::filesystem::directory_iterator(additionalGeneratorsDirectory))
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
								auto getGeneratorInstance = lib.get<Generator*()>("getGeneratorInstance");
								Generator* generator = getGeneratorInstance();
								
								if (generator != nullptr)
								{
									// Get the generator name (now required)
									if (lib.has("getGeneratorName"))
									{
										auto getGeneratorName = lib.get<const char*()>("getGeneratorName");
										std::string generatorName = getGeneratorName();
										
										dynamicGenerators.push_back(generator);
										dynamicGeneratorNames.push_back(generatorName);
										std::cout << "Loaded generator '" << generatorName << "' from: " << entry.path().string() << std::endl;
										
										// Check if the generator has a registerArguments function
										if (lib.has("registerArguments"))
										{
											auto registerArguments = lib.get<void(argumentParser*)>("registerArguments");
											registerArguments(&ap);
											std::cout << "Registered arguments for generator '" << generatorName << "' from: " << entry.path().string() << std::endl;
										}
									}
									else
									{
										std::cout << "Error: " << entry.path().string() << " does not have required getGeneratorName function" << std::endl;
									}
								}
							}
							else
							{
								std::cout << "Warning: " << entry.path().string() << " does not have getGeneratorInstance function" << std::endl;
							}
						}
						catch (const std::exception& e)
						{
							std::cout << "Error loading generator from " << entry.path().string() << ": " << e.what() << std::endl;
						}
					}
				}
			}
		}, INT32_MAX);
	ap.addParameter(&additionalGeneratorsParameter);

	Flag jsonFlag("json", false, [&]
				  { cppGenerator->add_generator(jsonGenerator); });
	ap.addFlag(&jsonFlag);
	Flag luaFlag("lua", false, [&]
				 { cppGenerator->add_generator(luaGenerator); });
	ap.addFlag(&luaFlag);
	Flag sqliteFlag("sqlite", false, [&]
					{ cppGenerator->add_generator(sqliteGenerator); });
	ap.addFlag(&sqliteFlag);
	Flag mysqlFlag("mysql", false, [&]
				   { cppGenerator->add_generator(mysqlGenerator); });
	ap.addFlag(&mysqlFlag);
	Flag cppFlag("cpp", false, [&]
				 { cppGenerator->add_generator(cppGenerator); });
	ap.addFlag(&cppFlag);

	// C++ specific parameters
	Parameter cppIncludePrefixParameter("cppIncludePrefix", false, [&](std::string value)
		{ cppGenerator->set_include_prefix(value); });
	ap.addParameter(&cppIncludePrefixParameter);

	Flag cppUseAngleBracketsFlag("cppUseAngleBrackets", false, [&]
		{ cppGenerator->set_use_angle_brackets(true); });
	ap.addFlag(&cppUseAngleBracketsFlag);

	Flag javaFlag("java", false, [&]
				  {
					  // Java generator flag - just enables Java generation
				  });
	ap.addFlag(&javaFlag);

	Parameter schemaDirectoryParameter("schemaDirectory", true, [&](std::string value)
									   { 
										schemaDirectory = value; 
									// generate classes for each schema file
	if (recursive)
	{
		for (const auto &entry : std::filesystem::recursive_directory_iterator(schemaDirectory))
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
			if (!ps.readFile(entry.path().string()))
			{
				std::cout << "Failed to read file: " << entry.path().string() << std::endl;
				exit(1);
			}
			else
			{
				std::cout << "Read file: " << entry.path().string() << std::endl;
			}
		}
	}
	else
	{
		for (const auto &entry : std::filesystem::directory_iterator(schemaDirectory))
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
			if (!ps.readFile(entry.path().string()))
			{
				std::cout << "Failed to read file: " << entry.path().string() << std::endl;
				exit(1);
			}
			else
			{
				std::cout << "Read file: " << entry.path().string() << std::endl;
			}
		}
	} }, 4);
	ap.addParameter(&schemaDirectoryParameter);
	Parameter outputDirectoryParameter("outputDirectory", true, [&](std::string value)
									   { outputDirectory = value; outputDirectory = outputDirectory / "Schemas";; });
	ap.addParameter(&outputDirectoryParameter);

	// flags for exponentiall opperations
	Flag enableExponentialOperationsFlag("enableExponentialOperations", false, [&]()
										 { EnableExponentialOperations = true; }, 5);
	ap.addFlag(&enableExponentialOperationsFlag);

	// Lambda to handle exponential operations warning
	auto exponentialWarning = [&](const std::string &flagName)
	{
		if (!EnableExponentialOperations)
		{
			std::cout << "Are you sure you want to enable " << flagName << "? This will generate an " << ps.getUniqueSubsetCount() << " files (all unique combinations of fields for each struct including subsets). This is not recommended for large schema files. If you are sure, please use the --enableExponentialOperations flag to enable this feature." << std::endl;
			exit(1);
		}
	};

	Flag selectAllFilesFlag("selectAllFiles", false, [&]()
							{
		exponentialWarning("selectAllFiles");
		mysqlGenerator->set_generate_select_all_files(true); }, 2);
	ap.addFlag(&selectAllFilesFlag);

	Flag selectFilesFlag("selectFiles", false, [&]()
						 {
		exponentialWarning("selectFiles");
		mysqlGenerator->set_generate_select_files(true); }, 2);
	ap.addFlag(&selectFilesFlag);

	Flag insertFilesFlag("insertFiles", false, [&]()
						 {
		exponentialWarning("insertFiles");
		mysqlGenerator->set_generate_insert_files(true); }, 2);
	ap.addFlag(&insertFilesFlag);

	Flag updateFilesFlag("updateFiles", false, [&]()
						 {
		exponentialWarning("updateFiles");
		mysqlGenerator->set_generate_update_files(true); }, 2);
	ap.addFlag(&updateFilesFlag);

	Flag deleteFilesFlag("deleteFiles", false, [&]()
						 {
		exponentialWarning("deleteFiles");
		mysqlGenerator->set_generate_delete_files(true); }, 2);
	ap.addFlag(&deleteFilesFlag);

	// -R for recursive directory iterator
	Flag recursiveFlag("R", false, [&]()
					   { recursive = true; }, 6);
	ap.addFlag(&recursiveFlag);

	if (!ap.parse(argc, argv))
	{
		return 1;
	}

	// Set up generator interactions
	std::vector<Generator*> allGenerators = {jsonGenerator, luaGenerator, sqliteGenerator, mysqlGenerator, javaGenerator, cppGenerator};
	
	// Add dynamic generators to built-in generators
	for (Generator* dynamicGen : dynamicGenerators)
	{
		cppGenerator->add_generator(dynamicGen);
	}
	
	// Add all built-in generators and other dynamic generators to each dynamic generator
	for (Generator* dynamicGen : dynamicGenerators)
	{
		// Add built-in generators
		for (Generator* builtInGen : allGenerators)
		{
			dynamicGen->add_generator(builtInGen);
		}
		
		// Add other dynamic generators
		for (Generator* otherDynamicGen : dynamicGenerators)
		{
			if (otherDynamicGen != dynamicGen)
			{
				dynamicGen->add_generator(otherDynamicGen);
			}
		}
	}
	
	// Set up drop-in system for Java and Lua generators (only if those generators are enabled)
	if (javaFlag.getValue())
	{
		javaGenerator->add_generator(jsonGenerator);
		javaGenerator->add_generator(sqliteGenerator);
		javaGenerator->add_generator(mysqlGenerator);
		javaGenerator->add_generator(luaGenerator);
		javaGenerator->add_generator(cppGenerator);
	}
	
	if (luaFlag.getValue())
	{
		luaGenerator->add_generator(jsonGenerator);
		luaGenerator->add_generator(sqliteGenerator);  
		luaGenerator->add_generator(mysqlGenerator);
		luaGenerator->add_generator(javaGenerator);
		luaGenerator->add_generator(cppGenerator);
	}
	
	// Set up drop-in system for C++ generator (only add generators that are enabled)
	if (javaFlag.getValue())
	{
		cppGenerator->add_generator(javaGenerator);
	}

	if (!std::filesystem::exists(schemaDirectory))
	{
		std::cout << "Directory does not exist: " << argv[1] << std::endl;
		return 1;
	}

	if (jsonFlag.getValue())
	{
		printf("Generating json files\n");
		if (!ps.generate_files(jsonGenerator, (outputDirectory / "Json").string()))
		{
			std::cout << "Failed to generate json files" << std::endl;
			return 1;
		}
	}

	if (luaFlag.getValue())
	{
		printf("Generating lua files\n");
		if (!ps.generate_files(luaGenerator, (outputDirectory / "Lua").string()))
		{
			std::cout << "Failed to generate lua files" << std::endl;
			return 1;
		}
	}

	if (sqliteFlag.getValue())
	{
		printf("Generating sqlite files\n");
		if (!ps.generate_files(sqliteGenerator, (outputDirectory / "Sqlite").string()))
		{
			std::cout << "Failed to generate sqlite files" << std::endl;
			return 1;
		}
	}

	if (mysqlFlag.getValue())
	{
		printf("Generating mysql files\n");
		if (!ps.generate_files(mysqlGenerator, (outputDirectory / "Mysql").string()))
		{
			std::cout << "Failed to generate mysql files" << std::endl;
			return 1;
		}
	}

	if (cppFlag.getValue())
	{
		printf("Generating cpp files\n");
		if (!ps.generate_files(cppGenerator, (outputDirectory / "Cpp").string()))
		{
			std::cout << "Failed to generate cpp files" << std::endl;
			return 1;
		}
	}

	if (javaFlag.getValue())
	{
		printf("Generating java files\n");
		if (!ps.generate_files(javaGenerator, (outputDirectory / "Java").string()))
		{
			std::cout << "Failed to generate java files" << std::endl;
			return 1;
		}
	}

	// Generate files for dynamic generators
	for (size_t i = 0; i < dynamicGenerators.size(); i++)
	{
		printf("Generating files for dynamic generator '%s'\n", dynamicGeneratorNames[i].c_str());
		if (!ps.generate_files(dynamicGenerators[i], (outputDirectory / dynamicGeneratorNames[i]).string()))
		{
			std::cout << "Failed to generate files for dynamic generator '" << dynamicGeneratorNames[i] << "'" << std::endl;
			return 1;
		}
	}

	return 0;
}