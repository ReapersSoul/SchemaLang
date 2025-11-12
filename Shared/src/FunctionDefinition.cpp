#include "FunctionDefinition.hpp"

inja::json FunctionDefinition::to_json(ProgramStructure* ps, Generator* generator)
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