#pragma once
#include <ForwardDeclerations.hpp>
#include <Generator.hpp>
#include <StructDefinition.hpp>
#include <EnumDefinition.hpp>
#include <Debug.hpp>
#include <memory>

class session;

struct ProgramStructure: std::enable_shared_from_this<ProgramStructure>
{
	std::shared_ptr<session> debug_server;

	// Current parsing context
	std::vector<std::string> already_included_files;
	std::string current_file;
	SourcePosition current_position;
	
	// Schema version tracking
	int schemaLang_transpiler_version_major = -1;  // -1 means unspecified (for backward compatibility)
	int schemaLang_transpiler_version_minor = -1;
	int schemaLang_transpiler_version_patch = -1;
	bool version_specified = false;
	SourcePosition version_position;  // Track where version was declared

	bool isInt(std::string str);

	bool isBreakChar(std::string str);

	bool isBreakChar(char c);

	bool isSpecialBreakChar(std::string c);

	bool isSpecialBreakChar(char c);

	std::vector<Token> tokenizeWithPosition(std::string str, const std::string& file_path);
	std::vector<std::string> tokenize(std::string str); // Keep for backward compatibility

	void reportError(const std::string& message);
	void reportError(const std::string& message, const SourcePosition& position);
	void reportError(const std::string& message, const Token& token);

	bool parseVersion(std::vector<Token> tokens, int &i);
	bool validateVersion();
	std::string getTranspilerVersionString() const;

	bool readMemberVariable(std::vector<Token> tokens, int &i, MemberVariableDefinition &current_MemberVariableDefinition);

	bool readStruct(std::vector<Token> tokens, int &i, StructDefinition &current_struct);

	bool readEnumValue(std::vector<Token> tokens, int &i, EnumDefinition &current_enum, int &curent_index);

	bool readEnum(std::vector<Token> tokens, int &i, EnumDefinition &current_enum);

	bool readConfig(std::vector<Token> tokens, int &i);

	bool validate(bool is_root);

	std::vector<StructDefinition> structs;
	std::vector<EnumDefinition> enums;
	std::vector<std::string> type_names;

	inja::json to_json(std::shared_ptr<Generator> generator);

public:
	bool tokenIsType(std::string token);

	bool tokenIsStruct(std::string token);

	bool tokenIsEnum(std::string token);

	bool tokenIsValidTypeName(std::string token);

	StructDefinition &getStruct(std::string identifier);

	EnumDefinition &getEnum(std::string identifier);

    bool parseTypeNames(std::vector<Token> tokens);

    bool readFile(std::string file_path, bool is_root=true);

	bool generate_files(std::shared_ptr<Generator>gen, std::string out_path);

	std::vector<StructDefinition> &getStructs();

	std::vector<EnumDefinition> &getEnums();

	int getUniqueSubsetCount() const
	{
		int count = 0;
		for (const auto &s : structs)
		{
			count += s.getUniqueSubsetCount();
		}
		return count;
	}
};
