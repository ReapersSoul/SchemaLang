#include <Generator.hpp>
#include <ProgramStructure.hpp>

bool Generator::generate_additional_files(Generator *gen, ProgramStructure *ps, std::string out_path)
{
	std::string base_path = "/" + name + "/" + gen->name + "/";
	if (!existsEmbeddedResourcesEmbeddedFile(base_path.c_str()))
	{
		return false;
	}
	inja::Environment env;
	env.set_trim_blocks(true);

	// open file
	std::string path = base_path + "Files/";
	std::vector<std::string> files = listEmbeddedResourcesEmbeddedFiles(path.c_str());
	inja::json data = ps->to_json(gen);
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
	for (auto &file : files)
	{
		try
		{
			std::vector<uint8_t> fileData = loadEmbeddedResourcesEmbeddedFile((path + file).c_str());
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
			throw std::runtime_error("Error fetching additional files from " + file + ": " + e.what());
		}
	}
	return true;
}