#include <StructDefinition.hpp>
#include <Generator.hpp>

bool StructDefinition::add_include(std::string include)
{
	auto result = includes.insert(include);
	return result.second; // true if inserted, false if already existed
}

bool StructDefinition::add_before_line(std::string line)
{
	if (has_before_line(line)) {
		return false; // Before line already exists
	}
	before_lines.emplace_back(line);
	return true;
}

bool StructDefinition::add_before_setter_line(std::string line)
{
	if (has_before_setter_line(line)) {
		return false; // Before setter line already exists
	}
	before_setter_lines.emplace_back(line);
	return true;
}

bool StructDefinition::add_before_getter_line(std::string line)
{
	if (has_before_getter_line(line)) {
		return false; // Before getter line already exists
	}
	before_getter_lines.emplace_back(line);
	return true;
}

bool StructDefinition::add_function(FunctionDefinition fd)
{
	functions.emplace_back(fd);
	return true;
}

bool StructDefinition::add_private_variable(PrivateVariableDefinition pv)
{
	if (has_private_variable(pv.identifier)) {
		return false; // Private variable already exists
	}
	private_variables.emplace_back(pv);
	return true;
}

bool StructDefinition::add_member_variable(MemberVariableDefinition mv)
{
	if (has_member_variable(mv.identifier)) {
		return false; // Member variable already exists
	}
	member_variables.emplace_back(mv);
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
		if (inc == include) {
			return true; // Include exists
		}
	}
	return false;
}

bool StructDefinition::has_before_line(std::string line)
{
	for (const auto& bl : before_lines) {
		if (bl == line) {
			return true; // Before line exists
		}
	}
	return false;
}

bool StructDefinition::has_before_setter_line(std::string line)
{
	for (const auto& bl : before_setter_lines) {
		if (bl == line) {
			return true; // Before setter line exists
		}
	}
	return false;
}

bool StructDefinition::has_before_getter_line(std::string line)
{
	for (const auto& bl : before_getter_lines) {
		if (bl == line) {
			return true; // Before getter line exists
		}
	}
	return false;
}

bool StructDefinition::has_function(std::string identifier)
{
	for (const auto& func : functions) {
		if (func.identifier == identifier) {
			return true; // Function exists
		}
	}
	return false;
}

bool StructDefinition::has_private_variable(std::string identifier)
{
	for (const auto& pv : private_variables) {
		if (pv.identifier == identifier) {
			return true; // Private variable exists
		}
	}
	return false;
}

bool StructDefinition::has_member_variable(std::string identifier)
{
	for (const auto& mv : member_variables) {
		if (mv.identifier == identifier) {
			return true; // Member variable exists
		}
	}
	return false;
}

MemberVariableDefinition& StructDefinition::get_member_variable(std::string identifier)
{
	for (auto& mv : member_variables) {
		if (mv.identifier == identifier) {
			return mv;
		}
	}
	throw std::runtime_error("StructDefinition::get_member_variable() - Member variable '" + identifier + "' not found in struct '" + this->identifier + "'");
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

void StructDefinition::update(StructDefinition def)
{
	// Merge
	if (identifier!=def.identifier){
		throw std::runtime_error("StructDefinition::update() - Conflicting struct identifiers: current='" + identifier + "', new='" + def.identifier + "'. Cannot merge structs with different identifiers.");
	}

	// Merge includes (set of pairs). Use add_include to maintain uniqueness.
	for (const auto &inc : def.includes) {
		// inc is pair<generator, include>
		add_include(inc);
	}

	// Merge before lines
	for (const auto &bl : def.before_lines) {
		add_before_line(bl);
	}

	// Merge before setter lines
	for (const auto &bsl : def.before_setter_lines) {
		add_before_setter_line(bsl);
	}

	// Merge before getter lines
	for (const auto &bgl : def.before_getter_lines) {
		add_before_getter_line(bgl);
	}

	// Merge functions
	for (const auto &fn : def.functions) {
		if (!has_function(fn.identifier)) {
			add_function(fn);
		}
	}

	// Merge private variables
	for (const auto &pv : def.private_variables) {
		if (!has_private_variable(pv.identifier)) {
			add_private_variable(pv);
		}
	}

	// Merge member variables
	for (const auto &mv : def.member_variables) {
		if (!has_member_variable(mv.identifier)) {
			add_member_variable(mv);
		}
	}

	// Merge enabled/disabled generator sets
	for (const auto &g : def.enabled_for_generators) {
		add_gen_enabled(g);
	}
	for (const auto &g : def.disabled_for_generators) {
		add_gen_disabled(g);
	}
}

inja::json StructDefinition::to_json(std::shared_ptr<ProgramStructure> ps, std::shared_ptr<Generator> generator)
{
    inja::json j;
	j["version"]["major"] = version.major;
	j["version"]["minor"] = version.minor;
	j["version"]["patch"] = version.patch;
    j["identifier"] = identifier;
	std::string identifierCamel = identifier;
	identifierCamel[0] = toupper(identifierCamel[0]);
	j["identifierCamel"] = identifierCamel;
	std::set<std::string> includesSet(includes.begin(), includes.end());
	for (auto &mv:member_variables){
		if(mv.type.is_struct(ps)||mv.type.is_enum(ps)){
			includesSet.insert(generator->format_include(mv.type.identifier()+"Schema.hpp"));
		}
		if(mv.type.is_array()){
			if(mv.type.element_type().is_struct(ps)||mv.type.element_type().is_enum(ps)){
				includesSet.insert(generator->format_include(mv.type.element_type().identifier()+"Schema.hpp"));
			}	
		}
	}
    j["includes"] = includesSet;
	
    j["before_lines"] = before_lines;
    j["before_setter_lines"] = before_setter_lines;
    j["before_getter_lines"] = before_getter_lines;
    j["functions"] = inja::json::array();
    for (auto &func : functions) {
        j["functions"].push_back(func.to_json(ps,generator));
    }
    j["private_variables"] = inja::json::array();
    for (auto &pv : private_variables) {
        j["private_variables"].push_back(pv.to_json(ps,generator));
    }
    j["member_variables"] = inja::json::array();
    for (auto &mv : member_variables) {
        j["member_variables"].push_back(mv.to_json(ps,generator));
    }
    j["enabled_for_generators"] = std::vector<std::string>(enabled_for_generators.begin(), enabled_for_generators.end());
    j["disabled_for_generators"] = std::vector<std::string>(disabled_for_generators.begin(), disabled_for_generators.end());
    return j;
}

void StructDefinition::from_json(const inja::json& j)
{
	version.major = j["version"][0];
	version.minor = j["version"][1];
	version.patch = j["version"][2];
	identifier = j["identifier"].get<std::string>();
	includes = j["includes"].get<std::set<std::string>>();
	before_lines = j["before_lines"].get<std::vector<std::string>>();
	before_setter_lines = j["before_setter_lines"].get<std::vector<std::string>>();
	before_getter_lines = j["before_getter_lines"].get<std::vector<std::string>>();
	functions.clear();
	for (const auto& func_json : j["functions"]) {
		FunctionDefinition func;
		func.from_json(func_json);
		functions.push_back(func);
	}
	private_variables.clear();
	for (const auto& pv_json : j["private_variables"]) {
		PrivateVariableDefinition pv;
		pv.from_json(pv_json);
		private_variables.push_back(pv);
	}
	member_variables.clear();
	for (const auto& mv_json : j["member_variables"]) {
		MemberVariableDefinition mv;
		mv.from_json(mv_json);
		member_variables.push_back(mv);
	}
	enabled_for_generators = std::set<std::string>(j["enabled_for_generators"].begin(), j["enabled_for_generators"].end());
	disabled_for_generators = std::set<std::string>(j["disabled_for_generators"].begin(), j["disabled_for_generators"].end());
}