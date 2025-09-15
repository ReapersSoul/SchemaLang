#pragma once
#include <ForwardDeclerations.hpp>
#include <StructDefinition.hpp>
#include <inja/inja.hpp>
#include <EmbeddedResources/EmbeddedResourcesEmbeddedVFS.hpp>

struct Generator
{
	std::vector<Generator *> generators;
	StructDefinition base_class;
	std::string name;

	virtual std::string format_include(std::string ident) = 0;
	virtual std::string format_default(ProgramStructure *ps, TypeDefinition type, std::string value = "") = 0;

	virtual std::string convert_to_local_type(ProgramStructure *ps, TypeDefinition type) = 0;

	virtual bool add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s) = 0;

	virtual bool generate_files(ProgramStructure ps, std::string out_path) = 0;

	virtual bool fetch_additions(ProgramStructure *ps, Generator *gen, Additions &additions, inja::json data)
	{

		std::string base_path = "/" + name + "/" + gen->name + "/";
		if (!existsEmbeddedResourcesEmbeddedFile(base_path.c_str()))
		{
			printf("Warning: No embedded resources found for generator %s at path %s\n", gen->name.c_str(), base_path.c_str());
			return false;
		}
		inja::Environment env;
		env.set_trim_blocks(true);
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
		// open file
		std::string path = base_path + "Functions/";
		std::vector<std::string> files = listEmbeddedResourcesEmbeddedFiles(path.c_str());
		for (auto &file : files)
		{
			try
			{
				std::vector<uint8_t> fileData = loadEmbeddedResourcesEmbeddedFile((path + file).c_str());
				std::string content(reinterpret_cast<const char *>(fileData.data()), fileData.size());
				additions.functions.insert(env.render(content, data));
			}
			catch (const std::exception &e)
			{
				throw std::runtime_error("Error fetching additions from " + file + ": " + e.what());
			}
		}
		path = base_path + "Getters/";
		files = listEmbeddedResourcesEmbeddedFiles(path.c_str());
		for (auto &file : files)
		{
			try
			{
				std::vector<uint8_t> fileData = loadEmbeddedResourcesEmbeddedFile((path + file).c_str());
				std::string content(reinterpret_cast<const char *>(fileData.data()), fileData.size());
				additions.before_getter_lines.push_back(env.render(content, data));
			}
			catch (const std::exception &e)
			{
				throw std::runtime_error("Error fetching additions from " + file + ": " + e.what());
			}
		}
		path = base_path + "Setters/";
		files = listEmbeddedResourcesEmbeddedFiles(path.c_str());
		for (auto &file : files)
		{
			try
			{
				std::vector<uint8_t> fileData = loadEmbeddedResourcesEmbeddedFile((path + file).c_str());
				std::string content(reinterpret_cast<const char *>(fileData.data()), fileData.size());
				additions.before_setter_lines.push_back(env.render(content, data));
			}
			catch (const std::exception &e)
			{
				throw std::runtime_error("Error fetching additions from " + file + ": " + e.what());
			}
		}
		path = base_path + "Variables/";
		files = listEmbeddedResourcesEmbeddedFiles(path.c_str());
		for (auto &file : files)
		{
			try
			{
				std::vector<uint8_t> fileData = loadEmbeddedResourcesEmbeddedFile((path + file).c_str());
				std::string content(reinterpret_cast<const char *>(fileData.data()), fileData.size());
				additions.private_variables.push_back(env.render(content, data));
			}
			catch (const std::exception &e)
			{
				throw std::runtime_error("Error fetching additions from " + file + ": " + e.what());
			}
		}
		path = base_path;
		std::stringstream ss;
		std::vector<uint8_t> fileData = loadEmbeddedResourcesEmbeddedFile((path + "includes.list").c_str());
		std::string content(reinterpret_cast<const char *>(fileData.data()), fileData.size());
		try
		{
			ss << env.render(content, data);
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error(std::string("Error fetching additions from includes.list: ") + e.what());
		}

		while (std::getline(ss, content))
		{
			additions.includes.insert(content);
		}
		return true;
	}

	virtual bool generate_additional_files(Generator *gen, ProgramStructure * ps, std::string out_path);

	virtual bool add_generator(Generator *gen)
	{
		return false;
	}
};