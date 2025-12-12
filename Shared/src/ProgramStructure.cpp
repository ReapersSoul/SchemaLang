#include <ProgramStructure.hpp>

// SchemaLang version info
#include <SchemaLangVersion.hpp>

// Include debug_server for hooks
#include <Networking/DebugServer.hpp>

#include <queue>
#include <filesystem>
#include <fstream>
#include <map>
#include <algorithm>

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

void ProgramStructure::reportError(const std::string &message)
{
	reportError(message, current_position);
}

void ProgramStructure::reportError(const std::string &message, const SourcePosition &position)
{
	printf("Error: %s:%d:%d: %s\n", position.file_path.c_str(), position.line, position.column, message.c_str());
}

void ProgramStructure::reportError(const std::string &message, const Token &token)
{
	// Try to provide surrounding token context and source line with caret.
	// Read the source file and attempt to find nearby tokens.
	std::string file_path = token.position.file_path;
	std::ifstream file(file_path);
	if (!file.is_open())
	{
		// fallback to existing behavior
		reportError(message, token.position);
		return;
	}

	// Read whole file into a string
	std::string whole_file((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	// Tokenize with position to reconstruct tokens and their positions
	std::vector<Token> all_tokens = tokenizeWithPosition(whole_file, file_path);

	// find index of the token (match by position.line and column and value)
	int idx = -1;
	for (int i = 0; i < (int)all_tokens.size(); ++i)
	{
		if (all_tokens[i].position.line == token.position.line && all_tokens[i].position.column == token.position.column && all_tokens[i].value == token.value)
		{
			idx = i;
			break;
		}
	}

	// Print the basic error header
	reportError(message, token.position);

	if (idx == -1)
	{
		// Couldn't find matching token; nothing more to show
		return;
	}

	// Print a few tokens around the error token
	int context_radius = 3;
	int start = std::max(0, idx - context_radius);
	int end = std::min((int)all_tokens.size() - 1, idx + context_radius);
	printf("Context tokens:\n");
	for (int i = start; i <= end; ++i)
	{
		if (i == idx)
		{
			printf(" -> [%s]\n", all_tokens[i].value.c_str());
		}
		else
		{
			printf("    %s\n", all_tokens[i].value.c_str());
		}
	}

	// Print the source line and show a caret under the offending token's column
	// Find the line in the file
	int line_no = token.position.line;
	int cur_line = 1;
	size_t pos = 0;
	size_t line_start = 0;
	size_t line_end = 0;
	while (pos < whole_file.size())
	{
		if (cur_line == line_no)
		{
			line_start = pos;
			// find end of line
			while (pos < whole_file.size() && whole_file[pos] != '\n')
				pos++;
			line_end = pos;
			break;
		}
		if (whole_file[pos] == '\n')
		{
			cur_line++;
		}
		pos++;
	}
	if (line_end > line_start)
	{
		std::string line = whole_file.substr(line_start, line_end - line_start);
		printf("%s\n", line.c_str());
		// build caret line: columns in SourcePosition are 1-based
		int caret_col = token.position.column - 1; // zero-based
		// but the token may start later due to previous tokens; attempt to place caret at column
		std::string caret;
		for (int i = 0; i < caret_col && i < (int)line.size(); ++i)
		{
			if (line[i] == '\t')
				caret += '\t';
			else
				caret += ' ';
		}
		caret += '^';
		printf("%s\n", caret.c_str());
	}
}

std::vector<Token> ProgramStructure::tokenizeWithPosition(std::string str, const std::string &file_path)
{
	std::vector<Token> tokens;
	std::string token_value = "";
	SourcePosition position(file_path, 1, 1);
	bool in_string = false;
	bool skip_next = false;

	for (size_t idx = 0; idx < str.size();)
	{
		char c = str[idx];
		char next = (idx + 1 < str.size() ? str[idx + 1] : '\0');

		// handle comment starts (only when not inside a string)
		if (!in_string && c == '/' && next == '/')
		{
			// single-line comment: skip until newline (or EOF)
			idx += 2;
			position.column += 2;
			while (idx < str.size() && str[idx] != '\n')
			{
				idx++;
				position.column++;
			}
			if (idx < str.size() && str[idx] == '\n')
			{
				position.line++;
				position.column = 1;
				idx++;
			}
			continue;
		}
		if (!in_string && c == '/' && next == '*')
		{
			// multi-line comment: skip until '*/'
			idx += 2;
			position.column += 2;
			bool found_end = false;
			while (idx < str.size())
			{
				if (str[idx] == '\n')
				{
					position.line++;
					position.column = 1;
					idx++;
					continue;
				}
				if (str[idx] == '*' && idx + 1 < str.size() && str[idx + 1] == '/')
				{
					idx += 2;
					position.column += 2;
					found_end = true;
					break;
				}
				idx++;
				position.column++;
			}
			if (!found_end)
			{
				// Unterminated comment — report and stop tokenizing
				reportError("Unterminated block comment", position);
				return tokens;
			}
			continue;
		}

		if (c == '\\')
		{
			// preserve previous behavior: skip next char (escape)
			skip_next = true;
			position.column++;
			idx++;
			continue;
		}
		if (skip_next)
		{
			skip_next = false;
			position.column++;
			idx++;
			continue;
		}

		if (c == '"')
		{
			in_string = !in_string;
			if (!in_string)
			{
				// end of string, emit token
				tokens.emplace_back(token_value, position);
				token_value.clear();
			}
			position.column++;
			idx++;
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
				idx++;
				continue;
			}
		}
		token_value += c;
		position.column++;
		idx++;
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
	for (size_t idx = 0; idx < str.size();)
	{
		char c = str[idx];
		char next = (idx + 1 < str.size() ? str[idx + 1] : '\0');

		// single-line comment
		if (!in_string && c == '/' && next == '/')
		{
			idx += 2;
			while (idx < str.size() && str[idx] != '\n')
			{
				idx++;
			}
			if (idx < str.size() && str[idx] == '\n')
			{
				idx++;
			}
			continue;
		}
		// multi-line comment
		if (!in_string && c == '/' && next == '*')
		{
			idx += 2;
			bool found_end = false;
			while (idx < str.size())
			{
				if (str[idx] == '*' && idx + 1 < str.size() && str[idx + 1] == '/')
				{
					idx += 2;
					found_end = true;
					break;
				}
				idx++;
			}
			// if unterminated, just stop scanning remainder
			if (!found_end)
			{
				break;
			}
			continue;
		}

		if (c == '\\')
		{
			skip_next = true;
			idx++;
			continue;
		}
		if (skip_next)
		{
			skip_next = false;
			idx++;
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
			idx++;
			continue;
		}
		if (!in_string)
		{
			if (isBreakChar(c) || isSpecialBreakChar(c))
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
				idx++;
				continue;
			}
		}
		token += c;
		idx++;
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

bool ProgramStructure::parseVersion(std::vector<Token> tokens, int &i)
{
	// Expected format: version 1.0.0;
	// We're at the token after "version"

	if (debug_server)
	{
		debug_server->beginParseOperation("parsing version");
	}

	// store by filename (basename). Duplicate filenames are errors.
	std::string filename = std::filesystem::path(current_file).filename().string();
	if (transpiler_versions.find(filename) != transpiler_versions.end())
	{
		auto &existing = transpiler_versions[filename];
		reportError("SchemaLangVersion already specified for filename '" + filename + "' in " + existing.position.file_path + ":" +
						std::to_string(existing.position.line) + ":" + std::to_string(existing.position.column),
					tokens[i]);
		return false;
	}
	VersionWithPosition fv;
	fv.position = tokens[i].position;

	// Parse major version
	if (!isInt(tokens[i].value))
	{
		reportError("Expected integer for major version number", tokens[i]);
		return false;
	}
	fv.major = std::stoi(tokens[i].value);
	i++;

	if (debug_server && i < tokens.size())
		debug_server->onTokenParsed(tokens[i]);

	// Expect '.'
	if (tokens[i] != ".")
	{
		reportError("Expected '.' after major version", tokens[i]);
		return false;
	}
	i++;

	if (debug_server && i < tokens.size())
		debug_server->onTokenParsed(tokens[i]);

	// Parse minor version
	if (!isInt(tokens[i].value))
	{
		reportError("Expected integer for minor version number", tokens[i]);
		return false;
	}
	fv.minor = std::stoi(tokens[i].value);
	i++;

	if (debug_server && i < tokens.size())
		debug_server->onTokenParsed(tokens[i]);

	// Expect '.'
	if (tokens[i] != ".")
	{
		reportError("Expected '.' after minor version", tokens[i]);
		return false;
	}
	i++;

	if (debug_server && i < tokens.size())
		debug_server->onTokenParsed(tokens[i]);

	// Parse patch version
	if (!isInt(tokens[i].value))
	{
		reportError("Expected integer for patch version number", tokens[i]);
		return false;
	}
	fv.patch = std::stoi(tokens[i].value);
	i++;

	if (debug_server && i < tokens.size())
		debug_server->onTokenParsed(tokens[i]);

	// Expect ';'
	if (tokens[i] != ";")
	{
		reportError("Expected ';' after version declaration", tokens[i]);
		return false;
	}
	if (debug_server)
		debug_server->beginParseOperation("version parsed: " + std::to_string(fv.major) + "." + std::to_string(fv.minor) + "." + std::to_string(fv.patch));

	// store it
	transpiler_versions[filename] = fv;
	if (debug_server)
		debug_server->endParseOperation();
	return true;
}

bool ProgramStructure::validateTranspilerVersion()
{
	bool ok = true;
	// Validate each file's transpiler version if present
	for (auto &pair : transpiler_versions)
	{
		const std::string &filename = pair.first;
		const VersionWithPosition &fv = pair.second;
		if (fv.major != SCHEMALANG_VERSION_MAJOR)
		{
			reportError("Schema major version mismatch for file '" + filename + "'. Schema requires v" + std::to_string(fv.major) + ".x.x but transpiler is v" +
							std::to_string(SCHEMALANG_VERSION_MAJOR) + "." + std::to_string(SCHEMALANG_VERSION_MINOR) + "." + std::to_string(SCHEMALANG_VERSION_PATCH) + ".",
						fv.position);
			ok = false;
		}
		if (fv.minor > SCHEMALANG_VERSION_MINOR)
		{
			reportError("Schema requires features from v" + std::to_string(fv.major) + "." + std::to_string(fv.minor) + ".x but transpiler is v" +
							std::to_string(SCHEMALANG_VERSION_MAJOR) + "." + std::to_string(SCHEMALANG_VERSION_MINOR) + "." + std::to_string(SCHEMALANG_VERSION_PATCH) + ".",
						fv.position);
			ok = false;
		}
	}

	// Warn for each included file missing a SchemaLangVersion (never an error)
	for (const auto &file_path : already_included_files)
	{
		// Already warned for this file?
		if (transpiler_version_warnings_emitted.find(file_path) != transpiler_version_warnings_emitted.end())
			continue;

		std::string filename = std::filesystem::path(file_path).filename().string();
		if (transpiler_versions.find(filename) == transpiler_versions.end())
		{
			PLOGW << "WARNING: No SchemaLangVersion specified for file '" << file_path << "'. Please add 'SchemaLangVersion X.Y.Z;' to the schema file." << std::endl;
			transpiler_version_warnings_emitted.insert(file_path);
		}
	}
	return ok;
}

std::string ProgramStructure::getTranspilerVersionString(const std::string &filename) const
{
	std::string key = filename.empty() ? root_filename : filename;
	auto v = getTranspilerVersion(key);
	if (!v.has_value())
		return "unspecified";
	return std::to_string(v->major) + "." + std::to_string(v->minor) + "." + std::to_string(v->patch);
}

std::optional<VersionWithPosition> ProgramStructure::getTranspilerVersion(const std::string &filename) const
{
	auto it = transpiler_versions.find(filename);
	if (it == transpiler_versions.end())
		return std::nullopt;
	return it->second;
}

bool ProgramStructure::readMemberVariable(std::vector<Token> tokens, int &i, MemberVariableDefinition &current_MemberVariableDefinition)
{

	if (debug_server)
	{
		debug_server->beginParseOperation("parsing member variable");
	}

	std::vector<Token> member_variable_tokens;
	if (tokenIsValidTypeName(tokens[i].value))
	{
		// collect the type
		current_MemberVariableDefinition.type.identifier() = tokens[i].value;
		i++;

		if (debug_server && i < tokens.size())
			debug_server->onTokenParsed(tokens[i]);

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
					current_MemberVariableDefinition.generate_initializer = [](std::shared_ptr<ProgramStructure> ps, MemberVariableDefinition &mv, std::ofstream &structFile) -> bool
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

		// if (debug_server) debug_server->onMemberParsing(&current_MemberVariableDefinition);

		i++;

		if (debug_server && i < tokens.size())
			debug_server->onTokenParsed(tokens[i]);

		// check for ':'
		if (tokens[i] != ":")
		{
			reportError("Expected ':' after member variable identifier " + current_MemberVariableDefinition.identifier, tokens[i]);
			return false;
		}
		i++;

		if (debug_server && i < tokens.size())
			debug_server->onTokenParsed(tokens[i]);

		// collect all tokens for member variable up to ';'
		bool next_token_should_be_colon = false;
		while (tokens[i] != ";")
		{
			if(i>=tokens.size())
			{
				reportError("Unexpected end of tokens while parsing member variable " + current_MemberVariableDefinition.identifier);
				return false;
			}
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
				current_MemberVariableDefinition.type.setRequired(true);
			}
			else if (member_variable_tokens[j] == "optional")
			{
				current_MemberVariableDefinition.type.setRequired(false);
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
				if (tokenIsStruct(current_MemberVariableDefinition.type.identifier()) && member_variable_tokens[j + 1] != "(")
				{
					current_MemberVariableDefinition.reference.struct_name = current_MemberVariableDefinition.type.identifier();
				}
				else
				{
					j++;
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
			else if (member_variable_tokens[j] == "gens_enabled")
			{
				j++;
				if (member_variable_tokens[j] != "(")
				{
					reportError("Expected '(' after gens_enabled", member_variable_tokens[j]);
					return false;
				}
				do
				{
					j++;
					current_MemberVariableDefinition.enabled_for_generators.insert(member_variable_tokens[j]);
					j++;
				} while (member_variable_tokens[j] == ",");
				if (member_variable_tokens[j] != ")")
				{
					reportError("Expected ')' after gens_enabled", member_variable_tokens[j]);
					return false;
				}
			}
			else if (member_variable_tokens[j] == "gens_disabled")
			{
				j++;
				if (member_variable_tokens[j] != "(")
				{
					reportError("Expected '(' after gens_disabled", member_variable_tokens[j]);
					return false;
				}
				do
				{
					j++;
					current_MemberVariableDefinition.disabled_for_generators.insert(member_variable_tokens[j]);
					j++;
				} while (member_variable_tokens[j] == ",");
				if (member_variable_tokens[j] != ")")
				{
					reportError("Expected ')' after gens_disabled", member_variable_tokens[j]);
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
	if (debug_server)
		debug_server->endParseOperation();
	return true;
}

bool ProgramStructure::readStruct(std::vector<Token> tokens, int &i, StructDefinition &current_struct)
{

	if (debug_server)
	{
		debug_server->beginParseOperation("reading struct definition");
	}

	if (tokens[i] != "struct")
	{
		reportError("Expected 'struct' keyword", tokens[i]);
		return false;
	}
	i++;

	if (debug_server && i < tokens.size())
		debug_server->onTokenParsed(tokens[i]);

	current_struct.setIdentifier(tokens[i].value);

	// if (debug_server) debug_server->onStructParsing(&current_struct);

	i++;

	if (debug_server && i < tokens.size())
		debug_server->onTokenParsed(tokens[i]);

	if (tokens[i] == ":")
	{
		i++;
		while (tokens[i] != "{")
		{
			if (tokens[i] == "gens_enabled")
			{
				i++;
				if (tokens[i] != "(")
				{
					reportError("Expected '(' after gens_enabled", tokens[i]);
					return false;
				}
				do
				{
					i++;
					current_struct.add_gen_enabled(tokens[i]);
					i++;
				} while (tokens[i] == ",");
				if (tokens[i] != ")")
				{
					reportError("Expected ')' after gens_enabled", tokens[i]);
					return false;
				}
				i++;
			}
			else if (tokens[i] == "gens_disabled")
			{
				i++;
				if (tokens[i] != "(")
				{
					reportError("Expected '(' after gens_disabled", tokens[i]);
					return false;
				}
				do
				{
					i++;
					current_struct.add_gen_disabled(tokens[i]);
					i++;
				} while (tokens[i] == ",");
				if (tokens[i] != ")")
				{
					reportError("Expected ')' after gens_disabled", tokens[i]);
					return false;
				}
				i++;
			}
			else if (tokens[i] == "version"){
				i++;
				if (tokens[i] != "(")
				{
					reportError("Expected '(' after version", tokens[i]);
					return false;
				}
				i++;
				// parse version number
				if (!isInt(tokens[i].value))
				{
					reportError("Expected integer for major version number", tokens[i]);
					return false;
				}
				int major = std::stoi(tokens[i].value);
				i++;

				if (debug_server && i < tokens.size())
					debug_server->onTokenParsed(tokens[i]);

				// Expect '.'
				if (tokens[i] != ".")
				{
					reportError("Expected '.' after major version", tokens[i]);
					return false;
				}
				i++;

				if (debug_server && i < tokens.size())
					debug_server->onTokenParsed(tokens[i]);

				// Parse minor version
				if (!isInt(tokens[i].value))
				{
					reportError("Expected integer for minor version number", tokens[i]);
					return false;
				}
				int minor = std::stoi(tokens[i].value);
				i++;

				if (debug_server && i < tokens.size())
					debug_server->onTokenParsed(tokens[i]);

				// Optional patch version
				int patch = 0;
				if (tokens[i] == ".")
				{
					i++;

					if (debug_server && i < tokens.size())
						debug_server->onTokenParsed(tokens[i]);

					if (!isInt(tokens[i].value))
					{
						reportError("Expected integer for patch version number", tokens[i]);
						return false;
					}
					patch = std::stoi(tokens[i].value);
					i++;

					if (debug_server && i < tokens.size())
						debug_server->onTokenParsed(tokens[i]);
				}

				// Expect ')'
				if (tokens[i] != ")")
				{
					reportError("Expected ')' after version", tokens[i]);
					return false;
				}
				i++;
				current_struct.setVersion(Version{major, minor, patch});
			}
			else
			{
				reportError("Invalid struct modifier", tokens[i]);
				return false;
			}
			if(tokens[i] == ","){
				i++;
			}
			if(tokens[i] != "{"&& tokens[i] != ","){
				reportError("Expected '{' or ',' after struct modifier", tokens[i]);
				return false;
			}
		}
	}
	if (tokens[i] != "{")
	{
		reportError("Expected '{' after struct identifier", tokens[i]);
		return false;
	}
	i++;

	if (debug_server && i < tokens.size())
		debug_server->onTokenParsed(tokens[i]);

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
	for (auto &mv : current_struct.getMemberVariables())
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
	id_member.type.setRequired(true);
	id_member.identifier = "id";
	id_member.primary_key = true;
	id_member.auto_increment = true;
	id_member.unique = true;
	id_member.description = "Primary unique identifier for " + current_struct.getIdentifier();
	current_struct.add_member_variable(id_member);

	// remove identifier from type_names
	auto it = std::find(type_names.begin(), type_names.end(), current_struct.getIdentifier());
	if (it != type_names.end())
	{
		type_names.erase(it);
	}
	if (debug_server)
		debug_server->endParseOperation();
	return true;
}

bool ProgramStructure::readEnumValue(std::vector<Token> tokens, int &i, EnumDefinition &current_enum, int &curent_index)
{

	if (debug_server)
	{
		debug_server->beginParseOperation("parsing enum value");
	}

	std::string identifier = tokens[i].value;
	i++;

	if (debug_server && i < tokens.size())
		debug_server->onTokenParsed(tokens[i]);

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
	if (debug_server)
		debug_server->endParseOperation();
	return true;
}

bool ProgramStructure::readEnum(std::vector<Token> tokens, int &i, EnumDefinition &current_enum)
{

	if (debug_server)
	{
		debug_server->beginParseOperation("reading enum definition");
	}

	if (tokens[i] != "enum")
	{
		reportError("Expected 'enum' keyword", tokens[i]);
		return false;
	}
	i++;

	if (debug_server && i < tokens.size())
		debug_server->onTokenParsed(tokens[i]);

	current_enum.identifier = tokens[i].value;

	// if (debug_server) debug_server->onEnumParsing(&current_enum);

	i++;

	if (debug_server && i < tokens.size())
		debug_server->onTokenParsed(tokens[i]);

	if (tokens[i] == ":")
	{
		i++;
		if (tokens[i] == "gens_enabled")
		{
			i++;
			if (tokens[i] != "(")
			{
				reportError("Expected '(' after gens_enabled", tokens[i]);
				return false;
			}
			do
			{
				i++;
				current_enum.enabled_for_generators.insert(tokens[i]);
				i++;
			} while (tokens[i] == ",");
			if (tokens[i] != ")")
			{
				reportError("Expected ')' after gens_enabled", tokens[i]);
				return false;
			}
			i++;
		}
		else if (tokens[i] == "gens_disabled")
		{
			i++;
			if (tokens[i] != "(")
			{
				reportError("Expected '(' after gens_disabled", tokens[i]);
				return false;
			}
			do
			{
				i++;
				current_enum.disabled_for_generators.insert(tokens[i]);
				i++;
			} while (tokens[i] == ",");
			if (tokens[i] != ")")
			{
				reportError("Expected ')' after gens_disabled", tokens[i]);
				return false;
			}
			i++;
		}
		else
		{
			reportError("Invalid enum modifier", tokens[i]);
			return false;
		}
	}
	if (tokens[i] != "{")
	{
		if (tokens[i] == "gens_enabled" || tokens[i] == "gens_disabled")
		{
			reportError("Cant have a whitelist and blacklist on the same struct", tokens[i]);
			return false;
		}
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
	if (debug_server)
		debug_server->endParseOperation();
	return true;
}

bool ProgramStructure::readConfig(std::vector<Token> tokens, int &i)
{
	if (debug_server)
	{
		debug_server->beginParseOperation("reading config");
	}

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
	if (debug_server && i < tokens.size())
		debug_server->onTokenParsed(tokens[i]);
	while (tokens[i] != "}")
	{
		i++;
	}
	i++;
	if (debug_server)
		debug_server->endParseOperation();
	return true;
}

bool ProgramStructure::validate(bool is_root)
{

	if (debug_server)
	{
		// debug_server->onValidation();
		debug_server->beginParseOperation("validating schema");
	}

	// Validate transpiler versions (warn if missing) before deeper validation
	if (!validateTranspilerVersion())
	{
		return false;
	}

	for (auto &s : structs)
	{
		for (auto &mv : s.getMemberVariables())
		{
			// Check for direct self-reference
			if (mv.type.identifier() == s.getIdentifier())
			{
				reportError("Member variable " + mv.identifier + " in struct " + s.getIdentifier() + " can not have the same type as the struct itself.\n"
																									 "This is a recursive dependency and will cause issues with certain generators.\n"
																									 "Please use the 'reference' modifier to resolve this issue.\n"
																									 "Example:\n"
																									 "\tstruct " +
							s.getIdentifier() + "{\n"
												"\t\t" +
							mv.type.identifier() + ": " + mv.identifier + ": reference;\n"
																		  "\t}");
				return false;
			}

			// Check for self-reference in array element type
			if (mv.type.is_array() && mv.type.element_type().identifier() == s.getIdentifier())
			{
				bool has_ref = !(mv.reference.struct_name.empty() && mv.reference.variable_name.empty());
				if (!has_ref)
				{
					reportError("Member variable " + mv.identifier + " in struct " + s.getIdentifier() + " can not have an array of the same type as the struct itself without a reference.\n"
																										 "This is a recursive dependency and will cause issues with certain generators.\n"
																										 "Please use the 'reference' modifier to resolve this issue.\n"
																										 "Example:\n"
																										 "\tstruct " +
								s.getIdentifier() + "{\n"
													"\t\t" +
								mv.type.identifier() + "<" + mv.type.element_type().identifier() + ">: " + mv.identifier + ": reference(" + s.getIdentifier() + ".id);\n"
																																								"\t}");
					return false;
				}
			}

			if (tokenIsStruct(mv.type.identifier()))
			{
				StructDefinition &struct_def = getStruct(mv.type.identifier());
				for (auto &other_mv : struct_def.getMemberVariables())
				{
					if (other_mv.type.identifier() == s.getIdentifier())
					{
						bool mv_has_ref = !(mv.reference.struct_name.empty() && mv.reference.variable_name.empty());
						bool other_mv_has_ref = !(other_mv.reference.struct_name.empty() && other_mv.reference.variable_name.empty());
						if (mv_has_ref && other_mv_has_ref)
						{
							reportError("Circular dependancy detected. use the 'reference' modifyer to resolve. Resolution examples:\n"
										"\tstruct " +
										s.getIdentifier() + "{\n"
															"\t\t" +
										mv.type.identifier() + ": " + mv.identifier + ": reference;\n"
																					  "\t}\n"
																					  "\n"
																					  "\tstruct " +
										struct_def.getIdentifier() + "{\n"
																	 "\t\t" +
										other_mv.type.identifier() + ": " + other_mv.identifier + ";\n"
																								  "\t}\n"
																								  "\n"
																								  "or\n"
																								  "\n"
																								  "\tstruct " +
										s.getIdentifier() + "{\n"
															"\t\t" +
										mv.type.identifier() + ": " + mv.identifier + ": reference;\n"
																					  "\t}\n"
																					  "\n"
																					  "\tstruct " +
										struct_def.getIdentifier() + "{\n"
																	 "\t\t" +
										other_mv.type.identifier() + ": " + other_mv.identifier + ": reference;\n"
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

				// Validate that the reference type matches the member variable type
				StructDefinition &ref_struct = getStruct(mv.reference.struct_name);
				MemberVariableDefinition &ref_member = ref_struct.get_member_variable(mv.reference.variable_name);

				// Get the type to compare - for arrays, use the element type, otherwise use the type itself
				std::string mv_type = mv.type.is_array() ? mv.type.element_type().identifier() : mv.type.identifier();
				std::string ref_type = ref_member.type.identifier();

				if (mv_type != ref_type)
				{
					reportError("Reference type mismatch for '" + mv.identifier + "' in struct '" + s.getIdentifier() + "'.\n"
																														"Member variable has type '" +
								mv_type + "' but references " +
								mv.reference.struct_name + "." + mv.reference.variable_name + " which has type '" + ref_type + "'.\n"
																															   "The types must match. For arrays, the element type must match the referenced field type.");
					return false;
				}
			}
		}
	}

	if (is_root)
	{
		// Check for unimplemented forward declarations
		for (auto &s : structs)
		{
			// A struct is considered "forward-declared only" if it has exactly 1 member (the auto-generated id)
			if (s.getMemberVariables().size() == 1 && s.getMemberVariables()[0].identifier == "id")
			{
				// Check if any other struct uses this forward-declared struct
				bool is_used = false;
				for (auto &other_s : structs)
				{
					if (other_s.getIdentifier() == s.getIdentifier())
						continue;

					for (auto &mv : other_s.getMemberVariables())
					{
						// Check if member variable type matches the forward-declared struct
						if (mv.type.identifier() == s.getIdentifier())
						{
							is_used = true;
							break;
						}
						// Check array element types
						if (mv.type.is_array() && mv.type.element_type().identifier() == s.getIdentifier())
						{
							is_used = true;
							break;
						}
					}
					if (is_used)
						break;
				}

				if (is_used)
				{
					reportError("Struct '" + s.getIdentifier() + "' was forward-declared but never fully implemented.\n"
																 "Forward declarations must be followed by a full struct definition.\n"
																 "Example:\n"
																 "\tdeclare struct " +
								s.getIdentifier() + ";\n"
													"\t// ... other structs that reference " +
								s.getIdentifier() + " ...\n"
													"\tstruct " +
								s.getIdentifier() + " {\n"
													"\t\tstring: name: required: description(\"Name field\");\n"
													"\t\t// ... other fields ...\n"
													"\t}");
					return false;
				}
			}
		}

		// Check for empty forward-declared enums that are used
		for (auto &e : enums)
		{
			// An enum with only "Unknown" and "Count" was forward-declared but never implemented
			if (e.values.size() == 2)
			{
				bool hasUnknown = false;
				bool hasCount = false;
				for (const auto &pair : e.values)
				{
					if (pair.first == "Unknown")
						hasUnknown = true;
					if (pair.first == "Count")
						hasCount = true;
				}

				if (hasUnknown && hasCount)
				{
					// Check if this enum is used anywhere
					bool is_used = false;
					for (auto &s : structs)
					{
						for (auto &mv : s.getMemberVariables())
						{
							if (mv.type.identifier() == e.identifier)
							{
								is_used = true;
								break;
							}
							if (mv.type.is_array() && mv.type.element_type().identifier() == e.identifier)
							{
								is_used = true;
								break;
							}
						}
						if (is_used)
							break;
					}

					if (is_used)
					{
						reportError("Enum '" + e.identifier + "' was forward-declared but never fully implemented.\n"
															  "Forward declarations must be followed by a full enum definition.\n"
															  "Example:\n"
															  "\tdeclare enum " +
									e.identifier + ";\n"
												   "\t// ... other definitions that reference " +
									e.identifier + " ...\n"
												   "\tenum " +
									e.identifier + " {\n"
												   "\t\tValue1,\n"
												   "\t\tValue2,\n"
												   "\t\tValue3\n"
												   "\t}");
						return false;
					}
				}
			}
		}
	}
	return true;
}

inja::json ProgramStructure::to_json(std::shared_ptr<Generator> generator)
{
	inja::json j;
	j["includes"] = inja::json::array();
	j["structs"] = inja::json::array();
	j["enums"] = inja::json::array();
	j["migrations"] = inja::json::array();
	j["file_versions"] = inja::json::object();
	j["transpiler_versions"] = inja::json::object();
	for (auto &s : structs)
	{
		j["includes"].push_back(generator->format_include(s.getIdentifier() + "Schema.hpp"));
		j["structs"].push_back(s.to_json(shared_from_this(), generator));
	}
	for (auto &e : enums)
	{
		j["includes"].push_back(generator->format_include(e.identifier + "Schema.hpp"));
		j["enums"].push_back(e.to_json(shared_from_this(), generator));
	}
	for (auto &m : migrations)
	{
		j["migrations"].push_back(m.to_json());
	}

	for (auto &pair : transpiler_versions)
	{
		j["transpiler_versions"][pair.first] = std::to_string(pair.second.major) + "." + std::to_string(pair.second.minor) + "." + std::to_string(pair.second.patch);
	}
	return j;
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
	throw std::runtime_error("ProgramStructure::getStruct() - Struct '" + identifier + "' not found in program. Available structs: " + std::to_string(structs.size()) + " total. Check spelling or ensure struct is defined before use.");
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
	throw std::runtime_error("ProgramStructure::getEnum() - Enum '" + identifier + "' not found in program. Available enums: " + std::to_string(enums.size()) + " total. Check spelling or ensure enum is defined before use.");
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
				reportError("Expected type name after 'struct' or 'enum'", tokens[i - 1]);
				return false;
			}
			continue;
		}
	}
	return true;
}

bool ProgramStructure::readFile(std::string file_path, bool is_root)
{
	// Notify debug_server about file load

	if (debug_server)
	{
		// debug_server->onFileLoaded(file_path);
	}

	if (std::find(already_included_files.begin(), already_included_files.end(), file_path) != already_included_files.end())
	{
		return true;
	}
	already_included_files.push_back(file_path);

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
	// store root filename for defaults (basename)
	std::string current_filename = std::filesystem::path(file_path).filename().string();
	if (is_root)
	{
		root_filename = current_filename;
	}
	current_position = SourcePosition(file_path, 1, 1);

	std::string whole_file;
	file.seekg(0, std::ios::end);
	whole_file.reserve(file.tellg());
	file.seekg(0, std::ios::beg);
	whole_file.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	std::vector<Token> tokens = tokenizeWithPosition(whole_file, file_path);

	if (tokens.empty())
	{
		reportError("Empty schema file: " + file_path);
		return false;
	}

	if (!parseTypeNames(tokens))
	{
		reportError("Failed to parse type and enum names from file " + file_path);
		return false;
	}

	StructDefinition current_struct;
	EnumDefinition current_enum;
	MemberVariableDefinition current_MemberVariableDefinition;

	// Per-file SchemaLangVersion warnings are handled globally in validateTranspilerVersion().

	for (int i = 0; i < tokens.size(); i++)
	{
		// Update current parsing position
		current_position = tokens[i].position;

		if (debug_server)
		{
			debug_server->onTokenParsed(tokens[i]);
		}

		std::string token = tokens[i].value;

		// Handle version declaration - must come before any other declarations
		if (token == "SchemaLangVersion")
		{
			i++;
			if (!parseVersion(tokens, i))
			{
				return false;
			}
			// do not validate here to avoid repeated warnings; global validate() will be called once per file
			continue;
		}

		if (token == "include")
		{
			i++;
			std::string include_file = tokens[i].value;
			// check if this is absolute path or relative path
			if (include_file[0] == '/')
			{
				readFile(include_file, false);
			}
			else
			{
				// get the path of the current file
				std::string current_file_path = std::filesystem::path(file_path).parent_path().string();
				// if the include_file starts with './'
				if (include_file.substr(0, 2) == "./")
				{
					include_file = include_file.substr(2);
				}
				current_file_path += "/" + include_file;
				if (!readFile(current_file_path, false))
				{
					reportError("Failed to read included file " + current_file_path, tokens[i]);
					return false;
				}
			}
			continue;
		}

		if (token == "declare")
		{
			i++;
			// if struct or enum
			if (tokens[i] == "struct")
			{
				i++;
				// check if struct already exists
				if (tokenIsStruct(tokens[i].value))
				{
					i++;
					continue; // Already declared, skip
				}
				// Handle forward declaration of struct
				StructDefinition forward_decl;
				forward_decl.setIdentifier(tokens[i].value);
				MemberVariableDefinition id_member;
				id_member.type = TypeDefinition("int64");
				id_member.type.setRequired(true);
				id_member.identifier = "id";
				id_member.primary_key = true;
				id_member.auto_increment = true;
				id_member.unique = true;
				id_member.description = "Primary unique identifier for " + current_struct.getIdentifier();
				forward_decl.add_member_variable(id_member);
				structs.push_back(forward_decl);
				i++; // Skip the struct name token
			}
			else if (tokens[i] == "enum")
			{
				i++;
				// check if enum already exists
				if (tokenIsEnum(tokens[i].value))
				{
					i++;
					continue; // Already declared, skip
				}
				// Handle forward declaration of enum
				EnumDefinition forward_decl;
				forward_decl.identifier = tokens[i].value;
				enums.push_back(forward_decl);
				i++; // Skip the enum name token
			}
			else
			{
				reportError("Expected 'struct' or 'enum' after 'declare'", tokens[i]);
				return false;
			}
			if (tokens[i] != ";")
			{
				reportError("Expected ';' after forward declaration", tokens[i]);
				return false;
			}
			continue;
		}

		if (token == "struct")
		{

			if (debug_server)
			{
				debug_server->beginParseOperation("parsing struct");
			}

			if (readStruct(tokens, i, current_struct))
			{

				// if (debug_server) {
				// 	debug_server->onStructParsing(&current_struct);
				// }

				auto it = std::find_if(structs.begin(), structs.end(), [&](const StructDefinition &s)
									   { return s.getIdentifier() == current_struct.getIdentifier(); });
				if (it != structs.end())
				{
					it->update(current_struct);
				}
				else
				{
					structs.push_back(current_struct);
				}
				current_struct.clear();
			}
			else
			{
				reportError("Failed to read struct", tokens[i]);
				return false;
			}
			continue;
		}

		if (token == "enum")
		{

			if (debug_server)
			{
				debug_server->beginParseOperation("parsing enum");
			}

			if (readEnum(tokens, i, current_enum))
			{

				// if (debug_server) {
				// 	debug_server->onEnumParsing(&current_enum);
				// }

				int count = current_enum.values.size();
				current_enum.add_value("Unknown", -1);
				current_enum.add_value("Count", count);
				auto it = std::find_if(enums.begin(), enums.end(), [&](const EnumDefinition &e)
									   { return e.identifier == current_enum.identifier; });
				if (it != enums.end())
				{
					it->update(current_enum);
				}
				else
				{
					enums.push_back(current_enum);
				}
				current_enum.clear();
			}
			else
			{
				reportError("Failed to read enum", tokens[i]);
				return false;
			}
			continue;
		}

		if (token == "config")
		{
			if (!readConfig(tokens, i))
			{
				reportError("Failed to read config", tokens[i]);
				return false;
			}
			continue;
		}
	}
	return validate(is_root);
}

bool ProgramStructure::readMigration(std::vector<Token> tokens, int &i, MigrationDefinition &current_migration)
{
	if (debug_server)
	{
		debug_server->beginParseOperation("reading migration definition");
	}

	// Expected: migration struct <StructName> from <version> to <version> { operations }
	if (tokens[i] != "migration")
	{
		reportError("Expected 'migration' keyword", tokens[i]);
		return false;
	}
	i++;

	if (tokens[i] != "struct")
	{
		reportError("Expected 'struct' after 'migration'", tokens[i]);
		return false;
	}
	i++;

	// Get struct name
	current_migration.structName = tokens[i].value;
	i++;

	// Expect 'from'
	if (tokens[i] != "from")
	{
		reportError("Expected 'from' after struct name", tokens[i]);
		return false;
	}
	i++;

	// Parse from version (major.minor.patch or major.minor)
	if (!isInt(tokens[i].value))
	{
		reportError("Expected integer for major version", tokens[i]);
		return false;
	}
	current_migration.fromVersion.major = std::stoi(tokens[i].value);
	i++;

	if (tokens[i] != ".")
	{
		reportError("Expected '.' after major version", tokens[i]);
		return false;
	}
	i++;

	if (!isInt(tokens[i].value))
	{
		reportError("Expected integer for minor version", tokens[i]);
		return false;
	}
	current_migration.fromVersion.minor = std::stoi(tokens[i].value);
	i++;

	// Optional patch version
	if (tokens[i] == ".")
	{
		i++;
		if (!isInt(tokens[i].value))
		{
			reportError("Expected integer for patch version", tokens[i]);
			return false;
		}
		current_migration.fromVersion.patch = std::stoi(tokens[i].value);
		i++;
	}

	// Expect 'to'
	if (tokens[i] != "to")
	{
		reportError("Expected 'to' after from version", tokens[i]);
		return false;
	}
	i++;

	// Parse to version
	if (!isInt(tokens[i].value))
	{
		reportError("Expected integer for major version", tokens[i]);
		return false;
	}
	current_migration.toVersion.major = std::stoi(tokens[i].value);
	i++;

	if (tokens[i] != ".")
	{
		reportError("Expected '.' after major version", tokens[i]);
		return false;
	}
	i++;

	if (!isInt(tokens[i].value))
	{
		reportError("Expected integer for minor version", tokens[i]);
		return false;
	}
	current_migration.toVersion.minor = std::stoi(tokens[i].value);
	i++;

	// Optional patch version
	if (tokens[i] == ".")
	{
		i++;
		if (!isInt(tokens[i].value))
		{
			reportError("Expected integer for patch version", tokens[i]);
			return false;
		}
		current_migration.toVersion.patch = std::stoi(tokens[i].value);
		i++;
	}

	// Expect '{'
	if (tokens[i] != "{")
	{
		reportError("Expected '{' after migration declaration", tokens[i]);
		return false;
	}
	i++;

	// Parse migration operations
	while (tokens[i] != "}")
	{
		MigrationOperation op;

		// Parse operation type
		if (tokens[i] == "add")
		{
			i++;
			if (tokens[i] != "field")
			{
				reportError("Expected 'field' after 'add'", tokens[i]);
				return false;
			}
			i++;

			op.type = MigrationOperation::Type::AddField;
			op.fieldName = tokens[i].value;
			i++;

			// Parse field type and modifiers (simplified - parse until semicolon)
			while (tokens[i] != ";")
			{
				i++;
			}
			i++; // Skip semicolon
		}
		else if (tokens[i] == "remove")
		{
			i++;
			if (tokens[i] != "field")
			{
				reportError("Expected 'field' after 'remove'", tokens[i]);
				return false;
			}
			i++;

			op.type = MigrationOperation::Type::RemoveField;
			op.fieldName = tokens[i].value;
			i++;

			if (tokens[i] != ";")
			{
				reportError("Expected ';' after field name", tokens[i]);
				return false;
			}
			i++;
		}
		else if (tokens[i] == "rename")
		{
			i++;
			if (tokens[i] != "field")
			{
				reportError("Expected 'field' after 'rename'", tokens[i]);
				return false;
			}
			i++;

			op.type = MigrationOperation::Type::RenameField;
			op.fieldName = tokens[i].value;
			i++;

			if (tokens[i] != "to")
			{
				reportError("Expected 'to' after field name", tokens[i]);
				return false;
			}
			i++;

			op.newFieldName = tokens[i].value;
			i++;

			if (tokens[i] != ";")
			{
				reportError("Expected ';' after new field name", tokens[i]);
				return false;
			}
			i++;
		}
		else if (tokens[i] == "change")
		{
			i++;
			if (tokens[i] != "field")
			{
				reportError("Expected 'field' after 'change'", tokens[i]);
				return false;
			}
			i++;

			op.fieldName = tokens[i].value;
			i++;

			if (tokens[i] == "type")
			{
				i++;
				op.type = MigrationOperation::Type::ChangeType;

				if (tokens[i] != "from")
				{
					reportError("Expected 'from' after 'type'", tokens[i]);
					return false;
				}
				i++;

				op.oldType = tokens[i].value;
				i++;

				if (tokens[i] != "to")
				{
					reportError("Expected 'to' after old type", tokens[i]);
					return false;
				}
				i++;

				op.newType = tokens[i].value;
				i++;
			}
			else if (tokens[i] == "modifier")
			{
				i++;

				if (tokens[i] == "from")
				{
					op.type = MigrationOperation::Type::ChangeModifier;
					i++;
					op.oldModifierValue = tokens[i].value;
					i++;

					if (tokens[i] != "to")
					{
						reportError("Expected 'to' after old modifier", tokens[i]);
						return false;
					}
					i++;

					op.newModifierValue = tokens[i].value;
					i++;
				}
				else if (tokens[i] == "add")
				{
					op.type = MigrationOperation::Type::SetModifier;
					i++;
					op.newModifierValue = tokens[i].value;
					i++;
				}
				else if (tokens[i] == "remove")
				{
					op.type = MigrationOperation::Type::RemoveModifier;
					i++;
					op.oldModifierValue = tokens[i].value;
					i++;
				}
				else if (tokens[i] == "set")
				{
					i++;
					if (tokens[i] == "description")
					{
						op.type = MigrationOperation::Type::SetDescription;
						i++;

						if (tokens[i] != "(")
						{
							reportError("Expected '(' after description", tokens[i]);
							return false;
						}
						i++;

						op.description = tokens[i].value;
						i++;

						if (tokens[i] != ")")
						{
							reportError("Expected ')' after description", tokens[i]);
							return false;
						}
						i++;
					}
					else
					{
						reportError("Expected 'description' after 'set'", tokens[i]);
						return false;
					}
				}
				else
				{
					reportError("Expected 'from', 'add', 'remove', or 'set' after 'modifier'", tokens[i]);
					return false;
				}
			}
			else
			{
				reportError("Expected 'type' or 'modifier' after field name", tokens[i]);
				return false;
			}

			if (tokens[i] != ";")
			{
				reportError("Expected ';' after migration operation", tokens[i]);
				return false;
			}
			i++;
		}
		else
		{
			reportError("Expected migration operation (add, remove, rename, change)", tokens[i]);
			return false;
		}

		current_migration.operations.push_back(op);
	}

	if (debug_server)
		debug_server->endParseOperation();

	return true;
}

bool ProgramStructure::readMigrationFile(std::string file_path)
{
	if (debug_server)
	{
		debug_server->beginParseOperation("reading migration file");
	}

	std::fstream file;
	file.open(file_path, std::ios::in);
	if (!file.is_open())
	{
		current_position.file_path = file_path;
		reportError("Failed to open migration file " + file_path);
		return false;
	}

	current_file = file_path;
	current_position = SourcePosition(file_path, 1, 1);

	std::string whole_file;
	file.seekg(0, std::ios::end);
	whole_file.reserve(file.tellg());
	file.seekg(0, std::ios::beg);
	whole_file.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	std::vector<Token> tokens = tokenizeWithPosition(whole_file, file_path);

	if (tokens.empty())
	{
		reportError("Empty migration file: " + file_path);
		return false;
	}

	for (int i = 0; i < tokens.size(); i++)
	{
		current_position = tokens[i].position;

		if (debug_server)
		{
			debug_server->onTokenParsed(tokens[i]);
		}

		if (tokens[i] == "migration")
		{
			MigrationDefinition current_migration;
			if (!readMigration(tokens, i, current_migration))
			{
				reportError("Failed to read migration", tokens[i]);
				return false;
			}

			// Validate struct exists
			if (!tokenIsStruct(current_migration.structName))
			{
				reportError("Migration references unknown struct: " + current_migration.structName, tokens[i]);
				return false;
			}

			// Validate migration operations
			if (!validateMigration(current_migration))
			{
				reportError("Migration validation failed for " + current_migration.structName, tokens[i]);
				return false;
			}

			migrations.push_back(current_migration);
			PLOGI << "Loaded migration for " << current_migration.structName 
				  << " from " << current_migration.fromVersion.major << "." << current_migration.fromVersion.minor << "." << current_migration.fromVersion.patch
				  << " to " << current_migration.toVersion.major << "." << current_migration.toVersion.minor << "." << current_migration.toVersion.patch << std::endl;
		}
	}

	if (debug_server)
		debug_server->endParseOperation();

	return true;
}

std::vector<MigrationDefinition> ProgramStructure::resolveMigrationChain(const std::string& structName, const Version& fromVersion, const Version& toVersion)
{
	std::vector<MigrationDefinition> chain;
	
	// Build adjacency list for migration graph
	std::map<std::string, std::vector<MigrationDefinition>> migrationGraph;
	for (const auto& migration : migrations)
	{
		if (migration.structName == structName)
		{
			std::string versionKey = std::to_string(migration.fromVersion.major) + "." + 
									 std::to_string(migration.fromVersion.minor) + "." + 
									 std::to_string(migration.fromVersion.patch);
			migrationGraph[versionKey].push_back(migration);
		}
	}

	// BFS to find shortest path
	std::string startKey = std::to_string(fromVersion.major) + "." + 
						   std::to_string(fromVersion.minor) + "." + 
						   std::to_string(fromVersion.patch);
	std::string targetKey = std::to_string(toVersion.major) + "." + 
							std::to_string(toVersion.minor) + "." + 
							std::to_string(toVersion.patch);

	std::queue<std::pair<std::string, std::vector<MigrationDefinition>>> queue;
	std::set<std::string> visited;

	queue.push({startKey, {}});
	visited.insert(startKey);

	while (!queue.empty())
	{
		auto [currentKey, currentChain] = queue.front();
		queue.pop();

		if (currentKey == targetKey)
		{
			return currentChain;
		}

		if (migrationGraph.find(currentKey) != migrationGraph.end())
		{
			for (const auto& migration : migrationGraph[currentKey])
			{
				std::string nextKey = std::to_string(migration.toVersion.major) + "." + 
									  std::to_string(migration.toVersion.minor) + "." + 
									  std::to_string(migration.toVersion.patch);

				if (visited.find(nextKey) == visited.end())
				{
					visited.insert(nextKey);
					auto newChain = currentChain;
					newChain.push_back(migration);
					queue.push({nextKey, newChain});
				}
			}
		}
	}

	// No path found - return empty vector (caller will use best-effort)
	return {};
}

MigrationDefinition ProgramStructure::generateBestEffortMigration(const std::string& structName, const Version& fromVersion, const Version& toVersion)
{
	MigrationDefinition migration;
	migration.structName = structName;
	migration.fromVersion = fromVersion;
	migration.toVersion = toVersion;

	// Get the current struct definition (this is the target version)
	StructDefinition& currentStruct = getStruct(structName);
	
	PLOGW << "No migration path found from " << fromVersion.major << "." << fromVersion.minor << "." << fromVersion.patch
		  << " to " << toVersion.major << "." << toVersion.minor << "." << toVersion.patch 
		  << " for struct " << structName << ". Generating best-effort migration based on current struct definition." << std::endl;

	// For best-effort, we assume all fields in current struct are new fields that need to be added
	// This is a simple heuristic - in production, you'd want to compare against actual old version
	for (const auto& mv : currentStruct.getMemberVariables())
	{
		// Skip the auto-generated id field
		if (mv.identifier == "id") continue;

		MigrationOperation op;
		op.type = MigrationOperation::Type::AddField;
		op.fieldName = mv.identifier;
		op.newType = mv.type.identifier();
		
		migration.operations.push_back(op);
		
		PLOGW << "Best-effort migration: Adding field '" << mv.identifier << "' of type '" << mv.type.identifier() << "'" << std::endl;
	}

	return migration;
}

bool ProgramStructure::generate_files(std::shared_ptr<Generator> gen, std::string out_path)
{
	return gen->generate_files(shared_from_this(), out_path);
}

bool ProgramStructure::generate_migration_files(std::shared_ptr<Generator> gen, std::string out_path)
{
	// Default implementation - generators override if they support migrations
	return gen->generate_migration_files(shared_from_this(), out_path);
}

bool ProgramStructure::validateMigration(const MigrationDefinition& migration)
{
	// Find the target struct
	StructDefinition* targetStruct = nullptr;
	for (auto& s : structs)
	{
		if (s.getIdentifier() == migration.structName)
		{
			targetStruct = &s;
			break;
		}
	}

	if (!targetStruct)
	{
		PLOGE << "Migration validation failed: struct '" << migration.structName << "' not found" << std::endl;
		return false;
	}

	// Validate that fromVersion matches one of the struct's historical versions
	// (In a full implementation, you'd track version history)
	// For now, we just warn if fromVersion doesn't match current version

	// Validate each operation
	for (const auto& op : migration.operations)
	{
		switch (op.type)
		{
			case MigrationOperation::Type::RemoveField:
			case MigrationOperation::Type::RenameField:
			{
				// Verify field exists in current struct (or at fromVersion in full implementation)
				bool fieldExists = false;
				for (const auto& mv : targetStruct->getMemberVariables())
				{
					if (mv.identifier == op.fieldName)
					{
						fieldExists = true;
						break;
					}
				}

				if (!fieldExists)
				{
					PLOGW << "Migration warning: field '" << op.fieldName 
						  << "' not found in current struct '" << migration.structName 
						  << "' (may be valid if removing from older version)" << std::endl;
				}

				// Warn on destructive operations
				if (op.type == MigrationOperation::Type::RemoveField)
				{
					PLOGW << "Migration contains destructive operation: removing field '" 
						  << op.fieldName << "' from '" << migration.structName << "'" << std::endl;
				}
				break;
			}

			case MigrationOperation::Type::ChangeType:
			{
				// Verify field exists
				bool fieldExists = false;
				for (const auto& mv : targetStruct->getMemberVariables())
				{
					if (mv.identifier == op.fieldName)
					{
						fieldExists = true;
						break;
					}
				}

				if (!fieldExists)
				{
					PLOGW << "Migration warning: field '" << op.fieldName 
						  << "' not found in current struct '" << migration.structName << "'" << std::endl;
				}

				// Warn about potential data loss
				if (op.oldType.has_value() && op.newType.has_value())
				{
					PLOGW << "Migration changes type of field '" << op.fieldName 
						  << "' from '" << op.oldType.value() << "' to '" << op.newType.value() 
						  << "' - verify data compatibility" << std::endl;
				}
				break;
			}

			case MigrationOperation::Type::AddField:
			{
				// Adding fields is generally safe
				// Could check if field already exists in current version
				for (const auto& mv : targetStruct->getMemberVariables())
				{
					if (mv.identifier == op.fieldName)
					{
						PLOGW << "Migration adds field '" << op.fieldName 
							  << "' which already exists in current struct '" << migration.structName << "'" << std::endl;
						break;
					}
				}
				break;
			}

			case MigrationOperation::Type::ChangeModifier:
			case MigrationOperation::Type::SetModifier:
			case MigrationOperation::Type::RemoveModifier:
			case MigrationOperation::Type::SetDescription:
			{
				// These are metadata changes, generally safe
				break;
			}

			default:
				PLOGW << "Unknown migration operation type in migration for '" << migration.structName << "'" << std::endl;
				break;
		}
	}

	return true;
}

std::vector<StructDefinition> &ProgramStructure::getStructs()
{
	return structs;
}

std::vector<EnumDefinition> &ProgramStructure::getEnums()
{
	return enums;
}

Version ProgramStructure::getLatestMigrationVersion(const std::string& structName)
{
	Version latest{0, 0, 0};
	
	for (const auto& migration : migrations)
	{
		if (migration.structName == structName)
		{
			// Check if toVersion is newer than current latest
			if (migration.toVersion.major > latest.major ||
				(migration.toVersion.major == latest.major && migration.toVersion.minor > latest.minor) ||
				(migration.toVersion.major == latest.major && migration.toVersion.minor == latest.minor && migration.toVersion.patch > latest.patch))
			{
				latest = migration.toVersion;
			}
		}
	}
	
	return latest;
}

void ProgramStructure::reconstructStructAtVersion(const std::string& structName, const Version& version, StructDefinition& outStruct)
{
	// Find current struct as starting point
	StructDefinition* currentStruct = nullptr;
	for (auto& s : structs)
	{
		if (s.getIdentifier() == structName)
		{
			currentStruct = &s;
			break;
		}
	}
	
	if (!currentStruct)
	{
		PLOGE << "Cannot reconstruct struct: '" << structName << "' not found" << std::endl;
		return;
	}
	
	// Start with current struct
	outStruct = *currentStruct;
	
	// Apply migrations in reverse to go back to target version
	// This is a simplified approach - in production you'd want to store historical versions
	// For now, we'll just work forward from version 0.0.0
	outStruct.getMemberVariables().clear();
	
	// Apply all migrations up to target version
	std::vector<MigrationDefinition> applicableMigrations;
	for (const auto& migration : migrations)
	{
		if (migration.structName == structName)
		{
			// Check if this migration's toVersion <= target version
			if (migration.toVersion.major < version.major ||
				(migration.toVersion.major == version.major && migration.toVersion.minor < version.minor) ||
				(migration.toVersion.major == version.major && migration.toVersion.minor == version.minor && migration.toVersion.patch <= version.patch))
			{
				applicableMigrations.push_back(migration);
			}
		}
	}
	
	// Apply migrations in order
	for (const auto& migration : applicableMigrations)
	{
		for (const auto& op : migration.operations)
		{
			switch (op.type)
			{
				case MigrationOperation::Type::AddField:
				{
					MemberVariableDefinition newField;
					newField.identifier = op.fieldName;
					if (op.newType.has_value())
					{
						newField.type = TypeDefinition(op.newType.value());
					}
					outStruct.add_member_variable(newField);
					break;
				}
				case MigrationOperation::Type::RemoveField:
				{
					auto& vars = outStruct.getMemberVariables();
					vars.erase(std::remove_if(vars.begin(), vars.end(),
						[&](const MemberVariableDefinition& v) { return v.identifier == op.fieldName; }),
						vars.end());
					break;
				}
				case MigrationOperation::Type::RenameField:
				{
					if (op.newFieldName.has_value())
					{
						for (auto& var : outStruct.getMemberVariables())
						{
							if (var.identifier == op.fieldName)
							{
								var.identifier = op.newFieldName.value();
								break;
							}
						}
					}
					break;
				}
				default:
					break;
			}
		}
	}
}

MigrationDefinition ProgramStructure::generateMigrationFromDiff(const std::string& structName, const Version& fromVersion, const Version& toVersion)
{
	MigrationDefinition migration;
	migration.structName = structName;
	migration.fromVersion = fromVersion;
	migration.toVersion = toVersion;
	
	// Find current struct
	StructDefinition* currentStruct = nullptr;
	for (auto& s : structs)
	{
		if (s.getIdentifier() == structName)
		{
			currentStruct = &s;
			break;
		}
	}
	
	if (!currentStruct)
	{
		PLOGE << "Cannot generate migration: struct '" << structName << "' not found" << std::endl;
		return migration;
	}
	
	// Reconstruct old version
	StructDefinition oldStruct;
	reconstructStructAtVersion(structName, fromVersion, oldStruct);
	
	// Build field maps for comparison
	std::map<std::string, MemberVariableDefinition> oldFields;
	for (const auto& field : oldStruct.getMemberVariables())
	{
		oldFields[field.identifier] = field;
	}
	
	std::map<std::string, MemberVariableDefinition> newFields;
	for (const auto& field : currentStruct->getMemberVariables())
	{
		newFields[field.identifier] = field;
	}
	
	// Detect added fields
	for (const auto& [name, field] : newFields)
	{
		if (oldFields.find(name) == oldFields.end())
		{
			MigrationOperation op;
			op.type = MigrationOperation::Type::AddField;
			op.fieldName = name;
			op.newType = field.type.identifier();
			migration.operations.push_back(op);
			
			PLOGI << "Auto-generated migration: Add field '" << name << "' to " << structName << std::endl;
		}
	}
	
	// Detect removed fields
	for (const auto& [name, field] : oldFields)
	{
		if (newFields.find(name) == newFields.end())
		{
			MigrationOperation op;
			op.type = MigrationOperation::Type::RemoveField;
			op.fieldName = name;
			migration.operations.push_back(op);
			
			PLOGW << "Auto-generated migration: Remove field '" << name << "' from " << structName << " (destructive)" << std::endl;
		}
	}
	
	// Detect type changes
	for (const auto& [name, newField] : newFields)
	{
		auto it = oldFields.find(name);
		if (it != oldFields.end())
		{
			const auto& oldField = it->second;
			if (oldField.type.identifier() != newField.type.identifier())
			{
				MigrationOperation op;
				op.type = MigrationOperation::Type::ChangeType;
				op.fieldName = name;
				op.oldType = oldField.type.identifier();
				op.newType = newField.type.identifier();
				migration.operations.push_back(op);
				
				PLOGW << "Auto-generated migration: Change field '" << name << "' type from " 
					  << oldField.type.identifier() << " to " << newField.type.identifier() << std::endl;
			}
		}
	}
	
	return migration;
}

bool ProgramStructure::writeMigrationFile(const std::string& filePath, const MigrationDefinition& migration)
{
	std::ofstream file(filePath);
	if (!file.is_open())
	{
		PLOGE << "Failed to create migration file: " << filePath << std::endl;
		return false;
	}
	
	file << "migration struct " << migration.structName 
		 << " from " << migration.fromVersion.major << "." << migration.fromVersion.minor << "." << migration.fromVersion.patch
		 << " to " << migration.toVersion.major << "." << migration.toVersion.minor << "." << migration.toVersion.patch << " {\n";
	
	for (const auto& op : migration.operations)
	{
		file << "    ";
		
		switch (op.type)
		{
			case MigrationOperation::Type::AddField:
				file << "add field " << op.fieldName;
				if (op.newType.has_value())
					file << " " << op.newType.value();
				file << " required";
				break;
				
			case MigrationOperation::Type::RemoveField:
				file << "remove field " << op.fieldName;
				break;
				
			case MigrationOperation::Type::RenameField:
				file << "rename field " << op.fieldName << " to " << op.newFieldName.value();
				break;
				
			case MigrationOperation::Type::ChangeType:
				file << "change field " << op.fieldName << " type from " 
					 << op.oldType.value() << " to " << op.newType.value();
				break;
				
			default:
				file << "// Unknown operation";
				break;
		}
		
		file << ";\n";
	}
	
	file << "}\n";
	file.close();
	
	PLOGI << "Created migration file: " << filePath << std::endl;
	return true;
}

void ProgramStructure::autoGenerateMigrations(const std::string& migrationsPath)
{
	if (migrationsPath.empty())
	{
		return; // No migrations path specified
	}
	
	// Ensure migrations directory exists
	if (!std::filesystem::exists(migrationsPath))
	{
		std::filesystem::create_directories(migrationsPath);
	}
	
	// For each struct, check if we need to generate a new migration
	for (auto& currentStruct : structs)
	{
		std::string structName = currentStruct.getIdentifier();
		Version currentVersion = currentStruct.getVersion();
		Version latestMigrationVersion = getLatestMigrationVersion(structName);
		
		// Check if current version is newer than latest migration
		bool needsNewMigration = false;
		if (currentVersion.major > latestMigrationVersion.major ||
			(currentVersion.major == latestMigrationVersion.major && currentVersion.minor > latestMigrationVersion.minor) ||
			(currentVersion.major == latestMigrationVersion.major && currentVersion.minor == latestMigrationVersion.minor && currentVersion.patch > latestMigrationVersion.patch))
		{
			needsNewMigration = true;
		}
		
		if (needsNewMigration)
		{
			// If no migrations exist yet, create initial migration from 0.0.0
			if (latestMigrationVersion.major == 0 && latestMigrationVersion.minor == 0 && latestMigrationVersion.patch == 0)
			{
				PLOGI << "Generating initial migration for " << structName << " from 0.0.0 to " 
					  << currentVersion.major << "." << currentVersion.minor << "." << currentVersion.patch << std::endl;
				
				// Create initial migration with all current fields as "add field"
				MigrationDefinition initialMigration;
				initialMigration.structName = structName;
				initialMigration.fromVersion = {0, 0, 0};
				initialMigration.toVersion = currentVersion;
				
				// Add all current fields
				for (const auto& field : currentStruct.getMemberVariables())
				{
					MigrationOperation op;
					op.type = MigrationOperation::Type::AddField;
					op.fieldName = field.identifier;
					op.newType = field.type.identifier();
					initialMigration.operations.push_back(op);
					
					PLOGI << "  - Add field '" << field.identifier << "' (" << field.type.identifier() << ")" << std::endl;
				}
				
				// Write migration file
				std::string filename = structName + "_0_0_0_to_" + 
									   std::to_string(currentVersion.major) + "_" + 
									   std::to_string(currentVersion.minor) + "_" + 
									   std::to_string(currentVersion.patch) + ".schema.migration";
				
				std::filesystem::path migrationFilePath = std::filesystem::path(migrationsPath) / filename;
				
				if (writeMigrationFile(migrationFilePath.string(), initialMigration))
				{
					// Add to migrations list
					migrations.push_back(initialMigration);
				}
			}
			else
			{
				// Generate incremental migration from latest to current version
				PLOGI << "Generating new migration for " << structName << " from " 
					  << latestMigrationVersion.major << "." << latestMigrationVersion.minor << "." << latestMigrationVersion.patch
					  << " to " << currentVersion.major << "." << currentVersion.minor << "." << currentVersion.patch << std::endl;
				
				MigrationDefinition newMigration = generateMigrationFromDiff(structName, latestMigrationVersion, currentVersion);
				
				// Write migration file
				std::string filename = structName + "_" + 
									   std::to_string(latestMigrationVersion.major) + "_" + 
									   std::to_string(latestMigrationVersion.minor) + "_" + 
									   std::to_string(latestMigrationVersion.patch) + "_to_" + 
									   std::to_string(currentVersion.major) + "_" + 
									   std::to_string(currentVersion.minor) + "_" + 
									   std::to_string(currentVersion.patch) + ".schema.migration";
				
				std::filesystem::path migrationFilePath = std::filesystem::path(migrationsPath) / filename;
				
				if (writeMigrationFile(migrationFilePath.string(), newMigration))
				{
					// Add to migrations list
					migrations.push_back(newMigration);
				}
			}
		}
	}
}