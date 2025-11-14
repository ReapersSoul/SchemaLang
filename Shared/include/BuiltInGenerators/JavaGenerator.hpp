#pragma once
#include <ForwardDeclerations.hpp>
#include <ProgramStructure.hpp>
#include <Generator.hpp>

class JavaGenerator : public Generator
{
	std::vector<std::shared_ptr<Generator>> generators;

	void generate_enum_file(EnumDefinition e, std::string out_path);
	
	void generate_struct_file(StructDefinition s, std::shared_ptr<ProgramStructure>ps, std::string out_path, std::vector<StructDefinition> base_classes);
	
	void generate_member_variable_getter(MemberVariableDefinition &mv, std::shared_ptr<ProgramStructure>ps, std::ofstream &structFile);
	
	void generate_member_variable_setter(MemberVariableDefinition &mv, std::shared_ptr<ProgramStructure>ps, std::ofstream &structFile);
	
	void generate_constructor(StructDefinition &s, std::shared_ptr<ProgramStructure>ps, std::ofstream &structFile);
	
	void generate_to_string_method(StructDefinition &s, std::shared_ptr<ProgramStructure>ps, std::ofstream &structFile);
	
	void generate_equals_method(StructDefinition &s, std::shared_ptr<ProgramStructure>ps, std::ofstream &structFile);
	
	void generate_hash_code_method(StructDefinition &s, std::shared_ptr<ProgramStructure>ps, std::ofstream &structFile);
	
	void generate_generator_methods(StructDefinition &s, std::shared_ptr<ProgramStructure>ps, std::vector<StructDefinition> &base_classes, std::ofstream &structFile);
	
	std::string get_java_type(TypeDefinition type, std::shared_ptr<ProgramStructure>ps);
	
	std::string get_java_default_value(TypeDefinition type, std::shared_ptr<ProgramStructure>ps);

public:
	std::string format_include(std::string ident) override{return"";};

	std::string format_default(std::shared_ptr<ProgramStructure>ps, TypeDefinition type,std::string value="") override{return value;};

	JavaGenerator();

	bool add_generator(std::shared_ptr<Generator>gen) override;

	std::string convert_to_local_type(std::shared_ptr<ProgramStructure>ps, TypeDefinition type);

	bool add_generator_specific_content_to_struct(std::shared_ptr<Generator>gen, std::shared_ptr<ProgramStructure>ps, StructDefinition &s);

	bool generate_files(std::shared_ptr<ProgramStructure> ps, std::string out_path);
};
