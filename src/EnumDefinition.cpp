#include <EnumDefinition.hpp>

void EnumDefinition::add_value(std::string identifier, int value)
{
	values.insert(std::make_pair(identifier, value));
}

void EnumDefinition::update(EnumDefinition def)
{
	// Merge
	if (identifier!=def.identifier){
		throw std::runtime_error("Conflicting identifiers in EnumDefinition::update");
	}

	// Merge values
	for (const auto &val : def.values) {
		values.insert(val);
	}
}

void EnumDefinition::clear()
{
	values.clear();
}

inja::json EnumDefinition::to_json(ProgramStructure* ps, Generator* generator)
{
	inja::json j;
	j["identifier"] = identifier;
	std::string identifierCamel = identifier;
	identifierCamel[0] = toupper(identifierCamel[0]);
	j["identifierCamel"] = identifierCamel;
	j["values"] = inja::json::array();
	for (const auto &val : values) {
		inja::json value_json;
		value_json["name"] = val.first;
		value_json["value"] = val.second;
		j["values"].push_back(value_json);
	}
	return j;
}
