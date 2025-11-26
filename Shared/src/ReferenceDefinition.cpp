#include "ReferenceDefinition.hpp"

inja::json ReferenceDefinition::to_json(std::shared_ptr<ProgramStructure>ps, std::shared_ptr<Generator> generator)
{
    inja::json j;
    j["struct_name"] = struct_name;
    j["variable_name"] = variable_name;
    return j;
}

void ReferenceDefinition::from_json(const inja::json &j)
{
    struct_name = j["struct_name"].get<std::string>();
    variable_name = j["variable_name"].get<std::string>();
}
