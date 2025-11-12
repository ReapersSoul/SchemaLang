#pragma once
#include <ForwardDeclerations.hpp>
#include <ProgramStructure.hpp>
#include <Generator.hpp>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

class JsonGenerator : public Generator
{

	bool isNumberType(std::string type);

	json enumToSchema(EnumDefinition e);

	json structToSchema(StructDefinition s, ProgramStructure *ps);

public:
	std::string format_include(std::string ident) override{return"";};
	std::string format_default(ProgramStructure *ps, TypeDefinition type,std::string value="") override{return value;};

	JsonGenerator();

	std::string convert_to_local_type(ProgramStructure *ps, TypeDefinition type);

	bool add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s);

	bool generate_files(ProgramStructure ps, std::string out_path);
};