#include "PrivateVariableDefinition.hpp"

inja::json PrivateVariableDefinition::to_json(std::shared_ptr<ProgramStructure> ps, std::shared_ptr<Generator> generator)
{
    inja::json j;
    j["type"] = type.to_json(ps,generator);
    j["identifier"] = identifier;
    std::string identifierCamel = identifier;
	identifierCamel[0] = toupper(identifierCamel[0]);
	j["identifierCamel"] = identifierCamel;
    j["in_class_init"] = in_class_init;
    j["static_member"] = static_member;
    j["const_member"] = const_member;
    return j;
}