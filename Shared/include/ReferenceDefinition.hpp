#pragma once
#include <string>
#include <ForwardDeclerations.hpp>

struct ReferenceDefinition
{
	std::string struct_name = "";
	std::string variable_name = "";

	inja::json to_json(std::shared_ptr<ProgramStructure> ps, std::shared_ptr<Generator> generator);
};