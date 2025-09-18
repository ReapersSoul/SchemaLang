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
	std::string format_include(std::string ident) override{return"";};
	std::string format_default(ProgramStructure *ps, TypeDefinition type,std::string value="") override{
		if(type.is_string()){
			if(value=="")
				return "\"\"";
			else
				return "\"" + escape_string(value) + "\"";
		}
		else if(type.is_char()){
			if(value=="")
				return "'\\0'";
			else if(value.length()==1)
				return "'" + escape_string(value) + "'";
			else if(value.length()==3 && value[0]=='\'' && value[2]=='\'')
				return "'" + escape_string(std::string(1,value[1])) + "'";
			else
				return "'\\0'";
		}
		else if(type.is_bool()){
			if(value=="true" || value=="1")
				return "true";
			else
				return "false";
		}
		else if(type.is_integer() || type.is_real() || type.is_number()){
			if(value=="")
				return "0";
			else
				return value;
		}
		else if(type.is_array()){
			return "\"\"";
		}
		else{
			return "\"\"";
		}
	};

	SqliteGenerator();

	std::string convert_to_local_type(ProgramStructure *ps, TypeDefinition type);

	bool add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);

	bool generate_files(ProgramStructure ps, std::string out_path);
};