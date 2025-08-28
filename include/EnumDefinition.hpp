#pragma once
#include <ForwardDeclerations.hpp>
#include <set>

struct EnumDefinition
{
	struct CompareByName {
		bool operator()(const std::pair<std::string,int>& a,
						const std::pair<std::string,int>& b) const
		{
			// only compare by the string (first); ints are ignored for ordering/equality
			return a.first < b.first;
		}
	};
	std::string identifier;
	std::set<std::pair<std::string, int>, CompareByName> values;
	std::set<std::string> enabled_for_generators;
	std::set<std::string> disabled_for_generators;

	void add_value(std::string identifier, int value);
	void update(EnumDefinition def);

	void clear();
};