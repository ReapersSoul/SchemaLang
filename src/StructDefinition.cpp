#include <StructDefinition.hpp>

bool StructDefinition::add_include(std::string include, std::string generator)
{
	auto p = std::make_pair(generator, include);
	auto result = includes.insert(p);
	return result.second; // true if inserted, false if already existed
}

bool StructDefinition::add_before_line(std::string line, std::string generator)
{
	if (has_before_line(line)) {
		return false; // Before line already exists
	}
	before_lines.emplace_back(generator, line);
	return true;
}

bool StructDefinition::add_before_setter_line(std::string line, std::string generator)
{
	if (has_before_setter_line(line)) {
		return false; // Before setter line already exists
	}
	before_setter_lines.emplace_back(generator, line);
	return true;
}

bool StructDefinition::add_before_getter_line(std::string line, std::string generator)
{
	if (has_before_getter_line(line)) {
		return false; // Before getter line already exists
	}
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
	if (has_private_variable(pv.identifier)) {
		return false; // Private variable already exists
	}
	private_variables.emplace_back(generator, pv);
	return true;
}

bool StructDefinition::add_member_variable(MemberVariableDefinition mv, std::string generator)
{
	if (has_member_variable(mv.identifier)) {
		return false; // Member variable already exists
	}
	member_variables.emplace_back(generator, mv);
	return true;
}

bool StructDefinition::add_gen_enabled(std::string gen)
{
	enabled_for_generators.insert(gen);
	return true;
}

bool StructDefinition::add_gen_disabled(std::string gen){
	disabled_for_generators.insert(gen);
	return true;
}

bool StructDefinition::has_include(std::string include)
{
	for (const auto &inc : includes) {
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

bool StructDefinition::isGenEnabled(std::string gen)
{
	return std::find(enabled_for_generators.begin(),enabled_for_generators.end(),gen)!=enabled_for_generators.end();
}

bool StructDefinition::isGenDisabled(std::string gen)
{
	return std::find(disabled_for_generators.begin(),disabled_for_generators.end(),gen)!=disabled_for_generators.end();
}

bool StructDefinition::whitelist(){
	return !enabled_for_generators.empty();
}

bool StructDefinition::blacklist(){
	return !disabled_for_generators.empty();
}