#include <StructDefinition.hpp>

bool StructDefinition::add_include(std::string include, std::string generator)
{
	for (auto& inc : includes) {
		if (inc.second == include) {
			return false; // Include already exists
		}
	}
	includes.emplace_back(generator, include);
	return true;
}

bool StructDefinition::add_before_line(std::string line, std::string generator)
{
	before_lines.emplace_back(generator, line);
	return true;
}

bool StructDefinition::add_before_setter_line(std::string line, std::string generator)
{
	before_setter_lines.emplace_back(generator, line);
	return true;
}

bool StructDefinition::add_before_getter_line(std::string line, std::string generator)
{
	before_getter_lines.emplace_back(generator, line);
	return true;
}

bool StructDefinition::add_function(FunctionDefinition fd, std::string generator)
{
	functions.emplace_back(generator, fd);
	return true;
}

bool StructDefinition::add_private_variable(PrivateVariableDefinition pv, std::string generator)
{
	private_variables.emplace_back(generator, pv);
	return true;
}

bool StructDefinition::add_member_variable(MemberVariableDefinition mv, std::string generator)
{
	member_variables.emplace_back(generator, mv);
	return true;
}

bool StructDefinition::has_include(std::string include)
{
	for (const auto& inc : includes) {
		if (inc.second == include) {
			return true; // Include exists
		}
	}
	return false;
}

bool StructDefinition::has_before_line(std::string line)
{
	for (const auto& bl : before_lines) {
		if (bl.second == line) {
			return true; // Before line exists
		}
	}
	return false;
}

bool StructDefinition::has_before_setter_line(std::string line)
{
	for (const auto& bl : before_setter_lines) {
		if (bl.second == line) {
			return true; // Before setter line exists
		}
	}
	return false;
}

bool StructDefinition::has_before_getter_line(std::string line)
{
	for (const auto& bl : before_getter_lines) {
		if (bl.second == line) {
			return true; // Before getter line exists
		}
	}
	return false;
}

bool StructDefinition::has_function(std::string identifier)
{
	for (const auto& func : functions) {
		if (func.second.identifier == identifier) {
			return true; // Function exists
		}
	}
	return false;
}

bool StructDefinition::has_private_variable(std::string identifier)
{
	for (const auto& pv : private_variables) {
		if (pv.second.identifier == identifier) {
			return true; // Private variable exists
		}
	}
	return false;
}

bool StructDefinition::has_member_variable(std::string identifier)
{
	for (const auto& mv : member_variables) {
		if (mv.second.identifier == identifier) {
			return true; // Member variable exists
		}
	}
	return false;
}

void StructDefinition::clear(){
	includes.clear();
	before_lines.clear();
	before_setter_lines.clear();
	before_getter_lines.clear();
	functions.clear();
	private_variables.clear();
	member_variables.clear();
	identifier.clear();
}