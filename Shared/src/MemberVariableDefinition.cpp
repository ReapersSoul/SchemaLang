#include "MemberVariableDefinition.hpp"

inja::json MemberVariableDefinition::to_json(std::shared_ptr<ProgramStructure> ps, std::shared_ptr<Generator> generator)
{
    inja::json j;
    j["type"] = type.to_json(ps,generator);
    j["identifier"] = identifier;
    std::string identifierCamel = identifier;
	identifierCamel[0] = toupper(identifierCamel[0]);
	j["identifierCamel"] = identifierCamel;
    j["auto_increment"] = auto_increment;
    j["primary_key"] = primary_key;
    j["unique"] = unique;
    j["reference"] = reference.to_json(ps,generator);
    j["description"] = description;
    j["default_value"] = default_value;
    j["min_items"] = min_items;
    j["max_items"] = max_items;
    j["in_class_init"] = in_class_init;
    j["static_member"] = static_member;
    j["const_member"] = const_member;
    j["enabled_for_generators"] = std::vector<std::string>(enabled_for_generators.begin(), enabled_for_generators.end());
    j["disabled_for_generators"] = std::vector<std::string>(disabled_for_generators.begin(), disabled_for_generators.end());
    return j;
}