#pragma once
#include <ForwardDeclerations.hpp>
#include <ProgramStructure.hpp>
#include <Generator.hpp>

class CppGenerator : public Generator
{
	std::vector<Generator *> generators;
	std::string include_prefix = "";
	bool use_angle_brackets = false; // false for quotes "", true for angle brackets <>

	bool generate_base_class_header_file(Generator *gen, ProgramStructure *ps, std::string out_path);

public:
	CppGenerator();

	bool add_generator(Generator *gen) override;

	std::string convert_to_local_type(ProgramStructure *ps, TypeDefinition type);

	std::string get_default_of_type(ProgramStructure *ps, TypeDefinition type);

	// Helper method to format includes with prefix and bracket type
	std::string format_include(const std::string& filename) const;

	bool add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);

	bool generate_files(ProgramStructure ps, std::string out_path);

	// Methods to configure include behavior
	void set_include_prefix(const std::string& prefix) { include_prefix = prefix; }
	void set_use_angle_brackets(bool use_angle) { use_angle_brackets = use_angle; }
	std::string get_include_prefix() const { return include_prefix; }
	bool get_use_angle_brackets() const { return use_angle_brackets; }
};