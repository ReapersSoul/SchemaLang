#pragma once
#include <ForwardDeclerations.hpp>
#include <StructDefinition.hpp>

struct Generator
{
	StructDefinition base_class;
	std::string name;

	virtual std::string convert_to_local_type(ProgramStructure *ps, TypeDefinition type) = 0;

	virtual bool add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s) = 0;

	virtual bool generate_files(ProgramStructure ps, std::string out_path) = 0;

	virtual bool add_generator(Generator *gen){
		return false;
	};
};