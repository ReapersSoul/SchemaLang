#pragma once
#include <string>
#include <vector>
#include <set>
#include <FunctionDefinition.hpp>
#include <PrivateVariableDefinition.hpp>
#include <MemberVariableDefinition.hpp>
#include <inja/inja.hpp>
#include <map>

template<typename T>
using generator_otherwise_pair = std::pair<std::string, T>;

struct Additions{
	std::set<std::string> includes;
	std::vector<std::string> before_lines;

	std::vector<std::string> before_setter_lines;
	std::vector<std::string> before_getter_lines;

	std::set<std::string> functions;

	std::vector<std::string> private_variables;
	std::vector<std::string> member_variables;

	inja::json to_json() const{
		inja::json j;
		j["includes"] = includes;
		j["before_lines"] = before_lines;
		j["before_setter_lines"] = before_setter_lines;
		j["before_getter_lines"] = before_getter_lines;
		j["functions"] = functions;
		j["private_variables"] = private_variables;
		j["member_variables"] = member_variables;
		return j;
	}

	void clear(){
		includes.clear();
		before_lines.clear();
		before_setter_lines.clear();
		before_getter_lines.clear();
		functions.clear();
		private_variables.clear();
		member_variables.clear();
	}
};

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

	std::set<std::string>& getIncludes(){
		return includes;
	}
	std::vector<std::string>& getBeforeLines(){
		return before_lines;
	}
	std::vector<std::string>& getBeforeSetterLines(){
		return before_setter_lines;
	}
	std::vector<std::string>& getBeforeGetterLines(){
		return before_getter_lines;
	}
	std::vector<FunctionDefinition>& getFunctions(){
		return functions;
	}
	std::vector<PrivateVariableDefinition>& getPrivateVariables(){
		return private_variables;
	}
	std::vector<MemberVariableDefinition>& getMemberVariables(){
		return member_variables;
	}

	bool add_include(std::string include);
	bool add_before_line(std::string line);
	bool add_before_setter_line(std::string line);
	bool add_before_getter_line(std::string line);
	bool add_function(FunctionDefinition fd);
	bool add_private_variable(PrivateVariableDefinition pv);
	bool add_member_variable(MemberVariableDefinition mv);
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

	inja::json to_json(ProgramStructure* ps, Generator* generator);
private:
	std::set<std::string> includes;
	std::vector<std::string> before_lines;
	std::string identifier;

	std::vector<std::string> before_setter_lines;
	std::vector<std::string> before_getter_lines;

	std::vector<FunctionDefinition> functions;

	std::vector<PrivateVariableDefinition> private_variables;
	std::vector<MemberVariableDefinition> member_variables;

	std::set<std::string> enabled_for_generators;
	std::set<std::string> disabled_for_generators;
};