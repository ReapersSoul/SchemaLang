#pragma once
#include <ForwardDeclerations.hpp>
#include <TypeDefinition.hpp>

struct PrivateVariableDefinition
{
	std::string generator;
	TypeDefinition type;
	std::string identifier = "";
	bool in_class_init = false;
	std::function<bool(ProgramStructure *ps, PrivateVariableDefinition &mv, std::ofstream &structFile)> generate_initializer;
	bool static_member = false;
	bool const_member = false;
};