#include "FunctionDefinition.hpp"

inja::json FunctionDefinition::to_json(std::shared_ptr<ProgramStructure> ps, std::shared_ptr<Generator> generator)
{
    inja::json j;
    j["identifier"] = identifier;
    std::string identifierCamel = identifier;
	identifierCamel[0] = toupper(identifierCamel[0]);
	j["identifierCamel"] = identifierCamel;
    j["static_function"] = static_function;
    j["return_type"] = return_type.to_json(ps,generator);
    j["parameters"] = inja::json::array();
    for (auto &param : parameters) {
        inja::json param_json;
        param_json["type"] = param.first.to_json(ps,generator);
        param_json["name"] = param.second;
        j["parameters"].push_back(param_json);
    }
    //j["generate_function"] = generate_function(generator, ps,s,this);
    return j;
}

void FunctionDefinition::from_json(const inja::json& j)
{
    identifier = j["identifier"].get<std::string>();
    static_function = j["static_function"].get<bool>();
    return_type.from_json(j["return_type"]);
    parameters.clear();
    for (const auto& param_json : j["parameters"]) {
        TypeDefinition type;
        type.from_json(param_json["type"]);
        std::string name = param_json["name"].get<std::string>();
        parameters.emplace_back(std::make_pair(type, name));
    }
}