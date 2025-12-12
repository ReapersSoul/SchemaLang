#pragma once
#include <ForwardDeclerations.hpp>
#include <ProgramStructure.hpp>
#include <Generator.hpp>

class SqliteGenerator : public Generator
{
	TypeDefinition sqlite_db = TypeDefinition("sqlite3 *");

	std::string escape_string(std::string str);

	// Function to add foreign key columns for array relationships
	void add_foreign_key_columns_for_arrays(std::shared_ptr<ProgramStructure>ps);

public:
	std::string format_include(std::string ident) override{return"";};
	std::string format_default(std::shared_ptr<ProgramStructure>ps, TypeDefinition type,std::string value="") override{
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

	std::string convert_to_local_type(std::shared_ptr<ProgramStructure>ps, TypeDefinition type);

	bool add_generator_specific_content_to_struct(std::shared_ptr<Generator>gen, std::shared_ptr<ProgramStructure>ps, StructDefinition &s);

	bool generate_files(std::shared_ptr<ProgramStructure> ps, std::string out_path);
	
	bool generate_migration_files(std::shared_ptr<ProgramStructure> ps, std::string out_path) override;
};