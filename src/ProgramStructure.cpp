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

void ProgramStructure::reportError(const std::string& message)
{
	reportError(message, current_position);
}

void ProgramStructure::reportError(const std::string& message, const SourcePosition& position)
{
	printf("Error: %s:%d:%d: %s\n", position.file_path.c_str(), position.line, position.column, message.c_str());
}

void ProgramStructure::reportError(const std::string& message, const Token& token)
{
	reportError(message, token.position);
}

std::vector<Token> ProgramStructure::tokenizeWithPosition(std::string str, const std::string& file_path)
{
	std::vector<Token> tokens;
	std::string token_value = "";
	SourcePosition position(file_path, 1, 1);
	bool in_string = false;
	bool skip_next = false;

	for (char c : str)
	{
		if (c == '\\')
		{
			skip_next = true;
			position.column++;
			continue;
		}
		if (skip_next)
		{
			skip_next = false;
			position.column++;
			continue;
		}

		if (c == '"')
		{
			in_string = !in_string;
			if (!in_string)
			{
				tokens.emplace_back(token_value, position);
				token_value.clear();
			}
			position.column++;
			continue;
		}
		if (!in_string)
		{
			if ((isBreakChar(c) || isSpecialBreakChar(c)))
			{
				if (!token_value.empty())
				{
					tokens.emplace_back(token_value, position);
					token_value.clear();
				}
				if (isSpecialBreakChar(c))
				{
					tokens.emplace_back(std::string(1, c), position);
				}
				if (c == '\n')
				{
					position.line++;
					position.column = 1;
				}
				else
				{
					position.column++;
				}
				continue;
			}
		}
		token_value += c;
		position.column++;
	}
	if (!token_value.empty())
	{
		tokens.emplace_back(token_value, position);
	}
	// remove empty tokens
	for (auto it = tokens.begin(); it != tokens.end();)
	{
		if (it->value.empty())
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

bool ProgramStructure::readMemberVariable(std::vector<Token> tokens, int &i, MemberVariableDefinition &current_MemberVariableDefinition)
{
	std::vector<Token> member_variable_tokens;
	if (tokenIsValidTypeName(tokens[i].value))
	{
		// collect the type
		current_MemberVariableDefinition.type.identifier() = tokens[i].value;
		i++;
		// if array
		if (current_MemberVariableDefinition.type.is_array())
		{
			// check for '<'
			if (tokens[i] == "<")
			{
				i++;
				if (tokenIsValidTypeName(tokens[i].value))
				{
					current_MemberVariableDefinition.type.element_type().identifier() = tokens[i].value;
					current_MemberVariableDefinition.generate_initializer = [](ProgramStructure *ps, MemberVariableDefinition &mv, std::ofstream &structFile) -> bool
					{
						structFile << "{}";
						return true;
					};
					current_MemberVariableDefinition.in_class_init = true;
				}
				else
				{
					reportError("Expected array element type for " + tokens[i + 3].value, tokens[i]);
					return false;
				}
				i++;
				// check for '>'
				if (tokens[i] != ">")
				{
					reportError("Expected '>' after array type " + current_MemberVariableDefinition.type.element_type().identifier() + " for " + current_MemberVariableDefinition.identifier, tokens[i]);
					return false;
				}
				i++;
			}
		}

		// check for ':'
		if (tokens[i] != ":")
		{
			reportError("Expected ':' after member variable type " + current_MemberVariableDefinition.type.identifier(), tokens[i]);
			return false;
		}
		i++;
		// collect the identifier
		current_MemberVariableDefinition.identifier = tokens[i].value;
		i++;
		// check for ':'
		if (tokens[i] != ":")
		{
			reportError("Expected ':' after member variable identifier " + current_MemberVariableDefinition.identifier, tokens[i]);
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
					reportError("Unexpected ':' after " + current_MemberVariableDefinition.identifier, tokens[i]);
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
					reportError("Expected '(' after min_items", member_variable_tokens[j]);
					return false;
				}
				j++;
				if (!isInt(member_variable_tokens[j].value))
				{
					reportError("Expected number after min_items(", member_variable_tokens[j]);
					return false;
				}
				current_MemberVariableDefinition.min_items = std::stoi(member_variable_tokens[j].value);
				j++;
				if (member_variable_tokens[j] != ")")
				{
					reportError("Expected ')' after min_items number", member_variable_tokens[j]);
					return false;
				}
			}
			else if (member_variable_tokens[j] == "min_items")
			{
				j++;
				if (member_variable_tokens[j] != "(")
				{
					reportError("Expected '(' after min_items", member_variable_tokens[j]);
					return false;
				}
				j++;
				if (!isInt(member_variable_tokens[j].value))
				{
					reportError("Expected number after min_items(", member_variable_tokens[j]);
					return false;
				}
				current_MemberVariableDefinition.min_items = std::stoi(member_variable_tokens[j].value);
				j++;
				if (member_variable_tokens[j] != ")")
				{
					reportError("Expected ')' after min_items number", member_variable_tokens[j]);
					return false;
				}
			}
			else if (member_variable_tokens[j] == "max_items")
			{
				j++;
				if (member_variable_tokens[j] != "(")
				{
					reportError("Expected '(' after max_items", member_variable_tokens[j]);
					return false;
				}
				j++;
				if (!isInt(member_variable_tokens[j].value))
				{
					reportError("Expected number after max_items(", member_variable_tokens[j]);
					return false;
				}
				current_MemberVariableDefinition.max_items = std::stoi(member_variable_tokens[j].value);
				j++;
				if (member_variable_tokens[j] != ")")
				{
					reportError("Expected ')' after max_items number", member_variable_tokens[j]);
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
						reportError("Expected '(' after reference", member_variable_tokens[j]);
						return false;
					}
					j++;
					current_MemberVariableDefinition.reference.struct_name = member_variable_tokens[j].value;
					j++;
					if (member_variable_tokens[j] != ".")
					{
						reportError("Expected '.' after reference struct name", member_variable_tokens[j]);
						return false;
					}
					j++;
					current_MemberVariableDefinition.reference.variable_name = member_variable_tokens[j].value;
					j++;
					if (member_variable_tokens[j] != ")")
					{
						reportError("Expected ')' after reference member variable name", member_variable_tokens[j]);
						return false;
					}
				}
			}
			else if (member_variable_tokens[j] == "description")
			{
				j++;
				if (member_variable_tokens[j] != "(")
				{
					reportError("Expected '(' after description", member_variable_tokens[j]);
					return false;
				}
				j++;
				current_MemberVariableDefinition.description = member_variable_tokens[j].value;
				j++;
				if (member_variable_tokens[j] != ")")
				{
					reportError("Expected ')' after description", member_variable_tokens[j]);
					return false;
				}
			}
			else
			{
				reportError("Unexpected token " + member_variable_tokens[j].value + " after " + current_MemberVariableDefinition.identifier, member_variable_tokens[j]);
				return false;
			}
		}
	}
	else
	{
		reportError("Expected member variable type After {", tokens[i]);
		return false;
	}
	return true;
}

bool ProgramStructure::readStruct(std::vector<Token> tokens, int &i, StructDefinition &current_struct)
{
	if (tokens[i] != "struct")
	{
		reportError("Expected 'struct' keyword", tokens[i]);
		return false;
	}
	i++;
	current_struct.setIdentifier(tokens[i].value);
	i++;
	if (tokens[i] != "{")
	{
		reportError("Expected '{' after struct identifier", tokens[i]);
		return false;
	}
	i++;
	while (tokens[i] != "}")
	{
		if (tokenIsValidTypeName(tokens[i].value))
		{
			MemberVariableDefinition current_MemberVariableDefinition;
			if (!readMemberVariable(tokens, i, current_MemberVariableDefinition))
			{
				return false;
			}
			current_struct.add_member_variable(current_MemberVariableDefinition);
		}
		else
		{
			reportError("Expected member variable type " + tokens[i].value + " is not a valid type for struct " + current_struct.getIdentifier() + " member variable " + tokens[i + 2].value, tokens[i]);
			return false;
		}
	}

	// check for an 'id' member variable
	bool has_id = false;
	for (auto &[generator,mv] : current_struct.getMemberVariables())
	{
		if (mv.identifier == "id")
		{
			has_id = true;
			break;
		}
	}
	if (has_id)
	{
		reportError("Struct " + current_struct.getIdentifier() + " can not have an 'id' member variable this is reserved for the primary key.");
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
	id_member.description = "Primary unique identifier for " + current_struct.getIdentifier();
	current_struct.add_member_variable(id_member);

	// remove identifier from type_names
	auto it = std::find(type_names.begin(), type_names.end(), current_struct.getIdentifier());
	if (it != type_names.end())
	{
		type_names.erase(it);
	}
	return true;
}

bool ProgramStructure::readEnumValue(std::vector<Token> tokens, int &i, EnumDefinition &current_enum, int &curent_index)
{
	std::string identifier = tokens[i].value;
	i++;
	if (tokens[i] == "=")
	{
		i++;
		// validate that the next token is a number
		if (!isInt(tokens[i].value))
		{
			reportError("Expected number after '='", tokens[i]);
			return false;
		}
		curent_index = std::stoi(tokens[i].value);
		i++;
		if (tokens[i] == ",")
		{
			i++;
		}
		else
		{
			reportError("Expected ',' after enum value for identifier " + identifier, tokens[i]);
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
		reportError("Expected ',' or '=' after enum value identifier " + identifier, tokens[i]);
		return false;
	}

	current_enum.add_value(identifier, curent_index);
	curent_index++;
	return true;
}

bool ProgramStructure::readEnum(std::vector<Token> tokens, int &i, EnumDefinition &current_enum)
{
	if (tokens[i] != "enum")
	{
		reportError("Expected 'enum' keyword", tokens[i]);
		return false;
	}
	i++;
	current_enum.identifier = tokens[i].value;
	i++;
	if (tokens[i] != "{")
	{
		reportError("Expected '{' after enum identifier", tokens[i]);
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

bool ProgramStructure::readConfig(std::vector<Token> tokens, int &i)
{
	// This function is a placeholder for future configuration parsing
	// Currently, it does nothing and just returns true
	if (tokens[i] != "config")
	{
		reportError("Expected 'config' keyword", tokens[i]);
		return false;
	}
	i++;
	if (tokens[i] != "{")
	{
		reportError("Expected '{' after config keyword", tokens[i]);
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
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			if(mv.type.identifier()==s.getIdentifier())
			{
				reportError("Member variable " + mv.identifier + " in struct " + s.getIdentifier() + " can not have the same type as the struct itself.\n"
					"This is a recursive dependency and will cause issues with certain generators.\n"
					"Please use the 'reference' modifier to resolve this issue.\n"
					"Example:\n"
					"\tstruct " + s.getIdentifier() + "{\n"
					"\t\t" + mv.type.identifier() + ": " + mv.identifier + ": reference;\n"
					"\t}");
				return false;
			}

			if (tokenIsStruct(mv.type.identifier()))
			{
				StructDefinition &struct_def = getStruct(mv.type.identifier());
				for (auto &[generator,other_mv] : struct_def.getMemberVariables())
				{
					if (other_mv.type.identifier() == s.getIdentifier())
					{
						bool mv_has_ref = !(mv.reference.struct_name.empty() && mv.reference.variable_name.empty());
						bool other_mv_has_ref = !(other_mv.reference.struct_name.empty() && other_mv.reference.variable_name.empty());
						if (mv_has_ref && other_mv_has_ref)
						{
							reportError("Circular dependancy detected. use the 'reference' modifyer to resolve. Resolution examples:\n"
								   "\tstruct " + s.getIdentifier() + "{\n"
								   "\t\t" + mv.type.identifier() + ": " + mv.identifier + ": reference;\n"
								   "\t}\n"
								   "\n"
								   "\tstruct " + struct_def.getIdentifier() + "{\n"
								   "\t\t" + other_mv.type.identifier() + ": " + other_mv.identifier + ";\n"
								   "\t}\n"
								   "\n"
								   "or\n"
								   "\n"
								   "\tstruct " + s.getIdentifier() + "{\n"
								   "\t\t" + mv.type.identifier() + ": " + mv.identifier + ": reference;\n"
								   "\t}\n"
								   "\n"
								   "\tstruct " + struct_def.getIdentifier() + "{\n"
								   "\t\t" + other_mv.type.identifier() + ": " + other_mv.identifier + ": reference;\n"
								   "\t}");
							return false;
						}
					}
				}
			}
			if (mv.type.is_array() && mv.type.element_type().identifier().empty())
			{
				reportError("Expected array element type for " + mv.identifier);
				return false;
			}
			if (!mv.reference.struct_name.empty())
			{
				if (!tokenIsStruct(mv.reference.struct_name))
				{
					reportError("Expected struct name for reference of " + mv.identifier);
					return false;
				}
				if (mv.reference.variable_name.empty())
				{
					reportError("Expected member variable name for reference of " + mv.identifier);
					return false;
				}
				// if struct does not have member variable with name of reference variable name
				if (!getStruct(mv.reference.struct_name).has_member_variable(mv.reference.variable_name))
				{
					reportError("Struct " + mv.reference.struct_name + " does not have member variable " + mv.reference.variable_name);
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
		if (s.getIdentifier() == token)
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
		if (s.getIdentifier() == identifier)
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

bool ProgramStructure::parseTypeNames(std::vector<Token> tokens)
{
	for (int i = 0; i < tokens.size(); i++)
	{
		if (tokens[i] == "struct" || tokens[i] == "enum")
		{
			i++;
			if (i < tokens.size())
			{
				type_names.push_back(tokens[i].value);
			}
			else
			{
				reportError("Expected type name after 'struct' or 'enum'", tokens[i-1]);
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
		current_position.file_path = file_path;
		reportError("Failed to open file " + file_path);
		return false;
	}
	
	// Set current file context
	current_file = file_path;
	current_position = SourcePosition(file_path, 1, 1);
	
	std::string whole_file;
	file.seekg(0, std::ios::end);
	whole_file.reserve(file.tellg());
	file.seekg(0, std::ios::beg);
	whole_file.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	
	std::vector<Token> tokens = tokenizeWithPosition(whole_file, file_path);
	if (!parseTypeNames(tokens))
	{
		reportError("Failed to parse type and enum names from file " + file_path);
		return false;
	}

	StructDefinition current_struct;
	EnumDefinition current_enum;
	MemberVariableDefinition current_MemberVariableDefinition;
	for (int i = 0; i < tokens.size(); i++)
	{
		// Update current parsing position
		current_position = tokens[i].position;
		
		std::string token = tokens[i].value;
		if (token == "include")
		{
			i++;
			std::string include_file = tokens[i].value;
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
					reportError("Failed to read included file " + current_file_path, tokens[i]);
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