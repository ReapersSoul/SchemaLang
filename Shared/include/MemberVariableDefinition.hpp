#pragma once
#include <ForwardDeclerations.hpp>
#include <TypeDefinition.hpp>
#include <ReferenceDefinition.hpp>
#include <set>

struct MemberVariableDefinition
{
	TypeDefinition type;
	std::string identifier = "";

	bool required = false;
	bool auto_increment = false;
	bool primary_key = false;
	bool unique = false;
	ReferenceDefinition reference = {"", ""};
	std::string description = "";
	std::string default_value = "";
	int min_items = 0;
	int max_items = 0;

	bool in_class_init = false;
	std::function<bool(std::shared_ptr<ProgramStructure>ps, MemberVariableDefinition &mv, std::ofstream &structFile)> generate_initializer;

	bool static_member = false;
	bool const_member = false;
	std::set<std::string> enabled_for_generators;
	std::set<std::string> disabled_for_generators;

	inja::json to_json(std::shared_ptr<ProgramStructure>ps,std::shared_ptr<Generator> generator);
};