#pragma once
#include <ForwardDeclerations.hpp>
#include <ProgramStructure.hpp>
#include <Generator.hpp>

class SqliteGenerator : public Generator
{
	TypeDefinition sqlite_db = TypeDefinition("sqlite3 *");

	// utility functions
	std::vector<std::vector<int>> comb(int N, int K);

	std::vector<std::vector<int>> comb(int N);

	// sql string generation functions
	std::string generate_create_table_statement_string_struct(ProgramStructure * ps,StructDefinition &s);

	std::string generate_select_all_statement_string_member_variable(StructDefinition &s, MemberVariableDefinition &mv);

	std::vector<std::string> genrate_select_all_statements_string_struct(StructDefinition &s);

	std::string generate_select_by_member_variable_statement_string(StructDefinition &s, MemberVariableDefinition &mv_1, std::vector<int> &criteria);

	std::string generat_insert_statement_string_struct(StructDefinition &s);

	std::string generate_update_all_statement_string_struct(StructDefinition &s);

	std::string generate_delete_statement_string_struct(StructDefinition &s);

	// functions for c++ code generation
	void generate_select_all_statement_function_member_variable(Generator *gen, ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv);

	void generate_select_all_statement_functions_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);

	void generate_select_member_variable_function_statement(Generator *gen, ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv_1, std::vector<int> &criteria);

	void generate_select_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);

	void generate_insert_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);

	void generate_update_all_statement_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);

	void generate_update_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);

	void generate_delete_statement_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);

	// file generation functions
	bool generate_create_table_file(ProgramStructure *ps, StructDefinition &s, std::string out_path);

	bool generate_select_all_files(ProgramStructure *ps, StructDefinition &s, std::string out_path);

	bool generate_select_files(ProgramStructure *ps, StructDefinition &s, std::string out_path);

	bool generate_struct_files(ProgramStructure *ps, StructDefinition &s, std::string out_path);

	std::string escape_string(std::string str);

	// Function to add foreign key columns for array relationships
	void add_foreign_key_columns_for_arrays(ProgramStructure *ps);

public:
	SqliteGenerator();

	std::string convert_to_local_type(ProgramStructure *ps, TypeDefinition type);

	bool add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);

	bool generate_files(ProgramStructure ps, std::string out_path);
};