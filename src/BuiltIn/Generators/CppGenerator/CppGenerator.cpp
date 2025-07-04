#include <BuiltIn/Generators/CppGenerator/CppGenerator.hpp>

bool CppGenerator::generate_base_class_header_file(Generator *gen, ProgramStructure *ps, std::string out_path)
{
	std::ofstream baseClassFile(out_path + "/Has" + gen->base_class.identifier + "Schema.hpp");
	if (!baseClassFile.is_open())
	{
		std::cout << "Failed to open file: " << out_path + "/Has" + gen->base_class.identifier + "Schema.hpp" << std::endl;
		return false;
	}
	baseClassFile << "#pragma once\n";
	for (auto &include : gen->base_class.includes)
	{
		baseClassFile << "#include " << include << "\n";
	}
	for (auto &line : gen->base_class.before_lines)
	{
		baseClassFile << line << "\n";
	}
	baseClassFile << "class Has" + gen->base_class.identifier + "Schema{\n";
	baseClassFile << "public:\n";
	for (auto &f : gen->base_class.functions)
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

void CppGenerator::generate_enum_file(EnumDefinition e, std::string out_path)
{
	std::ofstream enumFile(out_path + "/" + e.identifier + "Schema.hpp");
	if (!enumFile.is_open())
	{
		std::cout << "Failed to open file: " << out_path + "/" + e.identifier + ".hpp" << std::endl;
		return;
	}
	enumFile << "#pragma once\n";
	enumFile << "#include <string>\n";
	enumFile << "enum " << e.identifier << "Schema" << "{\n";
	for (auto &v : e.values)
	{
		enumFile << "\t" << e.identifier << "_" << v.first << " = " << v.second << ",\n";
	}
	enumFile << "};\n";

	// static enum to string function
	enumFile << "static std::string " << e.identifier << "SchemaToString(" << e.identifier << "Schema e){\n";
	enumFile << "\tswitch(e){\n";
	for (auto &v : e.values)
	{
		enumFile << "\t\tcase " << e.identifier << "Schema::" << e.identifier << "_" << v.first << ":\n";
		enumFile << "\t\t\treturn \"" << v.first << "\";\n";
	}
	enumFile << "\t\tdefault:\n";
	enumFile << "\t\t\treturn \"Unknown\";\n";
	enumFile << "\t}\n";
	enumFile << "}\n";

	// static string to enum function
	enumFile << "static " << e.identifier << "Schema " << e.identifier << "SchemaFromString(std::string str){\n";
	for (auto &v : e.values)
	{
		if (v == e.values[e.values.size() - 1])
		{
			continue;
		}
		enumFile << "\tif(str == \"" << v.first << "\"){\n";
		enumFile << "\t\treturn " << e.identifier << "Schema::" << e.identifier << "_" << v.first << ";\n";
		enumFile << "\t}\n";
	}
	enumFile << "\treturn " << e.identifier << "Schema::" << e.identifier << "_Unknown;\n";
	enumFile << "}\n";

	enumFile.close();
}

bool CppGenerator::generate_member_variable_declaration(ProgramStructure *ps, MemberVariableDefinition &mv, std::ofstream &structFile)
{
	structFile << "\t";
	if (mv.static_member)
	{
		structFile << "static ";
	}
	structFile << convert_to_local_type(ps, mv.type) << " " << mv.identifier << ";\n";
	return true;
}

bool CppGenerator::generate_member_variable_getter_declaration(ProgramStructure *ps, MemberVariableDefinition &mv, std::ofstream &structFile)
{
	structFile << "\t" << convert_to_local_type(ps, mv.type) << " get" << mv.identifier << "();\n";
	return true;
}

bool CppGenerator::generate_member_variable_setter_declaration(ProgramStructure *ps, MemberVariableDefinition &mv, std::ofstream &structFile)
{
	structFile << "\t" << convert_to_local_type(ps, mv.type) << " set" << mv.identifier << "(" << convert_to_local_type(ps, mv.type) << " " << mv.identifier << ");\n";
	return true;
}

bool CppGenerator::generate_member_variable_getter_definition(ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv, std::ofstream &structFile)
{
	structFile << convert_to_local_type(ps, mv.type) << " " << s.identifier << "Schema::get" << mv.identifier << "(){\n";
	structFile << "\treturn " << mv.identifier << ";\n";
	structFile << "}\n";
	return true;
}

bool CppGenerator::generate_member_variable_setter_definition(ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv, std::ofstream &structFile)
{
	structFile << convert_to_local_type(ps, mv.type) << " " << s.identifier << "Schema::set" << mv.identifier << "(" << convert_to_local_type(ps, mv.type) << " " << mv.identifier << "){\n";
	structFile << "\t" << mv.identifier << " = " << mv.identifier << ";\n";
	structFile << "\treturn " << mv.identifier << ";\n";
	structFile << "}\n";
	return true;
}

bool CppGenerator::generate_struct_file_header(std::vector<StructDefinition> base_classes, ProgramStructure *ps, StructDefinition &s, std::string &out_path)
{
	std::ofstream structFile(out_path + "/" + s.identifier + "Schema.hpp");
	if (!structFile.is_open())
	{
		std::cout << "Failed to open file: " << out_path + "/" + s.identifier + ".hpp" << std::endl;
		return false;
	}
	structFile << "#pragma once\n";

	for (auto &bc : base_classes)
	{
		if (bc.identifier.empty())
		{
			continue;
		}
		structFile << "#include \"Has" << bc.identifier << "Schema.hpp\"\n";
	}

	for (auto &include : s.includes)
	{
		structFile << "#include " << include << "\n";
	}

	for (auto &line : s.before_lines)
	{
		structFile << line << "\n";
	}

	for (int i = 0; i < s.member_variables.size(); i++)
	{
		// include the header file for the member variable type if it is a struct or enum
		if (ps->tokenIsStruct(s.member_variables[i].type.identifier()) || ps->tokenIsEnum(s.member_variables[i].type.identifier()))
		{
			structFile << "#include \"" << s.member_variables[i].type.identifier() << "Schema.hpp\"\n";
		}
		if (s.member_variables[i].type.identifier() == ARRAY)
		{
			if (ps->tokenIsStruct(s.member_variables[i].type.element_type().identifier()) || ps->tokenIsEnum(s.member_variables[i].type.element_type().identifier()))
			{
				structFile << "#include \"" << s.member_variables[i].type.element_type().identifier() << "Schema.hpp\"\n";
			}
		}
	}

	structFile << "class " << s.identifier << "Schema : ";
	for (auto &bc : base_classes)
	{
		if (bc.identifier.empty())
		{
			continue;
		}
		structFile << "public Has" << bc.identifier << "Schema";
		if (&bc != &base_classes.back())
		{
			structFile << ", ";
		}
	}
	structFile << "{\n";
	structFile << "public:\n";
	bool hasinit = false;
	for (int i = 0; i < s.private_variables.size(); i++)
	{
		if (s.private_variables[i].in_class_init == false && s.private_variables[i].generate_initializer)
		{
			hasinit = true;
		}
	}
	if (hasinit)
	{
		structFile << s.identifier << "Schema::" << s.identifier << "Schema();\n";
	}

	for (int i = 0; i < s.member_variables.size(); i++)
	{
		if (!generate_member_variable_getter_declaration(ps, s.member_variables[i], structFile))
		{
			std::cout << "Failed to generate getter for member variable: " << s.member_variables[i].identifier << std::endl;
			return false;
		}
		if (!generate_member_variable_setter_declaration(ps, s.member_variables[i], structFile))
		{
			std::cout << "Failed to generate setter for member variable: " << s.member_variables[i].identifier << std::endl;
			return false;
		}
	}

	for (auto &bc : base_classes)
	{
		if (bc.identifier.empty())
		{
			continue;
		}
		for (auto &f : bc.functions)
		{
			structFile << "\t";
			if (f.static_function)
			{
				structFile << "static ";
			}
			structFile << convert_to_local_type(ps, f.return_type) << " " << f.identifier << "(";
			for (int i = 0; i < f.parameters.size(); i++)
			{
				structFile << convert_to_local_type(ps, f.parameters[i].first) << " " << f.parameters[i].second;
				if (i < f.parameters.size() - 1)
				{
					structFile << ", ";
				}
			}
			structFile << ") override;\n";
		}
	}

	for (auto &f : s.functions)
	{
		structFile << "\t";
		if (f.static_function)
		{
			structFile << "static ";
		}
		structFile << convert_to_local_type(ps, f.return_type) << " " << f.identifier << "(";
		for (int i = 0; i < f.parameters.size(); i++)
		{
			structFile << convert_to_local_type(ps, f.parameters[i].first) << " " << f.parameters[i].second;
			if (i < f.parameters.size() - 1)
			{
				structFile << ", ";
			}
		}
		structFile << ");\n";
	}

	structFile << "private:\n";
	for (int i = 0; i < s.private_variables.size(); i++)
	{
		structFile << "\t";
		if (s.private_variables[i].static_member)
		{
			structFile << "static ";
		}
		if (s.private_variables[i].const_member)
		{
			structFile << "const ";
		}
		structFile << convert_to_local_type(ps, s.private_variables[i].type) << " " << s.private_variables[i].identifier;
		if (s.private_variables[i].in_class_init && s.private_variables[i].generate_initializer)
		{
			structFile << " = ";
			s.private_variables[i].generate_initializer(ps, s.private_variables[i], structFile);
		}
		structFile << ";\n";
	}

	for (int i = 0; i < s.member_variables.size(); i++)
	{
		if (!generate_member_variable_declaration(ps, s.member_variables[i], structFile))
		{
			std::cout << "Failed to generate declaration for member variable: " << s.member_variables[i].identifier << std::endl;
			return false;
		}
	}
	structFile << "};\n";
	structFile.close();
	return true;
}

void CppGenerator::generate_struct_file_source(std::vector<StructDefinition> base_classes, ProgramStructure *ps, StructDefinition &s, std::string &out_path)
{
	std::ofstream structFile(out_path + "/" + s.identifier + "Schema.cpp");
	if (!structFile.is_open())
	{
		std::cout << "Failed to open file: " << out_path + "/" + s.identifier + ".cpp" << std::endl;
		return;
	}
	structFile << "#include \"" << s.identifier << "Schema.hpp\"\n";

	bool hasinit = false;
	for (int i = 0; i < s.private_variables.size(); i++)
	{
		if ((!s.private_variables[i].in_class_init) && s.private_variables[i].generate_initializer)
		{
			hasinit = true;
		}
	}
	if (hasinit)
	{
		structFile << s.identifier << "Schema::" << s.identifier << "Schema(){\n";
		for (int i = 0; i < s.private_variables.size(); i++)
		{
			if (!(s.private_variables[i].in_class_init == true))
			{
				structFile << "\t" << s.private_variables[i].identifier << " = ";
				s.private_variables[i].generate_initializer(ps, s.private_variables[i], structFile);
				structFile << ";\n";
			}
		}
		structFile << "}\n";
	}

	for (int i = 0; i < s.member_variables.size(); i++)
	{
		if (!generate_member_variable_getter_definition(ps, s, s.member_variables[i], structFile))
		{
			std::cout << "Failed to generate getter for member variable: " << s.member_variables[i].identifier << std::endl;
			return;
		}
		if (!generate_member_variable_setter_definition(ps, s, s.member_variables[i], structFile))
		{
			std::cout << "Failed to generate setter for member variable: " << s.member_variables[i].identifier << std::endl;
			return;
		}
	}
	for (auto &bc : base_classes)
	{
		for (auto &f : bc.functions)
		{
			structFile << convert_to_local_type(ps, f.return_type) << " " << s.identifier << "Schema::" << f.identifier << "(";
			for (int i = 0; i < f.parameters.size(); i++)
			{
				structFile << convert_to_local_type(ps, f.parameters[i].first) << " " << f.parameters[i].second;
				if (i < f.parameters.size() - 1)
				{
					structFile << ", ";
				}
			}
			structFile << "){\n";
			if (f.generate_function)
			{
				f.generate_function(this, ps, s, f, structFile);
			}
			else
			{
				printf("Error: No function Generator for %s\n", f.identifier.c_str());
				return;
			}
			structFile << "}\n";
		}
	}

	for (auto &f : s.functions)
	{
		if (f.return_type.identifier() == ARRAY)
		{
			continue;
		}

		structFile << convert_to_local_type(ps, f.return_type) << " " << s.identifier << "Schema::" << f.identifier << "(";
		for (int i = 0; i < f.parameters.size(); i++)
		{
			structFile << convert_to_local_type(ps, f.parameters[i].first) << " " << f.parameters[i].second;
			if (i < f.parameters.size() - 1)
			{
				structFile << ", ";
			}
		}
		structFile << "){\n";
		if (f.generate_function)
		{
			f.generate_function(this, ps, s, f, structFile);
		}
		else
		{
			printf("Error: No function Generator for %s\n", f.identifier.c_str());
			return;
		}
		structFile << "}\n";
	}
	structFile.close();
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

bool CppGenerator::add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	return true;
}

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
		if (!gen->base_class.identifier.empty())
		{
			if (!generate_base_class_header_file(gen, &ps, out_path))
			{
				printf("Error: Failed to generate base class header file for %s\n", gen->base_class.identifier.c_str());
				return false;
			}
			base_classes.push_back(gen->base_class);
		}

		for (auto &s : ps.getStructs())
		{
			if (!gen->add_generator_specific_content_to_struct(this, &ps, s))
			{
				printf("Error: Failed to add Generator specific functions for %s\n", gen->base_class.identifier.c_str());
				return false;
			}
		}
	}

	for (auto &gen : generators)
	{
		if (gen == this)
		{
			continue;
		}
		for (auto &s : ps.getStructs())
		{
			generate_struct_file_header(base_classes, &ps, s, out_path);
			generate_struct_file_source(base_classes, &ps, s, out_path);
		}
		for (auto &e : ps.getEnums())
		{
			generate_enum_file(e, out_path);
		}
	}
	return true;
}
