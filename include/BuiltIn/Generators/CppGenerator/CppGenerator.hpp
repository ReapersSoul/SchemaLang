#pragma once
#include <ForwardDeclerations.hpp>
#include <ProgramStructure/ProgramStructure.hpp>
#include <Generator/Generator.hpp>

class CppGenerator : public Generator
{
	std::vector<Generator *> generators;

	bool generate_base_class_header_file(Generator *gen, ProgramStructure *ps, std::string out_path);

	void generate_enum_file(EnumDefinition e, std::string out_path);

	bool generate_member_variable_declaration(ProgramStructure *ps, MemberVariableDefinition &mv, std::ofstream &structFile);

	bool generate_member_variable_getter_declaration(ProgramStructure *ps, MemberVariableDefinition &mv, std::ofstream &structFile);

	bool generate_member_variable_setter_declaration(ProgramStructure *ps, MemberVariableDefinition &mv, std::ofstream &structFile);

	bool generate_member_variable_getter_definition(ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv, std::ofstream &structFile);

	bool generate_member_variable_setter_definition(ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv, std::ofstream &structFile);

	bool generate_struct_file_header(std::vector<StructDefinition> base_classes, ProgramStructure *ps, StructDefinition &s, std::string &out_path);

	void generate_struct_file_source(std::vector<StructDefinition> base_classes, ProgramStructure *ps, StructDefinition &s, std::string &out_path);

public:
	CppGenerator();

	bool add_generator(Generator *gen) override;

	std::string convert_to_local_type(ProgramStructure *ps, TypeDefinition type);

	bool add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);

	bool generate_files(ProgramStructure ps, std::string out_path);
};