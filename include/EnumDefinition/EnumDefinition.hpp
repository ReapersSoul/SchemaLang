#pragma once
#include <ForwardDeclerations.hpp>

struct EnumDefinition
{
	std::string identifier;
	std::vector<std::pair<std::string, int>> values;

	void add_value(std::string identifier, int value);

	void clear();
};