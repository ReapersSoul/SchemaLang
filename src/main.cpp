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
				  { cppGenerator->addGenerator(new JsonGenerator()); });
	ap.addFlag(&jsonFlag);
	Flag sqliteFlag("sqlite", false, [&]
					{ cppGenerator->addGenerator(new SqliteGenerator()); });
	ap.addFlag(&sqliteFlag);
	Flag mysqlFlag("mysql", false, [&]
				   { cppGenerator->addGenerator(new MysqlGenerator()); });
	ap.addFlag(&mysqlFlag);
	Flag cppFlag("cpp", false, [&]
				 { cppGenerator->addGenerator(new CppGenerator()); });
	ap.addFlag(&cppFlag);
	Flag javaFlag("java", false, [&]
				  {
					  // cppGenerator->addGenerator(new JavaGenerator());
				  });
	ap.addFlag(&javaFlag);

	Parameter schemaDirectoryParameter("schemaDirectory", true, [&](std::string value)
									   { schemaDirectory = value; });
	ap.addParameter(&schemaDirectoryParameter);
	Parameter outputDirectoryParameter("outputDirectory", true, [&](std::string value)
									   { outputDirectory = value; });
	ap.addParameter(&outputDirectoryParameter);

	if (!ap.parse(argc, argv))
	{
		return 1;
	}

	if (!std::filesystem::exists(schemaDirectory))
	{
		std::cout << "Directory does not exist: " << argv[1] << std::endl;
		return 1;
	}

	outputDirectory = outputDirectory / "Schemas";

	ProgramStructure ps;

	// generate classes for each schema file
	for (const auto &entry : std::filesystem::recursive_directory_iterator(schemaDirectory))
	{
		if (entry.is_directory())
		{
			printf("File is a directory entering: %s\n", entry.path().string().c_str());
			continue;
		}
		if(!entry.path().has_extension()){
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
			return 1;
		}
		else
		{
			std::cout << "Read file: " << entry.path().string() << std::endl;
		}
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