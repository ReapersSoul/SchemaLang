#pragma once
#include <ForwardDeclerations.hpp>
#include <ProgramStructure.hpp>
#include <Generator.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <regex>

class LuaGenerator : public Generator
{
	std::vector<Generator *> generators;

public:
	// Constructor
	LuaGenerator();

	// Drop-in system support
	bool add_generator(Generator *gen) override;

	// Lua-specific utility functions
	std::string lua_table_name(const std::string& identifier);
	std::string lua_field_name(const std::string& identifier);
	std::string lua_function_name(const std::string& identifier);

	// Lua table generation functions
	std::string generate_lua_table_struct(ProgramStructure *ps, StructDefinition &s);
	std::string generate_lua_enum_table(EnumDefinition &e);
	std::string generate_lua_validation_function(ProgramStructure *ps, StructDefinition &s);
	std::string generate_lua_serialization_functions(ProgramStructure *ps, StructDefinition &s);

	// File generation functions
	bool generate_struct_lua_file(ProgramStructure *ps, StructDefinition &s, std::string out_path, std::vector<StructDefinition> base_classes);
	bool generate_enum_lua_file(EnumDefinition &e, std::string out_path);
	bool generate_main_lua_file(ProgramStructure &ps, std::string out_path);

	// Utility functions
	std::string escape_lua_string(std::string str);
	std::string lua_type_comment(ProgramStructure *ps, TypeDefinition type);
	std::string lua_type_to_json_type(TypeDefinition type);

	// Override functions from Generator base class
	std::string convert_to_local_type(ProgramStructure *ps, TypeDefinition type) override;
	bool add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s) override;
	bool generate_files(ProgramStructure ps, std::string out_path) override;

private:
	// Helper functions for generating Lua code
	void generate_lua_field(ProgramStructure *ps, MemberVariableDefinition &mv, std::ofstream &luaFile, int indent = 1);
	void generate_lua_constructor(ProgramStructure *ps, StructDefinition &s, std::ofstream &luaFile);
	void generate_lua_tostring(ProgramStructure *ps, StructDefinition &s, std::ofstream &luaFile);
	void generate_lua_validate(ProgramStructure *ps, StructDefinition &s, std::ofstream &luaFile);
	void generate_generator_methods(ProgramStructure *ps, StructDefinition &s, std::vector<StructDefinition> &base_classes, std::ofstream &luaFile);
	std::string get_lua_default_value(ProgramStructure *ps, TypeDefinition type);
	std::string indent_string(int level);
};
