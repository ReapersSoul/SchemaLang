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
	int getUniqueSubsetCount() const
	{
		int count = 0;
		int n = member_variables.size();
		if (n == 0)
			return 0;
		count = (1 << n) - 1; // 2^n - 1
		return count;
	}

	std::string getIdentifier() const { return identifier; }
	void setIdentifier(const std::string &id) { identifier = id; }

	std::set<generator_otherwise_pair<std::string>>& getIncludes(){
		return includes;
	}
	std::vector<generator_otherwise_pair<std::string>>& getBeforeLines(){
		return before_lines;
	}
	std::vector<generator_otherwise_pair<std::string>>& getBeforeSetterLines(){
		return before_setter_lines;
	}
	std::vector<generator_otherwise_pair<std::string>>& getBeforeGetterLines(){
		return before_getter_lines;
	}
	std::vector<generator_otherwise_pair<FunctionDefinition>>& getFunctions(){
		return functions;
	}
	std::vector<generator_otherwise_pair<PrivateVariableDefinition>>& getPrivateVariables(){
		return private_variables;
	}
	std::vector<generator_otherwise_pair<MemberVariableDefinition>>& getMemberVariables(){
		return member_variables;
	}


	bool add_include(std::string include, std::string generator = "");
	bool add_before_line(std::string line, std::string generator = "");
	bool add_before_setter_line(std::string line, std::string generator = "");
	bool add_before_getter_line(std::string line, std::string generator = "");
	bool add_function(FunctionDefinition fd, std::string generator = "");
	bool add_private_variable(PrivateVariableDefinition pv, std::string generator = "");
	bool add_member_variable(MemberVariableDefinition mv, std::string generator = "");
	bool add_gen_enabled(std::string gen);
	bool add_gen_disabled(std::string gen);

	bool has_include(std::string include);
	bool has_before_line(std::string line);
	bool has_before_setter_line(std::string line);
	bool has_before_getter_line(std::string line);
	bool has_function(std::string identifier);
	bool has_private_variable(std::string identifier);
	bool has_member_variable(std::string identifier);

	void clear();

	bool isGenEnabled(std::string gen);
	bool isGenDisabled(std::string gen);
    bool whitelist();
	bool blacklist();

	void update(StructDefinition def);
private:
	std::set<generator_otherwise_pair<std::string>> includes;
	std::vector<generator_otherwise_pair<std::string>> before_lines;
	std::string identifier;

	std::vector<generator_otherwise_pair<std::string>> before_setter_lines;
	std::vector<generator_otherwise_pair<std::string>> before_getter_lines;

	std::vector<generator_otherwise_pair<FunctionDefinition>> functions;

	std::vector<generator_otherwise_pair<PrivateVariableDefinition>> private_variables;
	std::vector<generator_otherwise_pair<MemberVariableDefinition>> member_variables;

	std::set<std::string> enabled_for_generators;
	std::set<std::string> disabled_for_generators;
};