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

void MemberVariableDefinition::from_json(const inja::json &j)
{
    type.from_json(j["type"]);
    identifier = j["identifier"].get<std::string>();
    auto_increment = j["auto_increment"].get<bool>();
    primary_key = j["primary_key"].get<bool>();
    unique = j["unique"].get<bool>();
    reference.from_json(j["reference"]);
    description = j["description"].get<std::string>();
    default_value = j["default_value"].get<std::string>();
    min_items = j["min_items"].get<int>();
    max_items = j["max_items"].get<int>();
    in_class_init = j["in_class_init"].get<bool>();
    static_member = j["static_member"].get<bool>();
    const_member = j["const_member"].get<bool>();
    enabled_for_generators = std::set<std::string>(j["enabled_for_generators"].begin(), j["enabled_for_generators"].end());
    disabled_for_generators = std::set<std::string>(j["disabled_for_generators"].begin(), j["disabled_for_generators"].end());
}
