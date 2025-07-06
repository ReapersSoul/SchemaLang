#pragma once
#include <ForwardDeclerations.hpp>
#include <Generator/Generator.hpp>
#include <StructDefinition/StructDefinition.hpp>
#include <EnumDefinition/EnumDefinition.hpp>

struct ProgramStructure
{

	bool isInt(std::string str);

	bool isBreakChar(std::string str);

	bool isBreakChar(char c);

	bool isSpecialBreakChar(std::string c);

	bool isSpecialBreakChar(char c);

	std::vector<std::string> tokenize(std::string str);

	bool readMemberVariable(std::vector<std::string> tokens, int &i, MemberVariableDefinition &current_MemberVariableDefinition);

	bool readStruct(std::vector<std::string> tokens, int &i, StructDefinition &current_struct);

	bool readEnumValue(std::vector<std::string> tokens, int &i, EnumDefinition &current_enum, int &curent_index);

	bool readEnum(std::vector<std::string> tokens, int &i, EnumDefinition &current_enum);

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

    bool parseTypeNames(std::vector<std::string> tokens);

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