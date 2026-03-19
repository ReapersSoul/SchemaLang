#pragma once
#include <string>
#include <ForwardDeclerations.hpp>
#include <inja/inja.hpp>

class TypeDefinition
{
	std::string ident;
	TypeDefinition *elem_type;
	bool defaulted = false;
	bool required = false;
public:
	TypeDefinition();
	TypeDefinition(std::string ident);
    TypeDefinition(std::string ident, bool defaulted);
    TypeDefinition(std::string ident, TypeDefinition elem_type);
    std::string &identifier();
    const std::string &identifier() const;
	bool is_required() const { return required; }
	void setRequired(bool value) { required = value; }
	bool is_array();
	bool is_struct(std::shared_ptr<ProgramStructure>ps);
	bool is_enum(std::shared_ptr<ProgramStructure>ps);
	bool is_base_type();
	bool is_number();
	bool is_integer() const;
	bool is_real() const;
	bool is_bool() const;
	bool is_string() const;
	bool is_char() const;
	bool is_blob() const;
	bool is_array_of_struct(std::shared_ptr<ProgramStructure>ps);
	bool is_array_of_enum(std::shared_ptr<ProgramStructure>ps);
	bool is_array_of_base_type();
	bool is_array_of_number();
	bool is_array_of_integer();
	bool is_array_of_real();
	bool is_array_of_bool();
	bool is_array_of_string();
	bool is_array_of_char();
	bool is_optional();
	bool is_defaulted() const { return defaulted; }
	void setDefaulted(bool value) { defaulted = value; }
	TypeDefinition &element_type();

	inja::json to_json(std::shared_ptr<ProgramStructure>ps, std::shared_ptr<Generator> generator);
	void from_json(inja::json j);
};