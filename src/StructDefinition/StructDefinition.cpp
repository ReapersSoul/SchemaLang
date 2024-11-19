#include <StructDefinition/StructDefinition.hpp>

void StructDefinition::clear()
{
	includes.clear();
	before_lines.clear();
	identifier.clear();
	functions.clear();
	private_variables.clear();
	member_variables.clear();
}

void StructDefinition::add_function(FunctionDefinition fd)
{
	functions.push_back(fd);
}

void StructDefinition::add_private_variable(PrivateVariableDefinition pv)
{
	private_variables.push_back(pv);
}

void StructDefinition::add_MemberVariableDefinition(MemberVariableDefinition mv)
{
	member_variables.push_back(mv);
}

bool StructDefinition::has_MemberVariableDefinition(std::string identifier)
{
	for (auto &mv : member_variables)
	{
		if (mv.identifier == identifier)
		{
			return true;
		}
	}
	return false;
}

bool StructDefinition::has_private_variable(std::string identifier)
{
	for (auto &pv : private_variables)
	{
		if (pv.identifier == identifier)
		{
			return true;
		}
	}
	return false;
}

bool StructDefinition::has_function(std::string identifier)
{
	for (auto &fd : functions)
	{
		if (fd.identifier == identifier)
		{
			return true;
		}
	}
	return false;
}
