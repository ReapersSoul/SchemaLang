#pragma once
#include <ForwardDeclerations.hpp>
#include <ProgramStructure.hpp>
#include <Generator.hpp>

class JavaGenerator : public Generator
{
	std::vector<Generator *> generators;

	void generate_enum_file(EnumDefinition e, std::string out_path);
	
	void generate_struct_file(StructDefinition s, ProgramStructure *ps, std::string out_path, std::vector<StructDefinition> base_classes);
	
	void generate_member_variable_getter(MemberVariableDefinition &mv, ProgramStructure *ps, std::ofstream &structFile);
	
	void generate_member_variable_setter(MemberVariableDefinition &mv, ProgramStructure *ps, std::ofstream &structFile);
	
	void generate_constructor(StructDefinition &s, ProgramStructure *ps, std::ofstream &structFile);
	
	void generate_to_string_method(StructDefinition &s, ProgramStructure *ps, std::ofstream &structFile);
	
	void generate_equals_method(StructDefinition &s, ProgramStructure *ps, std::ofstream &structFile);
	
	void generate_hash_code_method(StructDefinition &s, ProgramStructure *ps, std::ofstream &structFile);
	
	void generate_generator_methods(StructDefinition &s, ProgramStructure *ps, std::vector<StructDefinition> &base_classes, std::ofstream &structFile);
	
	std::string get_java_type(TypeDefinition type, ProgramStructure *ps);
	
	std::string get_java_default_value(TypeDefinition type, ProgramStructure *ps);

public:
	JavaGenerator();

	bool add_generator(Generator *gen) override;

	std::string convert_to_local_type(ProgramStructure *ps, TypeDefinition type);

	bool add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);

	bool generate_files(ProgramStructure ps, std::string out_path);
};
