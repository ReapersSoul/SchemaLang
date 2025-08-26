#pragma once
#include <ForwardDeclerations.hpp>
#include <set>

struct EnumDefinition
{
	std::string identifier;
	std::vector<std::pair<std::string, int>> values;
	std::set<std::string> enabled_for_generators;
	std::set<std::string> disabled_for_generators;

	void add_value(std::string identifier, int value);

	void clear();
};