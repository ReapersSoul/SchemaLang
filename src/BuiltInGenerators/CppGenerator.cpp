#include <BuiltInGenerators/CppGenerator.hpp>

bool CppGenerator::generate_base_class_header_file(Generator *gen, ProgramStructure *ps, std::string out_path)
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
		baseClassFile << "#include " << include.second << "\n";
	}
	for (auto &line : gen->base_class.getBeforeLines())
	{
		baseClassFile << line.second << "\n";
	}
	baseClassFile << "class Has" + gen->base_class.getIdentifier() + "Schema{\n";
	baseClassFile << "public:\n";
	for (auto & [generator, f] : gen->base_class.getFunctions())
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
	name="Cpp";
}

bool CppGenerator::add_generator(Generator *gen)
{
	generators.push_back(gen);
	return true;
}

std::string CppGenerator::convert_to_local_type(ProgramStructure *ps, TypeDefinition type)
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
		return type.identifier() + "Schema *";
	}

	return type.identifier();
}


std::string CppGenerator::get_default_of_type(ProgramStructure * ps, TypeDefinition type)
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
		return type.identifier() + "Schema()";
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

std::string CppGenerator::format_include(const std::string& filename) const
{
	std::string full_path = include_prefix.empty() ? filename : include_prefix + "/" + filename;
	if (use_angle_brackets)
	{
		return "<" + full_path + ">";
	}
	else
	{
		return "\"" + full_path + "\"";
	}
}

bool CppGenerator::add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	return true;
}

#include <inja/inja.hpp>
#include <EmbeddedResources/EmbeddedResourcesEmbeddedVFS.hpp>

bool CppGenerator::generate_files(ProgramStructure ps, std::string out_path)
{
	if (!std::filesystem::exists(out_path))
	{
		std::filesystem::create_directories(out_path);
	}
	// generate the base class header files
	std::vector<StructDefinition> base_classes;

	for (auto &gen : generators)
	{
		if (gen == this)
		{          
			continue;
		}
		if (!gen->base_class.getIdentifier().empty())
		{
			if (!generate_base_class_header_file(gen, &ps, out_path))
			{
				printf("Error: Failed to generate base class header file for %s\n", gen->base_class.getIdentifier().c_str());
				return false;
			}
			base_classes.push_back(gen->base_class);
		}

		for (auto &s : ps.getStructs())
		{
			if (!gen->add_generator_specific_content_to_struct(this, &ps, s))
			{
				printf("Error: Failed to add Generator specific functions for %s\n", gen->base_class.getIdentifier().c_str());
				return false;
			}
		}
	}

	inja::Environment env;
	env.set_trim_blocks(true);
	// env.set_lstrip_blocks(true);

	std::map<std::string, std::string> struct_name_content_pairs;
	// open file
	std::vector<std::string> files = listEmbeddedResourcesEmbeddedFiles("/Cpp/struct/");
	for (auto &file : files)
	{
		std::string content(reinterpret_cast<const char *>(loadEmbeddedResourcesEmbeddedFile(("/Cpp/struct/" + file).c_str()).data()), loadEmbeddedResourcesEmbeddedFile(("/Cpp/struct/" + file).c_str()).size());
		std::string filename = std::filesystem::path(file).filename().string();
		struct_name_content_pairs[filename] = content;
	}

	std::map<std::string, std::string> enum_name_content_pairs;
	files = listEmbeddedResourcesEmbeddedFiles("/Cpp/enum/");
	for (auto &file : files)
	{
		std::string content(reinterpret_cast<const char *>(loadEmbeddedResourcesEmbeddedFile(("/Cpp/enum/" + file).c_str()).data()), loadEmbeddedResourcesEmbeddedFile(("/Cpp/enum/" + file).c_str()).size());
		std::string filename = std::filesystem::path(file).filename().string();
		enum_name_content_pairs[filename] = content;
	}

	for (auto &s : ps.getStructs())
	{
		inja::json data;
		data["struct"] = s.getIdentifier();
		data["struct_include"] = format_include(s.getIdentifier() + "Schema.hpp");

		data["generators"] = inja::json::object();

		data["functions"] = inja::json::array();
		for (auto &[generator,f] : s.getFunctions())
		{

			inja::json function_data;
			function_data["identifier"] = f.identifier;
			function_data["return_type"] = convert_to_local_type(&ps, f.return_type);
			function_data["static"] = f.static_function;
			function_data["parameters"] = inja::json::array();
			for (auto &p : f.parameters)
			{
				inja::json parameter_data;
				parameter_data["type"] = convert_to_local_type(&ps, p.first);
				parameter_data["identifier"] = p.second;
				if (!p.first.is_defaulted())
				{
					parameter_data["defaultArg"] = false;
				}
				else
				{
					parameter_data["defaultArg"] = get_default_of_type(&ps, p.first);
				}
				function_data["parameters"].push_back(parameter_data);
			}
			function_data["can_generate_function"] = f.generate_function != nullptr;

			// add the function to the data
			if (f.generate_function)
			{
				std::stringstream ss;
				f.generate_function(this, &ps, s, f, ss);
				function_data["generate_function"] = ss.str();
			}
			if (f.generator.empty())
			{
				data["functions"].push_back(function_data);
			}
			else
			{
				if (data["generators"].find(f.generator) == data["generators"].end())
				{
					data["generators"][f.generator] = inja::json::object();
					data["generators"][f.generator]["functions"] = inja::json::array();
				}

				data["generators"][f.generator]["functions"].push_back(function_data);
			}
		}

		data["includes"] = inja::json::array();
		for (auto &include : s.getIncludes())
		{
			if (include.first.empty())
			{
				data["includes"].push_back(include.second);
			}
			else
			{
				if (data["generators"].find(include.first) == data["generators"].end())
				{
					data["generators"][include.first] = inja::json::object();
					data["generators"][include.first]["includes"] = inja::json::array();
				}
				data["generators"][include.first]["includes"].push_back(include.second);
			}
		}

		data["base_classes"] = inja::json::array();
        for (auto &bc : base_classes)
        {
            if (bc.getIdentifier().empty())
            {
                continue;
            }
            inja::json base_class_data;
            base_class_data["identifier"] = bc.getIdentifier();
            base_class_data["formatted_include"] = format_include("Has" + bc.getIdentifier() + "Schema.hpp");
            
            // Add functions from base class
            base_class_data["functions"] = inja::json::array();
            for (auto & [generator, f] : bc.getFunctions())
            {
                inja::json function_data;
                function_data["identifier"] = f.identifier;
                function_data["return_type"] = convert_to_local_type(&ps, f.return_type);
                function_data["static"] = f.static_function;
                function_data["parameters"] = inja::json::array();
                for (auto &p : f.parameters)
                {
                    inja::json parameter_data;
                    parameter_data["type"] = convert_to_local_type(&ps, p.first);
                    parameter_data["identifier"] = p.second;
					if (!p.first.is_defaulted())
					{
						parameter_data["defaultArg"] = false;
					}
					else
					{
						parameter_data["defaultArg"] = get_default_of_type(&ps, p.first);
					}
                    function_data["parameters"].push_back(parameter_data);
                }
                function_data["can_generate_function"] = f.generate_function != nullptr;
				if (f.generate_function)
				{
					std::stringstream ss;
					f.generate_function(this, &ps, s, f, ss);
					function_data["generate_function"] = ss.str();
				}
                function_data["is_override"] = true;
                
                base_class_data["functions"].push_back(function_data);
            }
            
            // Add includes from base class
            base_class_data["includes"] = inja::json::array();
            for (auto &[generator,include] : bc.getIncludes())
            {
                base_class_data["includes"].push_back(include);
            }
            
            data["base_classes"].push_back(base_class_data);
        }

		// Add schema includes for member variable types
		data["schema_includes"] = inja::json::array();
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			// Include the header file for the member variable type if it is a struct or enum
			if (ps.tokenIsStruct(mv.type.identifier()) || ps.tokenIsEnum(mv.type.identifier()))
			{
				data["schema_includes"].push_back(format_include(mv.type.identifier() + "Schema.hpp"));
			}
			if (mv.type.identifier() == ARRAY)
			{
				if (ps.tokenIsStruct(mv.type.element_type().identifier()) || ps.tokenIsEnum(mv.type.element_type().identifier()))
				{
					data["schema_includes"].push_back(format_include(mv.type.element_type().identifier() + "Schema.hpp"));
				}
			}
		}

		data["member_variables"] = inja::json::array();
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			inja::json mv_data;
			mv_data["identifier"] = mv.identifier;
			mv_data["type"] = convert_to_local_type(&ps, mv.type);
			mv_data["static"] = mv.static_member;
			mv_data["required"] = mv.required;
			if (mv.default_value.empty())
			{
				mv_data["default_value"] = false;
			}
			else
			{
				mv_data["default_value"] = mv.default_value;
			}
			data["member_variables"].push_back(mv_data);
		}
		data["private_variables"] = inja::json::array();
		for (auto &[generator,pv] : s.getPrivateVariables())
		{
			inja::json pv_data;
			pv_data["identifier"] = pv.identifier;
			pv_data["type"] = convert_to_local_type(&ps, pv.type);
			pv_data["static"] = pv.static_member;
			pv_data["const"] = pv.const_member;
			data["private_variables"].push_back(pv_data);
		}

		//nested template data
		data["before_setter_lines"] = inja::json::array();
		for (auto &[generator,line] : s.getBeforeSetterLines())
		{
			inja::json line_data;
			line_data["line"] = env.render(line, data);
			data["before_setter_lines"].push_back(line_data);
		}
		data["before_getter_lines"] = inja::json::array();
		for (auto &[genrator,line] : s.getBeforeSetterLines())
		{
			inja::json line_data;
			line_data["line"] = env.render(line, data);
			data["before_getter_lines"].push_back(line_data);
		}

		try
		{
			for (auto &file : struct_name_content_pairs)
			{
				std::ofstream of(env.render(out_path + "/" + file.first, data));
				if (!of.is_open())
				{
					std::cout << "Failed to open file: " << env.render(out_path + "/" + file.first, data);
				}
				of << env.render(file.second, data);
				of.close();
				std::cout << "Generated file: " << env.render(out_path + "/" + file.first, data) << std::endl;
			}
		}
		catch (const std::exception &e)
		{
			std::cout << "Error generating file for struct " << s.getIdentifier() << ": " << e.what() << std::endl;
			return false;
		}
	}

	for (auto &e : ps.getEnums())
	{
		inja::json data;
		data["enum"] = e.identifier;
		data["enum_include"] = format_include(e.identifier + "Schema.hpp");

		data["values"] = inja::json::array();
		for (auto &v : e.values)
		{
			inja::json value_data;
			value_data["identifier"] = v.first;
			value_data["value"] = v.second;
			data["values"].push_back(value_data);
		}

		try
		{
			for (auto &file : enum_name_content_pairs)
			{
				std::ofstream of(env.render(out_path + "/" + file.first, data));
				if (!of.is_open())
				{
					std::cout << "Failed to open file: " << env.render(out_path + "/" + file.first, data);
				}
				of << env.render(file.second, data);
				of.close();
				std::cout << "Generated file: " << env.render(out_path + "/" + file.first, data) << std::endl;
			}
		}
		catch (const std::exception &ex)
		{
			std::cout << "Error generating file for enum " << e.identifier << ": " << ex.what() << std::endl;
			return false;
		}
	}

	return true;
}
