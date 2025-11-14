#include "ReferenceDefinition.hpp"

inja::json ReferenceDefinition::to_json(std::shared_ptr<ProgramStructure>ps, std::shared_ptr<Generator> generator)
{
    inja::json j;
    j["struct_name"] = struct_name;
    j["variable_name"] = variable_name;
    return j;
}