#include <iostream>
#include <filesystem>
#include <vector>
#include <ArgParser/ArgParser.hpp>
#include <ProgramStructure.hpp>
#include <BuiltInGenerators/CppGenerator.hpp>
// #include <BuiltInGenerators/JavaGenerator.hpp>
#include <BuiltInGenerators/JsonGenerator.hpp>
#include <BuiltInGenerators/SqliteGenerator.hpp>
#include <BuiltInGenerators/MySqlGenerator.hpp>
// #include <BuiltInGenerators/LuaGenerator.hpp>
#include <SchemaLangShared_Resources/SchemaLangShared_ResourcesEmbeddedVFS.hpp>

// SchemaLang version info
#include <SchemaLangVersion.hpp>

#include <Networking/DebugServer.hpp>

#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>

int main(int argc, char *argv[])
{
	if (!initSchemaLangShared_ResourcesEmbeddedVFS(argv[0]))
	{
		std::cerr << "Failed to initialize embedded resources VFS." << std::endl;
		return 1;
	}
	if (!mountSchemaLangShared_ResourcesEmbeddedVFS())
	{
		std::cerr << "Failed to mount embedded resources VFS." << std::endl;
		return 1;
	}

	//init plog
	plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
	plog::RollingFileAppender<plog::TxtFormatter> fileAppender("schemalang_transpiler.log", 1000000, 3);
	plog::init(plog::debug, &consoleAppender);
	plog::get()->addAppender(&fileAppender);

	bool startDebugger = false;
	unsigned short port = 8902;
	std::shared_ptr<DebugServer> dbServer = std::make_shared<DebugServer>(port);
	Builder builder;

	// parse arguments
	argumentParser ap;
	Flag helpFlag("help", false, [&]()
				  { 
					PLOGI << "SchemaLang Transpiler v" 
							  << SCHEMALANG_VERSION_MAJOR << "."
							  << SCHEMALANG_VERSION_MINOR << "."
							  << SCHEMALANG_VERSION_PATCH << std::endl;
					ap.printUsage(); });
	ap.addFlag(&helpFlag);

	Flag versionFlag("version", false, [&]()
					 { 
						PLOGI << "SchemaLang Transpiler v" 
								  << SCHEMALANG_VERSION_MAJOR << "."
								  << SCHEMALANG_VERSION_MINOR << "."
								  << SCHEMALANG_VERSION_PATCH << std::endl;
						exit(0); });
	ap.addFlag(&versionFlag);

	Parameter portParameter("port", false, [&](std::string value)
							{
			try
			{
				dbServer->setPort(static_cast<unsigned short>(std::stoul(value)));
			}
			catch (const std::exception& e)
			{
				PLOGE << "Invalid port number: " << value << std::endl;
				exit(1);
			} }, -3);
	ap.addParameter(&portParameter);

	Flag debuggerFlag("debugger", false, [&]()
					  { 
						startDebugger=true;
					dbServer->start();
					dbServer->setOnNewSessionCallback([&builder](std::shared_ptr<session> new_session){
						builder.setSession(new_session);
					}); }, -2);
	ap.addFlag(&debuggerFlag);

	Parameter additionalGeneratorsParameter("additionalGenerators", false, [&](std::string value)
											{ builder.loadAdditionalGenerators(value, ap); }, INT32_MAX);
	ap.addParameter(&additionalGeneratorsParameter);

	Flag jsonFlag("json", false, [&]
				  { builder.enableGenerator("Json"); });
	ap.addFlag(&jsonFlag);
	// Flag luaFlag("lua", false, [&]
	//			 { builder.enableGenerator("Lua"); });
	// ap.addFlag(&luaFlag);
	Flag sqliteFlag("sqlite", false, [&]
					{ builder.enableGenerator("SQLite"); });
	ap.addFlag(&sqliteFlag);
	Flag mysqlFlag("mysql", false, [&]
				   { builder.enableGenerator("MySQL"); });
	ap.addFlag(&mysqlFlag);
	Flag cppFlag("cpp", false, [&]
				 { builder.enableGenerator("Cpp"); });
	ap.addFlag(&cppFlag);

	// C++ specific parameters
	Parameter cppIncludePrefixParameter("cppIncludePrefix", false, [&](std::string value)
										{ 
			std::shared_ptr<CppGenerator> cppGenerator = std::dynamic_pointer_cast<CppGenerator>(builder.getGenerator("Cpp"));
			if (cppGenerator){
				cppGenerator->set_include_prefix(value);
			} });
	ap.addParameter(&cppIncludePrefixParameter);

	Flag cppUseAngleBracketsFlag("cppUseAngleBrackets", false, [&]
								 { 
			std::shared_ptr<CppGenerator> cppGenerator = std::dynamic_pointer_cast<CppGenerator>(builder.getGenerator("Cpp"));
			if (cppGenerator){
				cppGenerator->set_use_angle_brackets(true);
 			} });
	ap.addFlag(&cppUseAngleBracketsFlag);

	Flag javaFlag("java", false, [&]
				  {
					  // Java generator flag - just enables Java generation
				  });
	ap.addFlag(&javaFlag);

	// Single schema file parameter
	Parameter schemaFileParameter("schema", false, [&](std::string value)
								  { builder.setSchema(value); }, 4);
	ap.addParameter(&schemaFileParameter);

	Parameter outputDirectoryParameter("outputDirectory", false, [&](std::string value)
									   { builder.setOutputDirectory(value + "/Schemas"); });
	ap.addParameter(&outputDirectoryParameter);

	// flags for exponentiall opperations
	Flag enableExponentialOperationsFlag("enableExponentialOperations", false, [&]()
										 { builder.enableExponentialOperations(true); }, 5);
	ap.addFlag(&enableExponentialOperationsFlag);

	// Lambda to handle exponential operations warning
	auto exponentialWarning = [&](const std::string &flagName)
	{
		if (!builder.isExponentialOperationsEnabled())
		{
			PLOGW << "Are you sure you want to enable " << flagName << "? This will generate an " << builder.getUniqueSubsetCount() << " files (all unique combinations of fields for each struct including subsets). This is not recommended for large schema files. If you are sure, please use the --enableExponentialOperations flag to enable this feature." << std::endl;
			exit(1);
		}
	};

	Flag selectAllFilesFlag("selectAllFiles", false, [&]()
							{
		exponentialWarning("selectAllFiles");
		std::shared_ptr<MysqlGenerator> mysqlGenerator = std::dynamic_pointer_cast<MysqlGenerator>(builder.getGenerator("MySQL"));
			if (mysqlGenerator){
				mysqlGenerator->set_generate_select_all_files(true);
			} }, 2);
	ap.addFlag(&selectAllFilesFlag);

	Flag selectFilesFlag("selectFiles", false, [&]()
						 {
		exponentialWarning("selectFiles");
		std::shared_ptr<MysqlGenerator> mysqlGenerator = std::dynamic_pointer_cast<MysqlGenerator>(builder.getGenerator("MySQL"));
			if (mysqlGenerator){
				mysqlGenerator->set_generate_select_files(true);
 } }, 2);
	ap.addFlag(&selectFilesFlag);

	Flag insertFilesFlag("insertFiles", false, [&]()
						 {
		exponentialWarning("insertFiles");
		std::shared_ptr<MysqlGenerator> mysqlGenerator = std::dynamic_pointer_cast<MysqlGenerator>(builder.getGenerator("MySQL"));
			if (mysqlGenerator){
				mysqlGenerator->set_generate_insert_files(true);
 } }, 2);
	ap.addFlag(&insertFilesFlag);

	Flag updateFilesFlag("updateFiles", false, [&]()
						 {
		exponentialWarning("updateFiles");
		std::shared_ptr<MysqlGenerator> mysqlGenerator = std::dynamic_pointer_cast<MysqlGenerator>(builder.getGenerator("MySQL"));
			if (mysqlGenerator){
				mysqlGenerator->set_generate_update_files(true); 
	} }, 2);
	ap.addFlag(&updateFilesFlag);

	Flag deleteFilesFlag("deleteFiles", false, [&]()
						 {
		exponentialWarning("deleteFiles");
		std::shared_ptr<MysqlGenerator> mysqlGenerator = std::dynamic_pointer_cast<MysqlGenerator>(builder.getGenerator("MySQL"));
			if (mysqlGenerator){
				mysqlGenerator->set_generate_delete_files(true); 
} }, 2);
	ap.addFlag(&deleteFilesFlag);

	// -R for recursive directory iterator
	Flag recursiveFlag("R", false, [&]()
					   { builder.setRecursive(true); }, 6);
	ap.addFlag(&recursiveFlag);

	if (!ap.parse(argc, argv))
	{
		return 1;
	}

	if (!startDebugger)
	{
		builder.run();
	}
	else
	{
		dbServer->join();
	}

	return 0;
}