#include <BuiltInGenerators/JsonGenerator.hpp>

bool JsonGenerator::isNumberType(std::string type)
{
	if (type == INT8 || type == INT16 || type == INT32 || type == INT64 ||
		type == UINT8 || type == UINT16 || type == UINT32 || type == UINT64 ||
		type == FLOAT || type == DOUBLE)
	{
		return true;
	}
	return false;
}

json JsonGenerator::enumToSchema(EnumDefinition e)
{
	json j;
	j["title"] = e.identifier;
	j["type"] = "string";
	json enum_values;
	for (auto &v : e.values)
	{
		enum_values.push_back(v.first);
	}
	j["enum"] = enum_values;
	return j;
}

json JsonGenerator::structToSchema(StructDefinition s, ProgramStructure *ps)
{
	json j;
	j["title"] = s.getIdentifier();
	j["type"] = "object";
	json properties;

	try
	{
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			json property;

			if (mv.type.is_struct(ps))
			{
				property = structToSchema(ps->getStruct(mv.type.identifier()), ps);
			}
			else if (mv.type.is_enum(ps))
			{
				property = enumToSchema(ps->getEnum(mv.type.identifier()));
			}
			else if (mv.type.is_array())
			{
				json array_property;
				array_property["type"] = "array";
				if (ps->tokenIsStruct(mv.type.element_type().identifier()))
				{
					array_property["items"] = structToSchema(ps->getStruct(mv.type.element_type().identifier()), ps);
				}
				else if (ps->tokenIsEnum(mv.type.element_type().identifier()))
				{
					array_property["items"] = enumToSchema(ps->getEnum(mv.type.element_type().identifier()));
				}
				else
				{
					if (mv.type.element_type().is_number())
					{
						array_property["items"]["type"] = "number";
					}
					if (mv.type.element_type().is_bool())
					{
						array_property["items"]["type"] = "boolean";
					}
					else
					{
						array_property["items"]["type"] = mv.type.element_type().identifier();
					}
				}
				array_property["minItems"] = mv.min_items;
				array_property["uniqueItems"] = mv.unique;
				property = array_property;
			}
			else
			{
				if (mv.type.is_number())
				{
					property["type"] = "number";
				}
				else if (mv.type.is_bool())
				{
					property["type"] = "boolean";
				}
				else
				{
					property["type"] = mv.type.identifier();
				}
			}
			property["description"] = mv.description;

			properties[mv.identifier] = property;
		}
		j["properties"] = properties;

		j["required"] = json::array();
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			if (mv.required)
			{
				j["required"].push_back(mv.identifier);
			}
		}
	}
	catch (const std::exception &e)
	{
		printf("Error generating schema for struct %s: %s\n", s.getIdentifier().c_str(), e.what());
		exit(1);
	}
	return j;
}

JsonGenerator::JsonGenerator()
{
	name = "Json";
	base_class.setIdentifier("Json");
	FunctionDefinition toJSON;
	toJSON.generator = name;
	toJSON.identifier = "toJSON";
	toJSON.return_type.identifier() = "nlohmann::json";
	toJSON.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\tnlohmann::json j;\n";
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			if (mv.type.is_struct(ps))
			{
				structFile << "\tj[\"" << mv.identifier << "\"] = " << mv.identifier << "->toJSON();\n";
			}
			else if (mv.type.is_enum(ps))
			{
				structFile << "\tj[\"" << mv.identifier << "\"] = " << mv.identifier << ";\n";
			}
			else if (mv.type.is_array())
			{
				structFile << "\tnlohmann::json " << mv.identifier << "Array;\n";
				structFile << "\tfor(auto &v : " << mv.identifier << "){\n";
				if (ps->tokenIsStruct(mv.type.element_type().identifier()))
				{
					structFile << "\t\t" << mv.identifier << "Array.push_back(v->toJSON());\n";
				}
				else if (ps->tokenIsEnum(mv.type.element_type().identifier()))
				{
					structFile << "\t\t" << mv.identifier << "Array.push_back(v);\n";
				}
				else
				{
					structFile << "\t\t" << mv.identifier << "Array.push_back(v);\n";
				}
				structFile << "\t}\n";
				structFile << "\tj[\"" << mv.identifier << "\"] = " << mv.identifier << "Array;\n";
			}
			else
			{
				structFile << "\tj[\"" << mv.identifier << "\"] = " << mv.identifier << ";\n";
			}
		}
		structFile << "\treturn j;\n";
		return true;
	};
	FunctionDefinition fromJSON;
	fromJSON.generator = name;
	fromJSON.identifier = "fromJSON";
	fromJSON.return_type.identifier() = "void";
	fromJSON.parameters.push_back(std::make_pair(TypeDefinition("nlohmann::json"), "j"));
	fromJSON.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			if (mv.type.is_struct(ps))
			{
				structFile << "if(j.find(\"" << mv.identifier << "\") != j.end()){\n";
				structFile << mv.identifier << " = new " << mv.type.identifier() << "Schema();\n";
				structFile << "\t" << mv.identifier << "->fromJSON(j[\"" << mv.identifier << "\"]);\n";
				structFile << "}\n";
			}
			else if (mv.type.is_enum(ps))
			{
				structFile << "\tif(j.find(\"" << mv.identifier << "\") != j.end()){\n";
				structFile << "\t\t" << mv.identifier << " = " + mv.type.identifier() + "SchemaFromString(j[\"" << mv.identifier << "\"]);\n";
				structFile << "\t}\n";
				structFile << "\telse{\n";
				structFile << "\t\t" << mv.identifier << " = " + mv.type.identifier() + "Schema::" + mv.type.identifier() + "_Unknown;\n";
				structFile << "\t}\n";
			}
			else if (mv.type.is_array())
			{
				structFile << "\tif(j.find(\"" << mv.identifier << "\") != j.end()){\n";
				structFile << "\t\t" << mv.identifier << ".clear();\n";
				structFile << "\t\tfor(auto &v : j[\"" << mv.identifier << "\"]){\n";
				if (ps->tokenIsStruct(mv.type.element_type().identifier()))
				{
					structFile << "\t\t\t" << mv.type.element_type().identifier() << "Schema *new_" << mv.type.element_type().identifier() << " = new " << mv.type.element_type().identifier() << "Schema();\n";
					structFile << "\t\t\tnew_" << mv.type.element_type().identifier() << "->fromJSON(v);\n";
					structFile << "\t\t\t" << mv.identifier << ".push_back(new_" << mv.type.element_type().identifier() << ");\n";
				}
				else if (ps->tokenIsEnum(mv.type.element_type().identifier()))
				{
					structFile << "\t\t\t" << mv.type.element_type().identifier() << " new_" << mv.type.element_type().identifier() << " = v;\n";
					structFile << "\t\t\t" << mv.identifier << ".push_back(new_" << mv.type.element_type().identifier() << ");\n";
				}
				else
				{
					structFile << "\t\t\t" << mv.type.element_type().identifier() << " new_" << mv.type.element_type().identifier() << " = v;\n";
					structFile << "\t\t\t" << mv.identifier << ".push_back(new_" << mv.type.element_type().identifier() << ");\n";
				}
				structFile << "\t\t}\n";
				structFile << "\t}\n";
			}
			else
			{
				structFile << "\tif(j.find(\"" << mv.identifier << "\") != j.end()){\n";
				structFile << "\t\t" << mv.identifier << " = j[\"" << mv.identifier << "\"];\n";
				structFile << "\t}\n";
			}
		}
		return true;
	};
	FunctionDefinition getSchema;
	getSchema.generator = name;
	getSchema.identifier = "getSchema";
	getSchema.return_type.identifier() = "nlohmann::json";
	getSchema.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\treturn nlohmann::json::parse(\n";
		json j = structToSchema(s, ps);
		// escape quotes using regex
		std::regex quote_regex("\"");
		std::string escaped = std::regex_replace(j.dump(), quote_regex, "\\\"");
		structFile << "\"" << escaped << "\")" << ";\n";
		return true;
	};
	base_class.add_function(toJSON);
	base_class.add_function(fromJSON);
	base_class.add_function(getSchema);

	base_class.add_include("<nlohmann/json.hpp>",name);
	base_class.add_include("<string>",name);
}

std::string JsonGenerator::convert_to_local_type(ProgramStructure *ps, TypeDefinition type)
{
	// convert number types to "number"
	if (type.is_number())
	{
		return "number";
	}
	// convert bool to "boolean"
	if (type.is_bool())
	{
		return "boolean";
	}
	// convert string to "string"
	if (type.is_string())
	{
		return "string";
	}
	// convert char to "string"
	if (type.is_char())
	{
		return "string";
	}
	// convert array to "array"
	if (type.is_array())
	{
		return "array";
	}
	return type.identifier();
}

bool JsonGenerator::add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	return true;
}

bool JsonGenerator::generate_files(ProgramStructure ps, std::string out_path)
{
	if (!std::filesystem::exists(out_path))
	{
		std::filesystem::create_directories(out_path);
	}

	for (auto &s : ps.getStructs())
	{
		json j = structToSchema(s, &ps);

		std::ofstream schemaFile(out_path + "/" + s.getIdentifier() + ".schema.json");
		if (!schemaFile.is_open())
		{
			std::cout << "Failed to open file: " << out_path + "/" + s.getIdentifier() + ".schema.json" << std::endl;
			return false;
		}
		schemaFile << j.dump(4);
		schemaFile.close();
	}
	return true;
}
