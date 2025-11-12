#include <Generator.hpp>
#include <ProgramStructure.hpp>

inja::Environment Generator::getEnv(Generator *gen, ProgramStructure *ps){
	inja::Environment env;
	env.set_trim_blocks(true);
	env.set_lstrip_blocks(false);
	env.add_callback("format_include", 1, [gen](inja::Arguments &args) {
		std::string ident = args.at(0)->get<std::string>();
		return gen->format_include(ident);
	});
	env.add_callback("format_default", 2, [gen, ps](inja::Arguments &args) {
		TypeDefinition type;
		type.from_json(*args.at(0));
		std::string value = args.at(1)->get<std::string>();
		return gen->format_default(ps, type, value);
	});
	env.add_callback("convert_to_local_type", 1, [gen, ps](inja::Arguments &args) {
		TypeDefinition type;
		type.from_json(*args.at(0));
		return gen->convert_to_local_type(ps, type);
	});
	env.add_callback((name + "_format_include").c_str(), 1, [&,ps](inja::Arguments &args) {
		std::string ident = args.at(0)->get<std::string>();
		return format_include(ident);
	});
	env.add_callback((name + "_format_default").c_str(), 2, [&,ps](inja::Arguments &args) {
		TypeDefinition type;
		type.from_json(*args.at(0));
		std::string value = args.at(1)->get<std::string>();
		return format_default(ps, type, value);
	});
	env.add_callback(name + "_convert_to_local_type", 1, [&,ps](inja::Arguments &args) {
		TypeDefinition type;
		type.from_json(*args.at(0));
		return convert_to_local_type(ps, type);
	});
	for (auto &generator : generators){
		env.add_callback((generator->name + "_format_include").c_str(), 1, [generator](inja::Arguments &args) {
			std::string ident = args.at(0)->get<std::string>();
			return generator->format_include(ident);
		});
		env.add_callback((generator->name + "_format_default").c_str(), 2, [generator, &ps](inja::Arguments &args) {
			TypeDefinition type;
			type.from_json(*args.at(0));
			std::string value = args.at(1)->get<std::string>();
			return generator->format_default(ps, type, value);
		});
		env.add_callback(generator->name +"_convert_to_local_type", 1, [generator, &ps](inja::Arguments &args) {
			TypeDefinition type;
			type.from_json(*args.at(0));
			return generator->convert_to_local_type(ps, type);
		});
	}
	return env;
}

bool Generator::generate_additional_files(Generator *gen, ProgramStructure *ps, std::string out_path)
{
	std::string base_path = "/" + name + "/" + gen->name + "/";
	if (!existsSchemaLangShared_ResourcesEmbeddedFile(base_path.c_str()))
	{
		return false;
	}
	inja::Environment env = getEnv(gen, ps);
	// open file
	std::string path = base_path + "Files/";
	std::vector<std::string> files = listSchemaLangShared_ResourcesEmbeddedFiles(path.c_str());
	inja::json data = ps->to_json(gen);

	for (auto &file : files)
	{
		try
		{
			std::vector<uint8_t> fileData = loadSchemaLangShared_ResourcesEmbeddedFile((path + file).c_str());
			std::string content(reinterpret_cast<const char *>(fileData.data()), fileData.size());
			content = env.render(content, data);
			std::ofstream of(out_path + "/" + file);
			if (!of.is_open())
			{
				std::cout << "Failed to open file: " << out_path + "/" + file << std::endl;
				continue;
			}
			of << content;
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error("Generator::generate_additional_files() - Error processing additional file '" + file + "' for generator '" + name + "': " + e.what());
		}
	}
	return true;
}