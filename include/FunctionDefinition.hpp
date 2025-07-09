#pragma once
#include <ForwardDeclerations.hpp>
#include <TypeDefinition.hpp>

struct FunctionDefinition
{
	std::string generator;
	std::string identifier;
	bool static_function = false;
	TypeDefinition return_type;
	std::vector<std::pair<TypeDefinition, std::string>> parameters;
	std::function<bool(Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)> generate_function;
};