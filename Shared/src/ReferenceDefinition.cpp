#include "ReferenceDefinition.hpp"

inja::json ReferenceDefinition::to_json(ProgramStructure *ps, Generator* generator)
{
    inja::json j;
    j["struct_name"] = struct_name;
    j["variable_name"] = variable_name;
    return j;
}