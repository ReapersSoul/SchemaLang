#pragma once
#include <ForwardDeclerations.hpp>
#include <ProgramStructure.hpp>
#include <Generator.hpp>

class CppGenerator : public Generator
{
	std::string include_prefix = "";
	bool use_angle_brackets = false; // false for quotes "", true for angle brackets <>

	bool generate_base_class_header_file(std::shared_ptr<Generator>gen, std::shared_ptr<ProgramStructure>ps, std::string out_path);

public:
	CppGenerator();

	bool add_generator(std::shared_ptr<Generator>gen) override;

	std::string convert_to_local_type(std::shared_ptr<ProgramStructure>ps, TypeDefinition type);

	std::string get_default_of_type(std::shared_ptr<ProgramStructure>ps, TypeDefinition type);

	// Helper method to format includes with prefix and bracket type
	std::string format_include(std::string ident) override;

	std::string format_default(std::shared_ptr<ProgramStructure>ps, TypeDefinition type,std::string value="") override;

	bool add_generator_specific_content_to_struct(std::shared_ptr<Generator>gen, std::shared_ptr<ProgramStructure>ps, StructDefinition &s);

	bool generate_files(std::shared_ptr<ProgramStructure> ps, std::string out_path);

	// Methods to configure include behavior
	void set_include_prefix(const std::string& prefix) { include_prefix = prefix; }
	void set_use_angle_brackets(bool use_angle) { use_angle_brackets = use_angle; }
	std::string get_include_prefix() const { return include_prefix; }
	bool get_use_angle_brackets() const { return use_angle_brackets; }
};