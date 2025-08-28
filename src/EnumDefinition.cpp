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