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
	for (auto &file : files)
	{
		try
		{
			std::vector<uint8_t> fileData = loadEmbeddedResourcesEmbeddedFile((path + file).c_str());
			std::string content(reinterpret_cast<const char *>(fileData.data()), fileData.size());
			content = env.render(content, ps->to_json(gen));
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