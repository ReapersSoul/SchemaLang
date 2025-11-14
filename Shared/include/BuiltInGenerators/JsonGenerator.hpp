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

	json structToSchema(StructDefinition s, std::shared_ptr<ProgramStructure>ps);

public:
	std::string format_include(std::string ident) override{return"";};
	std::string format_default(std::shared_ptr<ProgramStructure>ps, TypeDefinition type,std::string value="") override{return value;};

	JsonGenerator();

	std::string convert_to_local_type(std::shared_ptr<ProgramStructure>ps, TypeDefinition type);

	bool add_generator_specific_content_to_struct(std::shared_ptr<Generator>gen, std::shared_ptr<ProgramStructure>ps, StructDefinition &s);

	bool generate_files(std::shared_ptr<ProgramStructure> ps, std::string out_path);
};