#include <BuiltInGenerators/CppGenerator.hpp>
#include <stdexcept>

bool CppGenerator::generate_base_class_header_file(std::shared_ptr<Generator>gen, std::shared_ptr<ProgramStructure>ps, std::string out_path)
{
	std::ofstream baseClassFile(out_path + "/Has" + gen->base_class.getIdentifier() + "Schema.hpp");
	if (!baseClassFile.is_open())
	{
		std::cout << "Failed to open file: " << out_path + "/Has" + gen->base_class.getIdentifier() + "Schema.hpp" << std::endl;
		return false;
	}
	baseClassFile << "#pragma once\n";
	for (auto &include : gen->base_class.getIncludes())
	{
		baseClassFile << "#include " << include << "\n";
	}
	for (auto &line : gen->base_class.getBeforeLines())
	{
		baseClassFile << line << "\n";
	}
	baseClassFile << "class Has" + gen->base_class.getIdentifier() + "Schema{\n";
	baseClassFile << "public:\n";
	for (auto &f : gen->base_class.getFunctions())
	{
		baseClassFile << "\tvirtual " << convert_to_local_type(ps, f.return_type) << " " << f.identifier << "(";
		for (int i = 0; i < f.parameters.size(); i++)
		{
			baseClassFile << convert_to_local_type(ps, f.parameters[i].first) << " " << f.parameters[i].second;
			if (i < f.parameters.size() - 1)
			{
				baseClassFile << ", ";
			}
		}
		baseClassFile << ") = 0;\n";
	}
	baseClassFile << "};\n";
	baseClassFile.close();
	return true;
}

CppGenerator::CppGenerator()
{
	name = "Cpp";
	type= GeneratorType::Language;
}

bool CppGenerator::add_generator(std::shared_ptr<Generator>gen)
{
	generators.push_back(gen);
	return true;
}

std::string CppGenerator::convert_to_local_type(std::shared_ptr<ProgramStructure>ps, TypeDefinition type)
{
	// convert int types to "int"
	if (type.identifier() == INT8)
	{
		return "int8_t";
	}
	if (type.identifier() == INT16)
	{
		return "int16_t";
	}
	if (type.identifier() == INT32)
	{
		return "int32_t";
	}
	if (type.identifier() == INT64)
	{
		return "int64_t";
	}
	if (type.identifier() == UINT8)
	{
		return "uint8_t";
	}
	if (type.identifier() == UINT16)
	{
		return "uint16_t";
	}
	if (type.identifier() == UINT32)
	{
		return "uint32_t";
	}
	if (type.identifier() == UINT64)
	{
		return "uint64_t";
	}
	// convert float types to "float"
	if (type.identifier() == FLOAT)
	{
		return "float";
	}
	if (type.identifier() == DOUBLE)
	{
		return "double";
	}
	// convert bool to "bool"
	if (type.identifier() == BOOL)
	{
		return "bool";
	}
	// convert string to "std::string"
	if (type.identifier() == STRING)
	{
		return "std::string";
	}
	// convert char to "char"
	if (type.identifier() == CHAR)
	{
		return "char";
	}
	// convert array to "std::vector"f.parameters[i].first
	if (type.identifier() == ARRAY)
	{
		return "std::vector<" + convert_to_local_type(ps, type.element_type()) + ">";
	}

	// if the type is a enum, return the identifier
	if (ps->tokenIsEnum(type.identifier()))
	{
		return type.identifier() + "Schema";
	}

	// if the type is a struct, return the identifier
	if (ps->tokenIsStruct(type.identifier()))
	{
		return "std::shared_ptr<" + type.identifier() + "Schema>";
	}

	return type.identifier();
}

std::string CppGenerator::get_default_of_type(std::shared_ptr<ProgramStructure>ps, TypeDefinition type)
{
	if (type.is_integer())
	{
		return "0";
	}
	else if (type.is_real())
	{
		return "0.0";
	}
	else if (type.is_bool())
	{
		return "false";
	}
	else if (type.is_string())
	{
		return "\"\"";
	}
	else if (type.is_char())
	{
		return "'\\0'";
	}
	else if (type.is_array())
	{
		return "std::vector<" + convert_to_local_type(ps, type.element_type()) + ">()";
	}
	else if (type.is_struct(ps))
	{
		return "std::make_shared<" + type.identifier() + "Schema>()";
	}
	else if (type.is_enum(ps))
	{
		return type.identifier() + "Schema::" + type.identifier() + "_DEFAULT";
	}
	else if (type.is_optional())
	{
		return "std::nullopt";
	}
	return "";
}

std::string CppGenerator::format_include(std::string ident)
{
	std::string full_path = include_prefix.empty() ? ident : include_prefix + "/" + ident;
	if (use_angle_brackets)
	{
		return "<" + full_path + ">";
	}
	else
	{
		return "\"" + full_path + "\"";
	}
}

std::string CppGenerator::format_default(std::shared_ptr<ProgramStructure>ps, TypeDefinition type, std::string value)
{
	if(value.empty()){
		return get_default_of_type(ps, type);
	}
	return value;
}

bool CppGenerator::add_generator_specific_content_to_struct(std::shared_ptr<Generator>gen, std::shared_ptr<ProgramStructure>ps, StructDefinition &s)
{
	return true;
}

#include <inja/inja.hpp>
#include <SchemaLangShared_Resources/SchemaLangShared_ResourcesEmbeddedVFS.hpp>

bool CppGenerator::generate_files(std::shared_ptr<ProgramStructure> ps, std::string out_path)
{
	if (!std::filesystem::exists(out_path))
	{
		std::filesystem::create_directories(out_path);
	}
	// generate the base class header files
	std::vector<StructDefinition> base_classes;

	for (auto &gen : generators)
	{
		if (gen == shared_from_this())
		{
			continue;
		}
		if (!gen->base_class.getIdentifier().empty())
		{
			if (!generate_base_class_header_file(gen, ps, out_path))
			{
				printf("Error: Failed to generate base class header file for %s\n", gen->base_class.getIdentifier().c_str());
				return false;
			}
			base_classes.push_back(gen->base_class);
		}

		// for (auto &s : ps.getStructs())
		// {
		// 	if (!gen->add_generator_specific_content_to_struct(this, &ps, s))
		// 	{
		// 		printf("Error: Failed to add Generator specific functions for %s\n", gen->base_class.getIdentifier().c_str());
		// 		return false;
		// 	}
		// }
	}

	inja::Environment env = getEnv(shared_from_this(), ps);

	std::map<std::string, std::string> struct_name_content_pairs;
	// open file
	std::vector<std::string> files = listSchemaLangShared_ResourcesEmbeddedFiles("/Cpp/struct/");
	for (auto &file : files)
	{
		std::string content(reinterpret_cast<const char *>(loadSchemaLangShared_ResourcesEmbeddedFile(("/Cpp/struct/" + file).c_str()).data()), loadSchemaLangShared_ResourcesEmbeddedFile(("/Cpp/struct/" + file).c_str()).size());
		std::string filename = std::filesystem::path(file).filename().string();
		struct_name_content_pairs[filename] = content;
	}

	std::map<std::string, std::string> enum_name_content_pairs;
	files = listSchemaLangShared_ResourcesEmbeddedFiles("/Cpp/enum/");
	for (auto &file : files)
	{
		std::string content(reinterpret_cast<const char *>(loadSchemaLangShared_ResourcesEmbeddedFile(("/Cpp/enum/" + file).c_str()).data()), loadSchemaLangShared_ResourcesEmbeddedFile(("/Cpp/enum/" + file).c_str()).size());
		std::string filename = std::filesystem::path(file).filename().string();
		enum_name_content_pairs[filename] = content;
	}

	for (auto &s : ps->getStructs())
	{
		inja::json data = ps->to_json(shared_from_this());
		inja::json struct_data=s.to_json(ps, shared_from_this());
		for (auto& [key, value] : struct_data.items()) {
		    data[key] = value;
		}
		
		data["includePrefix"]=include_prefix;
		data["useAngleBrackets"] = use_angle_brackets;

		data["header_include"] = format_include(s.getIdentifier()+"Schema.hpp");
		try
		{
			for (auto &file : struct_name_content_pairs)
			{
				// render output path in its own try/catch
				std::string rendered_path;
				try
				{
					rendered_path = env.render(out_path + "/" + file.first, data);
				}
				catch (const std::exception &e)
				{
					throw std::runtime_error(std::string("Error rendering output path for ") + file.first + ": " + e.what());
				}

				std::ofstream of(rendered_path);
				if (!of.is_open())
				{
					std::cout << "Failed to open file: " << rendered_path << std::endl;
				}
				if (file.first.find(".hpp") != std::string::npos)
				{
					data["header"] = true;
				}
				else
				{
					data["header"] = false;
				}

				// additions
				data["additions"] = inja::json::array();
				for (auto &gen : generators)
				{
					if (gen == shared_from_this())
						continue;
					// Use a fresh Additions object per-generator so fetched additions don't accumulate
					Additions additions;
					if (!gen->fetch_additions(ps, shared_from_this(), additions, data))
					{
						printf("Warning: Failed to fetch additions from generator %s\n", gen->name.c_str());
						continue;
					}

					inja::json addition_data;
					addition_data["gen_name"] = gen->name;
					addition_data["includes"] = inja::json::array();
					for (auto &inc : additions.includes)
					{
						addition_data["includes"].push_back(inc);
					}

					addition_data["before_setter_lines"] = inja::json::array();
					for (auto &before_setter_line : additions.before_setter_lines)
					{
						addition_data["before_setter_lines"].push_back(before_setter_line);
					}

					addition_data["before_getter_lines"] = inja::json::array();
					for (auto &before_getter_line : additions.before_getter_lines)
					{
						addition_data["before_getter_lines"].push_back(before_getter_line);
					}

					addition_data["functions"] = inja::json::array();
					for (auto &func : additions.functions)
					{
						addition_data["functions"].push_back(func);
					}

					addition_data["private_variables"] = inja::json::array();
					for (auto &pv : additions.private_variables)
					{
						addition_data["private_variables"].push_back(pv);
					}

					addition_data["member_variables"] = inja::json::array();
					for (auto &mv : additions.member_variables)
					{
						addition_data["member_variables"].push_back(mv);
					}

					data["additions"].push_back(addition_data);
				}

				// render template content in its own try/catch
				std::string rendered_content;
				try
				{
					rendered_content = env.render(file.second, data);
				}
				catch (const std::exception &e)
				{
					throw std::runtime_error(std::string("Error rendering template ") + file.first + ": " + e.what());
				}

				of << rendered_content;
				of.close();
				std::cout << "Generated file: " << rendered_path << std::endl;
			}
		}
		catch (const std::exception &e)
		{
			std::cout << "Error generating file for struct " << s.getIdentifier() << ": " << e.what() << std::endl;
			return false;
		}
	}

	for (auto &e : ps->getEnums())
	{
		inja::json data;
		data["enum"] = e.identifier;
		data["enum_include"] = format_include(e.identifier+"Schema.hpp");

		data["values"] = inja::json::array();
		for (auto &v : e.values)
		{
			inja::json value_data;
			value_data["identifier"] = v.first;
			std::string identifierCamel = v.first;
			std::function<std::string(std::string)> capitalFirst = [](std::string str)
			{
				if (str.empty())
					return str;
				str[0] = std::toupper(str[0]);
				return str;
			};
			identifierCamel = capitalFirst(identifierCamel);
			value_data["identifierCamel"] = identifierCamel;
			value_data["value"] = v.second;
			data["values"].push_back(value_data);
		}

		try
		{
			for (auto &file : enum_name_content_pairs)
			{
				// render output path separately
				std::string rendered_path;
				try
				{
					rendered_path = env.render(out_path + "/" + file.first, data);
				}
				catch (const std::exception &e)
				{
					throw std::runtime_error(std::string("Error rendering output path for ") + file.first + ": " + e.what());
				}

				std::ofstream of(rendered_path);
				if (!of.is_open())
				{
					std::cout << "Failed to open file: " << rendered_path << std::endl;
				}

				// render template content separately
				std::string rendered_content;
				try
				{
					rendered_content = env.render(file.second, data);
				}
				catch (const std::exception &ex)
				{
					throw std::runtime_error(std::string("Error rendering template ") + file.first + ": " + ex.what());
				}

				of << rendered_content;
				of.close();
				std::cout << "Generated file: " << rendered_path << std::endl;
			}
		}
		catch (const std::exception &ex)
		{
			std::cout << "Error generating file for enum " << e.identifier << ": " << ex.what() << std::endl;
			return false;
		}
	}

	files = listSchemaLangShared_ResourcesEmbeddedFiles("/Cpp/files/");
	for (auto &file : files)
	{
		std::string content(reinterpret_cast<const char *>(loadSchemaLangShared_ResourcesEmbeddedFile(("/Cpp/files/" + file).c_str()).data()), loadSchemaLangShared_ResourcesEmbeddedFile(("/Cpp/files/" + file).c_str()).size());
		std::string filename = std::filesystem::path(file).filename().string();
		inja::json data=ps->to_json(shared_from_this());
		std::ofstream of(out_path + "/" + filename);
		if (!of.is_open())
		{
			std::cout << "Failed to open file: " << out_path + "/" + filename << std::endl;
		}
		try
		{
			std::string rendered_content = env.render(content, data);
			of << rendered_content;
			of.close();
			std::cout << "Generated file: " << out_path + "/" + filename << std::endl;
		}
		catch (const std::exception &e)
		{
			std::cout << "Error rendering template " << filename << ": " << e.what() << std::endl;
			return false;
		}
	}

	for (auto &gen : generators)
	{
		if (gen == shared_from_this())
		{
			continue;
		}
		try
		{
			if (!gen->generate_additional_files(shared_from_this(), ps, out_path))
			{
				printf("Warning: Failed to generate additional files from generator %s\n", gen->name.c_str());
				continue;
			}
		}
		catch (const std::exception &e)
		{
			printf("Warning: Exception generating additional files from generator %s: %s\n", gen->name.c_str(), e.what());
			continue;
		}
	}

	return true;
}
