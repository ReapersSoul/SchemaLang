#pragma once
#include <string>
#include <vector>
#include <set>
#include <FunctionDefinition.hpp>
#include <PrivateVariableDefinition.hpp>
#include <MemberVariableDefinition.hpp>

#include <map>

template<typename T>
using generator_otherwise_pair = std::pair<std::string, T>;

struct StructDefinition
{
	struct CompareBySecond {
		bool operator()(const generator_otherwise_pair<std::string>& a, const generator_otherwise_pair<std::string>& b) const {
			return a.second < b.second;  // ignore .first for comparison
		}
	};
	std::set<generator_otherwise_pair<std::string>,CompareBySecond> includes;
	std::vector<generator_otherwise_pair<std::string>> before_lines;
	std::string identifier;

	std::vector<FunctionDefinition> functions;

	std::vector<PrivateVariableDefinition> private_variables;
	std::vector<MemberVariableDefinition> member_variables;

	std::vector<generator_otherwise_pair<std::string>> before_setter_lines;
	std::vector<generator_otherwise_pair<std::string>> before_getter_lines;

	void clear();
	void add_function(FunctionDefinition fd);
	void add_private_variable(PrivateVariableDefinition pv);
	void add_MemberVariableDefinition(MemberVariableDefinition mv);
	bool has_MemberVariableDefinition(std::string identifier);
	bool has_private_variable(std::string identifier);
	bool has_function(std::string identifier);

	int getUniqueSubsetCount() const
	{
		int count = 0;
		int n = member_variables.size();
		if (n == 0)
			return 0;
		count = (1 << n) - 1; // 2^n - 1
		return count;
	}
};