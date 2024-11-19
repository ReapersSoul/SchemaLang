#pragma once
#include <ForwardDeclerations.hpp>
#include <TypeDefinition/TypeDefinition.hpp>
#include <ForiegnKeyDefinition/ForiegnKeyDefinition.hpp>

struct MemberVariableDefinition
{
	TypeDefinition type;
	std::string identifier = "";

	bool required = false;
	bool auto_increment = false;
	bool primary_key = false;
	bool unique = false;
	ForiegnKeyDefinition fk = {"", ""};
	std::string description = "";
	int min_items = 0;
	int max_tokens = 0;
	bool unique_items = false;

	bool in_class_init = false;
	std::function<bool(ProgramStructure *ps, MemberVariableDefinition &mv, std::ofstream &structFile)> generate_initializer;

	bool static_member = false;
	bool const_member = false;
};