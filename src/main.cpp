#include <iostream>
#include <filesystem>
#include <ArgParser/ArgParser.hpp>
#include <ProgramStructure/ProgramStructure.hpp>
#include <BuiltIn/Generators/CppGenerator/CppGenerator.hpp>
// #include <BuiltIn/Generators/JavaGenerator/JavaGenerator.hpp>
#include <BuiltIn/Generators/JsonGenerator/JsonGenerator.hpp>
#include <BuiltIn/Generators/SqliteGenerator/SqliteGenerator.hpp>
#include <BuiltIn/Generators/MySqlGenerator/MySqlGenerator.hpp>

int main(int argc, char *argv[])
{
	std::filesystem::path schemaDirectory;
	std::filesystem::path outputDirectory;
	bool EnableExponentialOperations = false;
	bool recursive = false;

	ProgramStructure ps;

	JsonGenerator *jsonGenerator = new JsonGenerator();
	SqliteGenerator *sqliteGenerator = new SqliteGenerator();
	MysqlGenerator *mysqlGenerator = new MysqlGenerator();

	CppGenerator *cppGenerator = new CppGenerator();
	// JavaGenerator* javaGenerator = new JavaGenerator();

	// parse arguments
	argumentParser ap;
	Flag helpFlag("help", false, [&]()
				  { ap.printUsage(); });
	ap.addFlag(&helpFlag);

	Flag jsonFlag("json", false, [&]
				  { cppGenerator->addGenerator(jsonGenerator); });
	ap.addFlag(&jsonFlag);
	Flag sqliteFlag("sqlite", false, [&]
					{ cppGenerator->addGenerator(sqliteGenerator); });
	ap.addFlag(&sqliteFlag);
	Flag mysqlFlag("mysql", false, [&]
				   { cppGenerator->addGenerator(mysqlGenerator); });
	ap.addFlag(&mysqlFlag);
	Flag cppFlag("cpp", false, [&]
				 { cppGenerator->addGenerator(cppGenerator); });
	ap.addFlag(&cppFlag);
	Flag javaFlag("java", false, [&]
				  {
					  // cppGenerator->addGenerator(new JavaGenerator());
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
		// if (!ps.generate_files(javaGenerator, outputDirectory/"Schemas"/"Java"/entry.path().filename().replace_extension(".java").string()))
		// {
		// 	std::cout << "Failed to generate java files" << std::endl;
		// 	return 1;
		// }
	}

	return 0;
}