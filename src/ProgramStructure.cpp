#include <ProgramStructure.hpp>

bool ProgramStructure::isInt(std::string str)
{
	std::regex int_regex("^[0-9]+$");
	return std::regex_match(str, int_regex);
}

bool ProgramStructure::isBreakChar(std::string str)
{
	return str == " " || str == "\n" || str == "\t";
}

bool ProgramStructure::isBreakChar(char c)
{
	return c == ' ' || c == '\n' || c == '\t';
}

bool ProgramStructure::isSpecialBreakChar(std::string c)
{
	return c == "{" || c == "}" || c == "(" || c == ")" || c == "," || c == ";" || c == ":" || c == "<" || c == ">" || c == "=";
}

bool ProgramStructure::isSpecialBreakChar(char c)
{
	return c == '{' || c == '}' || c == '(' || c == ')' || c == ',' || c == ';' || c == ':' || c == '<' || c == '>' || c == '=' || c == '.';
}

std::vector<std::string> ProgramStructure::tokenize(std::string str)
{
	std::vector<std::string> tokens;
	std::string token = "";
	bool in_string = false;
	bool skip_next = false;
	for (char c : str)
	{
		if (c == '\\')
		{
			skip_next = true;
			continue;
		}
		if (skip_next)
		{
			skip_next = false;
			continue;
		}

		if (c == '"')
		{
			in_string = !in_string;
			if (!in_string)
			{
				tokens.push_back(token);
				token.clear();
			}
			continue;
		}
		if (!in_string)
		{
			if ((isBreakChar(c) || isSpecialBreakChar(c)))
			{
				if (!token.empty())
				{
					tokens.push_back(token);
					token.clear();
				}
				if (isSpecialBreakChar(c))
				{
					tokens.push_back(std::string(1, c));
				}
				continue;
			}
		}
		token += c;
	}
	if (!token.empty())
	{
		tokens.push_back(token);
	}
	// remove empty tokens
	for (auto it = tokens.begin(); it != tokens.end();)
	{
		if (it->empty())
		{
			it = tokens.erase(it);
		}
		else
		{
			++it;
		}
	}
	return tokens;
}

bool ProgramStructure::readMemberVariable(std::vector<std::string> tokens, int &i, MemberVariableDefinition &current_MemberVariableDefinition)
{
	std::vector<std::string> member_variable_tokens;
	if (tokenIsValidTypeName(tokens[i]))
	{
		// collect the type
		current_MemberVariableDefinition.type.identifier() = tokens[i];
		i++;
		// if array
		if (current_MemberVariableDefinition.type.is_array())
		{
			// check for '<'
			if (tokens[i] == "<")
			{
				i++;
				if (tokenIsValidTypeName(tokens[i]))
				{
					current_MemberVariableDefinition.type.element_type().identifier() = tokens[i];
					current_MemberVariableDefinition.generate_initializer = [](ProgramStructure *ps, MemberVariableDefinition &mv, std::ofstream &structFile) -> bool
					{
						structFile << "{}";
						return true;
					};
					current_MemberVariableDefinition.in_class_init = true;
				}
				else
				{
					printf("Error: Expected array element type for %s\n", tokens[i + 3].c_str());
					return false;
				}
				i++;
				// check for '>'
				if (tokens[i] != ">")
				{
					printf("Error: Expected '>' after array type %s for %s\n", current_MemberVariableDefinition.type.element_type().identifier().c_str(), current_MemberVariableDefinition.identifier.c_str());
					return false;
				}
				i++;
			}
		}

		// check for ':'
		if (tokens[i] != ":")
		{
			printf("Error: Expected ':' after member variable type %s\n", current_MemberVariableDefinition.type.identifier().c_str());
			return false;
		}
		i++;
		// collect the identifier
		current_MemberVariableDefinition.identifier = tokens[i];
		i++;
		// check for ':'
		if (tokens[i] != ":")
		{
			printf("Error: Expected ':' after member variable identifier %s\n", current_MemberVariableDefinition.identifier.c_str());
			return false;
		}
		i++;
		// collect all tokens for member variable up to ';'
		bool next_token_should_be_colon = false;
		while (tokens[i] != ";")
		{
			if (tokens[i] != ":")
			{
				member_variable_tokens.push_back(tokens[i]);
				next_token_should_be_colon = true;
			}
			else
			{
				if (!next_token_should_be_colon)
				{
					printf("Error: Unexpected ':' after %s\n", current_MemberVariableDefinition.identifier.c_str());
					return false;
				}
				else
				{
					next_token_should_be_colon = false;
				}
			}
			i++;
		}
		i++;
		// parse member variable tokens
		for (int j = 0; j < member_variable_tokens.size(); j++)
		{
			if (member_variable_tokens[j] == "required")
			{
				current_MemberVariableDefinition.required = true;
			}
			else if (member_variable_tokens[j] == "optional")
			{
				current_MemberVariableDefinition.required = false;
			}
			else if (member_variable_tokens[j] == "unique")
			{
				current_MemberVariableDefinition.unique = true;
			}
			else if (member_variable_tokens[j] == "auto_increment")
			{
				current_MemberVariableDefinition.auto_increment = true;
			}
			else if (member_variable_tokens[j] == "primary_key")
			{
				current_MemberVariableDefinition.primary_key = true;
			}
			else if (member_variable_tokens[j] == "min_items")
			{
				j++;
				if (member_variable_tokens[j] != "(")
				{
					printf("Error: Expected '(' after min_items\n");
					return false;
				}
				j++;
				if (!isInt(member_variable_tokens[j]))
				{
					printf("Error: Expected number after min_items(\n");
					return false;
				}
				current_MemberVariableDefinition.min_items = std::stoi(member_variable_tokens[j]);
				j++;
				if (member_variable_tokens[j] != ")")
				{
					printf("Error: Expected ')' after min_items number\n");
					return false;
				}
			}
			else if (member_variable_tokens[j] == "min_items")
			{
				j++;
				if (member_variable_tokens[j] != "(")
				{
					printf("Error: Expected '(' after min_items\n");
					return false;
				}
				j++;
				if (!isInt(member_variable_tokens[j]))
				{
					printf("Error: Expected number after min_items(\n");
					return false;
				}
				current_MemberVariableDefinition.min_items = std::stoi(member_variable_tokens[j]);
				j++;
				if (member_variable_tokens[j] != ")")
				{
					printf("Error: Expected ')' after min_items number\n");
					return false;
				}
			}
			else if (member_variable_tokens[j] == "max_items")
			{
				j++;
				if (member_variable_tokens[j] != "(")
				{
					printf("Error: Expected '(' after max_items\n");
					return false;
				}
				j++;
				if (!isInt(member_variable_tokens[j]))
				{
					printf("Error: Expected number after max_items(\n");
					return false;
				}
				current_MemberVariableDefinition.max_items = std::stoi(member_variable_tokens[j]);
				j++;
				if (member_variable_tokens[j] != ")")
				{
					printf("Error: Expected ')' after max_items number\n");
					return false;
				}
			}
			else if (member_variable_tokens[j] == "reference")
			{
				j++;
				if (tokenIsStruct(current_MemberVariableDefinition.type.identifier()))
				{
					current_MemberVariableDefinition.reference.struct_name = current_MemberVariableDefinition.type.identifier();
				}
				else
				{
					if (member_variable_tokens[j] != "(")
					{
						printf("Error: Expected '(' after reference\n");
						return false;
					}
					j++;
					current_MemberVariableDefinition.reference.struct_name = member_variable_tokens[j];
					j++;
					if (member_variable_tokens[j] != ".")
					{
						printf("Error: Expected '.' after reference struct name\n");
						return false;
					}
					j++;
					current_MemberVariableDefinition.reference.variable_name = member_variable_tokens[j];
					j++;
					if (member_variable_tokens[j] != ")")
					{
						printf("Error: Expected ')' after reference member variable name\n");
						return false;
					}
				}
			}
			else if (member_variable_tokens[j] == "description")
			{
				j++;
				if (member_variable_tokens[j] != "(")
				{
					printf("Error: Expected '(' after description\n");
					return false;
				}
				j++;
				current_MemberVariableDefinition.description = member_variable_tokens[j];
				j++;
				if (member_variable_tokens[j] != ")")
				{
					printf("Error: Expected ')' after description\n");
					return false;
				}
			}
			else
			{
				printf("Error: Unexpected token %s after %s\n", member_variable_tokens[j].c_str(), current_MemberVariableDefinition.identifier.c_str());
				return false;
			}
		}
	}
	else
	{
		printf("Error: Expected member variable type After {");
		return false;
	}
	return true;
}

bool ProgramStructure::readStruct(std::vector<std::string> tokens, int &i, StructDefinition &current_struct)
{
	if (tokens[i] != "struct")
	{
		printf("Error: Expected 'struct' keyword\n");
		return false;
	}
	i++;
	current_struct.identifier = tokens[i];
	i++;
	if (tokens[i] != "{")
	{
		printf("Error: Expected '{' after struct identifier\n");
		return false;
	}
	i++;
	while (tokens[i] != "}")
	{
		if (tokenIsValidTypeName(tokens[i]))
		{
			MemberVariableDefinition current_MemberVariableDefinition;
			if (!readMemberVariable(tokens, i, current_MemberVariableDefinition))
			{
				return false;
			}
			current_struct.add_MemberVariableDefinition(current_MemberVariableDefinition);
		}
		else
		{
			printf("Error: Expected member variable type %s is not a valid type for struct %s member variable %s\n", tokens[i].c_str(), current_struct.identifier.c_str(), tokens[i + 2].c_str());
			return false;
		}
	}

	// check for an 'id' member variable
	int id_index = -1;
	for (auto &mv : current_struct.member_variables)
	{
		if (mv.identifier == "id")
		{
			id_index = &mv - &current_struct.member_variables[0];
			break;
		}
	}
	if (id_index != -1)
	{
		printf("Error: Struct %s can not have an 'id' member variable this is reserved for the primary key.\n", current_struct.identifier.c_str());
		return false;
	}

	// insert an 'id' member variable
	MemberVariableDefinition id_member;
	id_member.type = TypeDefinition("int64");
	id_member.identifier = "id";
	id_member.primary_key = true;
	id_member.auto_increment = true;
	id_member.required = true;
	id_member.unique = true;
	id_member.description = "Primary unique identifier for " + current_struct.identifier;
	current_struct.add_MemberVariableDefinition(id_member);

	// remove identifier from type_names
	auto it = std::find(type_names.begin(), type_names.end(), current_struct.identifier);
	if (it != type_names.end())
	{
		type_names.erase(it);
	}
	return true;
}

bool ProgramStructure::readEnumValue(std::vector<std::string> tokens, int &i, EnumDefinition &current_enum, int &curent_index)
{
	std::string identifier = tokens[i];
	i++;
	if (tokens[i] == "=")
	{
		i++;
		// validate that the next token is a number
		if (!isInt(tokens[i]))
		{
			printf("Error: Expected number after '='\n");
			return false;
		}
		curent_index = std::stoi(tokens[i]);
		i++;
		if (tokens[i] == ",")
		{
			i++;
		}
		else
		{
			printf("Error: Expected ',' after enum value for identifier %s\n", identifier.c_str());
			return false;
		}
	}
	else if (tokens[i] == ",")
	{
		i++;
	}
	else if (tokens[i] == "}")
	{
		current_enum.add_value(identifier, curent_index);
		curent_index++;
		return true;
	}
	else
	{
		printf("Error: Expected ',' or '=' after enum value identifier %s\n", identifier.c_str());
		return false;
	}

	current_enum.add_value(identifier, curent_index);
	curent_index++;
	return true;
}

bool ProgramStructure::readEnum(std::vector<std::string> tokens, int &i, EnumDefinition &current_enum)
{
	if (tokens[i] != "enum")
	{
		printf("Error: Expected 'enum' keyword\n");
		return false;
	}
	i++;
	current_enum.identifier = tokens[i];
	i++;
	if (tokens[i] != "{")
	{
		printf("Error: Expected '{' after enum identifier\n");
		return false;
	}
	i++;
	int curent_index = 0;
	while (tokens[i] != "}")
	{
		if (!readEnumValue(tokens, i, current_enum, curent_index))
		{
			return false;
		}
	}
	return true;
}

bool ProgramStructure::readConfig(std::vector<std::string> tokens, int &i)
{
	// This function is a placeholder for future configuration parsing
	// Currently, it does nothing and just returns true
	if (tokens[i] != "config")
	{
		printf("Error: Expected 'config' keyword\n");
		return false;
	}
	i++;
	if (tokens[i] != "{")
	{
		printf("Error: Expected '{' after config keyword\n");
		return false;
	}
	i++;
	while (tokens[i] != "}")
	{
		i++;
	}
	i++;
	return true;
}

bool ProgramStructure::validate()
{
	for (auto &s : structs)
	{
		for (auto &mv : s.member_variables)
		{
			if(mv.type.identifier()==s.identifier)
			{
				printf("Error: Member variable %s in struct %s can not have the same type as the struct itself.\n"
					"This is a recursive dependency and will cause issues with certain generators.\n"
					"Please use the 'reference' modifier to resolve this issue.\n"
					"Example:\n"
					"\tstruct %s{\n"
					"\t\t%s: %s: reference;\n"
					"\t}\n",
					mv.identifier.c_str(),
					s.identifier.c_str(),
					s.identifier.c_str(),
					mv.type.identifier().c_str(),
					mv.identifier.c_str());
				return false;
			}

			if (tokenIsStruct(mv.type.identifier()))
			{
				StructDefinition &struct_def = getStruct(mv.type.identifier());
				for (auto &other_mv : struct_def.member_variables)
				{
					if (other_mv.type.identifier() == s.identifier)
					{
						bool mv_has_ref = !(mv.reference.struct_name.empty() && mv.reference.variable_name.empty());
						bool other_mv_has_ref = !(other_mv.reference.struct_name.empty() && other_mv.reference.variable_name.empty());
						if (mv_has_ref && other_mv_has_ref)
						{
							printf("Error: Circular dependancy detected. use the 'reference' modifyer to resolve. Resolution examples:\n"
								   "\tstruct %s{\n"
								   "\t\t%s: %s: reference;\n"
								   "\t}\n"
								   "\n"
								   "\tstruct %s{\n"
								   "\t\t%s: %s;\n"
								   "\t}\n"
								   "\n"
								   "or\n"
								   "\n"
								   "\tstruct %s{\n"
								   "\t\t%s: %s: reference;\n"
								   "\t}\n"
								   "\n"
								   "\tstruct %s{\n"
								   "\t\t%s: %s: reference;\n"
								   "\t}\n"
								   "\n",
								   s.identifier.c_str(),
								   mv.type.identifier().c_str(),
								   mv.identifier.c_str(),

								   struct_def.identifier.c_str(),
								   other_mv.type.identifier().c_str(),
								   other_mv.identifier.c_str(),

								   s.identifier.c_str(),
								   mv.type.identifier().c_str(),
								   mv.identifier.c_str(),

								   struct_def.identifier.c_str(),
								   other_mv.type.identifier().c_str(),
								   other_mv.identifier.c_str());
							return false;
						}
					}
				}
			}
			if (mv.type.is_array() && mv.type.element_type().identifier().empty())
			{
				printf("Error: Expected array element type for %s\n", mv.identifier.c_str());
				return false;
			}
			if (!mv.reference.struct_name.empty())
			{
				if (!tokenIsStruct(mv.reference.struct_name))
				{
					printf("Error: Expected struct name for reference of %s\n", mv.identifier.c_str());
					return false;
				}
				if (mv.reference.variable_name.empty())
				{
					printf("Error: Expected member variable name for reference of %s\n", mv.identifier.c_str());
					return false;
				}
				// if struct does not have member variable with name of reference variable name
				if (!getStruct(mv.reference.struct_name).has_MemberVariableDefinition(mv.reference.variable_name))
				{
					printf("Error: Struct %s does not have member variable %s\n", mv.reference.struct_name.c_str(), mv.reference.variable_name.c_str());
					return false;
				}
			}
		}
	}
	return true;
}

bool ProgramStructure::tokenIsType(std::string token)
{
	if (token == "int8" || token == "int16" || token == "int32" || token == "int64" || token == "uint8" || token == "uint16" || token == "uint32" || token == "uint64" || token == "float" || token == "double" || token == "bool" || token == "string" || token == "char" || token == "array")
	{
		return true;
	}
	return false;
}

bool ProgramStructure::tokenIsStruct(std::string token)
{
	for (auto &s : structs)
	{
		if (s.identifier == token)
		{
			return true;
		}
	}
	return false;
}

bool ProgramStructure::tokenIsEnum(std::string token)
{
	for (auto &e : enums)
	{
		if (e.identifier == token)
		{
			return true;
		}
	}
	return false;
}

bool ProgramStructure::tokenIsValidTypeName(std::string token)
{
	if (tokenIsType(token) || tokenIsStruct(token) || tokenIsEnum(token) || std::find(type_names.begin(), type_names.end(), token) != type_names.end())
	{
		return true;
	}
	return false;
}

StructDefinition &ProgramStructure::getStruct(std::string identifier)
{
	for (auto &s : structs)
	{
		if (s.identifier == identifier)
		{
			return s;
		}
	}
}

EnumDefinition &ProgramStructure::getEnum(std::string identifier)
{
	for (auto &e : enums)
	{
		if (e.identifier == identifier)
		{
			return e;
		}
	}
}

bool ProgramStructure::parseTypeNames(std::vector<std::string> tokens)
{
	for (int i = 0; i < tokens.size(); i++)
	{
		if (tokens[i] == "struct" || tokens[i] == "enum")
		{
			i++;
			if (i < tokens.size())
			{
				type_names.push_back(tokens[i]);
			}
			else
			{
				printf("Error: Expected type name after 'struct' or 'enum'\n");
				return false;
			}
			continue;
		}
	}
	return true;
}

bool ProgramStructure::readFile(std::string file_path)
{
	std::fstream file;
	file.open(file_path, std::ios::in);
	if (!file.is_open())
	{
		printf("Error: Failed to open file %s\n", file_path.c_str());
		return false;
	}
	std::string whole_file;
	file.seekg(0, std::ios::end);
	whole_file.reserve(file.tellg());
	file.seekg(0, std::ios::beg);
	whole_file.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	std::vector<std::string> tokens = tokenize(whole_file);
	if (!parseTypeNames(tokens))
	{
		printf("Error: Failed to parse type and enum names from file %s\n", file_path.c_str());
		return false;
	}

	StructDefinition current_struct;
	EnumDefinition current_enum;
	MemberVariableDefinition current_MemberVariableDefinition;
	for (int i = 0; i < tokens.size(); i++)
	{
		std::string token = tokens[i];
		if (token == "include")
		{
			i++;
			std::string include_file = tokens[i];
			//check if this is absolute path or relative path
			if (include_file[0] == '/')
			{
				readFile(include_file);
			}else
			{
				//get the path of the current file
				std::string current_file_path = std::filesystem::path(file_path).parent_path().string();
				//if the include_file starts with './'
				if (include_file.substr(0, 2) == "./")
				{
					include_file = include_file.substr(2);
				}
				current_file_path += "/" + include_file;
				if (!readFile(current_file_path))
				{
					printf("Error: Failed to read included file %s\n", current_file_path.c_str());
					return false;
				}
			}
		}
				

		if (token == "struct")
		{
			if (readStruct(tokens, i, current_struct))
			{
				structs.push_back(current_struct);
				current_struct.clear();
			}
			else
			{
				return false;
			}
		}
		if (token == "enum")
		{
			if (readEnum(tokens, i, current_enum))
			{
				current_enum.add_value("Unknown", -1);
				enums.push_back(current_enum);
				current_enum.clear();
			}
			else
			{
				return false;
			}
		}
		if (token == "config")
		{
			if (!readConfig(tokens, i))
			{
				return false;
			}
		}
	}
	return validate();
}

bool ProgramStructure::generate_files(Generator *gen, std::string out_path)
{
	return gen->generate_files(*this, out_path);
}

std::vector<StructDefinition> &ProgramStructure::getStructs()
{
	return structs;
}

std::vector<EnumDefinition> &ProgramStructure::getEnums()
{
	return enums;
}