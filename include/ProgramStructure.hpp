#pragma once
#include <ForwardDeclerations.hpp>
#include <Generator.hpp>
#include <StructDefinition.hpp>
#include <EnumDefinition.hpp>

struct SourcePosition
{
	std::string file_path;
	int line;
	int column;
	
	SourcePosition() : line(1), column(1) {}
	SourcePosition(const std::string& file, int l, int c) : file_path(file), line(l), column(c) {}
};

struct Token
{
	std::string value;
	SourcePosition position;
	
	Token() {}
	Token(const std::string& val, const SourcePosition& pos) : value(val), position(pos) {}
	
	// Comparison operators for convenience
	bool operator==(const std::string& str) const { return value == str; }
	bool operator!=(const std::string& str) const { return value != str; }
	bool operator==(const char* str) const { return value == str; }
	bool operator!=(const char* str) const { return value != str; }
	
	// Implicit conversion to string for compatibility
	operator const std::string&() const { return value; }
};

struct ProgramStructure
{
	// Current parsing context
	std::string current_file;
	SourcePosition current_position;

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

	bool readMemberVariable(std::vector<Token> tokens, int &i, MemberVariableDefinition &current_MemberVariableDefinition);

	bool readStruct(std::vector<Token> tokens, int &i, StructDefinition &current_struct);

	bool readEnumValue(std::vector<Token> tokens, int &i, EnumDefinition &current_enum, int &curent_index);

	bool readEnum(std::vector<Token> tokens, int &i, EnumDefinition &current_enum);

	bool readConfig(std::vector<Token> tokens, int &i);

	bool validate();

	std::vector<StructDefinition> structs;
	std::vector<EnumDefinition> enums;
	std::vector<std::string> type_names;

public:
	bool tokenIsType(std::string token);

	bool tokenIsStruct(std::string token);

	bool tokenIsEnum(std::string token);

	bool tokenIsValidTypeName(std::string token);

	StructDefinition &getStruct(std::string identifier);

	EnumDefinition &getEnum(std::string identifier);

    bool parseTypeNames(std::vector<Token> tokens);

    bool readFile(std::string file_path);

	bool generate_files(Generator *gen, std::string out_path);

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
