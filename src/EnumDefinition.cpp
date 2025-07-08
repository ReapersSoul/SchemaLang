#include <EnumDefinition.hpp>

void EnumDefinition::add_value(std::string identifier, int value)
{
	values.push_back(std::make_pair(identifier, value));
}

void EnumDefinition::clear()
{
	values.clear();
}