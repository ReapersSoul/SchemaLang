#pragma once
#include <ForwardDeclerations.hpp>
#include <ProgramStructure/ProgramStructure.hpp>
#include <Generator/Generator.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <regex>

class MysqlGenerator : public Generator
{
public:
	// Constructor
	MysqlGenerator();

	// Utility functions
	std::vector<std::vector<int>> comb(int N, int K);
	std::vector<std::vector<int>> comb(int N);

	// SQL string generation functions
	std::string generate_create_table_statement_string_struct(ProgramStructure *ps, StructDefinition &s);
	std::string generate_select_all_statement_string_member_variable(StructDefinition &s, MemberVariableDefinition &mv);
	std::vector<std::string> generate_select_all_statements_string_struct(StructDefinition &s);
	std::string generate_select_by_member_variable_statement_string(StructDefinition &s, MemberVariableDefinition &mv_1, std::vector<int> &criteria);
	std::string generate_insert_statement_string_struct(StructDefinition &s);
	std::string generate_update_all_statement_string_struct(StructDefinition &s);

	// Functions for C++ code generation
	void generate_select_all_statement_function_member_variable(Generator *gen, ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv);
	void generate_select_all_statement_functions_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);
	void generate_select_member_variable_function_statement(Generator *gen, ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv_1, std::vector<int> &criteria);
	void generate_select_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);
	void generate_insert_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);
	void generate_update_all_statement_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);
	void generate_update_statements_function_struct();
	void generate_delete_statement_function_struct();

	// File generation functions
	bool generate_create_table_file(ProgramStructure *ps, StructDefinition &s, std::string out_path);
	bool generate_select_all_files(ProgramStructure *ps, StructDefinition &s, std::string out_path);
	bool generate_select_files(ProgramStructure *ps, StructDefinition &s, std::string out_path);
	bool generate_struct_files(ProgramStructure *ps, StructDefinition &s, std::string out_path);

	// Utility functions
	std::string escape_string(std::string str);

	// Function to add foreign key columns for array relationships
	void add_foreign_key_columns_for_arrays(ProgramStructure *ps);

	// Override functions from Generator base class
	std::string convert_to_local_type(ProgramStructure *ps, TypeDefinition type) override;
	bool add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s) override;
	bool generate_files(ProgramStructure ps, std::string out_path) override;

	void set_generate_select_all_files(bool value) { gen_select_all_files = value; }
	void set_generate_select_files(bool value) { gen_select_files = value; }
	void set_generate_insert_files(bool value) { gen_insert_files = value; }
	void set_generate_update_files(bool value) { gen_update_files = value; }
	void set_generate_delete_files(bool value) { gen_delete_files = value; }

private:
	const std::string mysql_connection = "mysqlx::Session";
	const TypeDefinition mysql_session = TypeDefinition("mysqlx::Session &");
	bool gen_select_all_files = false;
	bool gen_select_files = false;
	bool gen_insert_files = false;
	bool gen_update_files = false;
	bool gen_delete_files = false;
};