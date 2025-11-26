#pragma once
#include <ForwardDeclerations.hpp>
#include <TypeDefinition.hpp>

struct FunctionDefinition
{
	std::string identifier;
	bool static_function = false;
	TypeDefinition return_type;
	std::vector<std::pair<TypeDefinition, std::string>> parameters;
	std::function<bool(std::shared_ptr<Generator>gen, std::shared_ptr<ProgramStructure>ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)> generate_function;
	inja::json to_json(std::shared_ptr<ProgramStructure> ps, std::shared_ptr<Generator> generator);
    void from_json(const inja::json &j);
};