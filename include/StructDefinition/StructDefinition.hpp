#pragma once
#include <string>
#include <vector>
#include <FunctionDefinition/FunctionDefinition.hpp>
#include <PrivateVariableDefinition/PrivateVariableDefinition.hpp>
#include <MemberVariableDefinition/MemberVariableDefinition.hpp>

struct StructDefinition
{
	std::vector<std::string> includes;
	std::vector<std::string> before_lines;
	std::string identifier;

	std::vector<FunctionDefinition> functions;

	std::vector<PrivateVariableDefinition> private_variables;
	std::vector<MemberVariableDefinition> member_variables;

	void clear();
	void add_function(FunctionDefinition fd);
	void add_private_variable(PrivateVariableDefinition pv);
	void add_MemberVariableDefinition(MemberVariableDefinition mv);
	bool has_MemberVariableDefinition(std::string identifier);
	bool has_private_variable(std::string identifier);
	bool has_function(std::string identifier);
};