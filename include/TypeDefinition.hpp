#pragma once
#include <string>
#include <ForwardDeclerations.hpp>

class TypeDefinition
{
	std::string ident;
	TypeDefinition *elem_type;

public:
	TypeDefinition();
	TypeDefinition(std::string ident);
	TypeDefinition(std::string ident, TypeDefinition elem_type);
	std::string &identifier();
	bool is_array();
	bool is_struct(ProgramStructure*ps);
	bool is_enum(ProgramStructure*ps);
	bool is_base_type();
	bool is_number();
	bool is_integer();
	bool is_real();
	bool is_bool();
	bool is_string();
	bool is_char();
	bool is_array_of_struct(ProgramStructure*ps);
	bool is_array_of_enum(ProgramStructure*ps);
	bool is_array_of_base_type();
	bool is_array_of_number();
	bool is_array_of_integer();
	bool is_array_of_real();
	bool is_array_of_bool();
	bool is_array_of_string();
	bool is_array_of_char();
	TypeDefinition &element_type();
};