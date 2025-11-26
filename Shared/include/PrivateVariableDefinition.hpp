#pragma once
#include <ForwardDeclerations.hpp>
#include <TypeDefinition.hpp>

struct PrivateVariableDefinition
{
	TypeDefinition type;
	std::string identifier = "";
	bool in_class_init = false;
	std::function<bool(std::shared_ptr<ProgramStructure>ps, PrivateVariableDefinition &mv, std::ofstream &structFile)> generate_initializer;
	bool static_member = false;
	bool const_member = false;

	inja::json to_json(std::shared_ptr<ProgramStructure> ps, std::shared_ptr<Generator> generator);
	void from_json(const inja::json& j);
};