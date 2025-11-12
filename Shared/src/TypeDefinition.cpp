#include <TypeDefinition.hpp>
#include <ProgramStructure.hpp>

TypeDefinition::TypeDefinition()
{
	ident = "";
	elem_type = nullptr;
}

TypeDefinition::TypeDefinition(std::string ident)
{
	this->ident = ident;
	this->elem_type = nullptr;
}

TypeDefinition::TypeDefinition(std::string ident, bool defaulted)
{
	this->ident = ident;
	this->elem_type = nullptr;
	this->defaulted = defaulted;
}

TypeDefinition::TypeDefinition(std::string ident, TypeDefinition elem_type)
{
	this->ident = ident;
	this->elem_type = new TypeDefinition(elem_type);
}

std::string &TypeDefinition::identifier()
{
	return ident;
}

bool TypeDefinition::is_array()
{
	return ident == ARRAY;
}

bool TypeDefinition::is_struct(ProgramStructure *ps)
{
	return ps->tokenIsStruct(ident);
}

bool TypeDefinition::is_enum(ProgramStructure *ps)
{
	return ps->tokenIsEnum(ident);
}

bool TypeDefinition::is_base_type()
{
	bool is_base = false;
	if (ident == VOID)
	{
		is_base=true;
	}
	else if (ident == INT8)
	{
		is_base=true;
	}
	else if (ident == INT16)
	{
		is_base=true;
	}
	else if (ident == INT32)
	{
		is_base=true;
	}
	else if (ident == INT64)
	{
		is_base=true;
	}
	else if (ident == UINT8)
	{
		is_base=true;
	}
	else if (ident == UINT16)
	{
		is_base=true;
	}
	else if (ident == UINT32)
	{
		is_base=true;
	}
	else if (ident == UINT64)
	{
		is_base=true;
	}
	else if (ident == FLOAT)
	{
		is_base=true;
	}
	else if (ident == DOUBLE)
	{
		is_base=true;
	}
	else if (ident == BOOL)
	{
		is_base=true;
	}
	else if (ident == STRING)
	{
		is_base=true;
	}
	else if (ident == CHAR)
	{
		is_base=true;
	}
	else if (ident == UCHAR)
	{
		is_base=true;
	}
	else if (ident == POINTER)
	{
		is_base=true;
	}
	else if (ident == ARRAY)
	{
		is_base=true;
	}
	return is_base;
}

bool TypeDefinition::is_number()
{
	bool is_number = false;
	if (ident == INT8)
	{
		is_number=true;
	}
	else if (ident == INT16)
	{
		is_number=true;
	}
	else if (ident == INT32)
	{
		is_number=true;
	}
	else if (ident == INT64)
	{
		is_number=true;
	}
	else if (ident == UINT8)
	{
		is_number=true;
	}
	else if (ident == UINT16)
	{
		is_number=true;
	}
	else if (ident == UINT32)
	{
		is_number=true;
	}
	else if (ident == UINT64)
	{
		is_number=true;
	}
	else if (ident == FLOAT)
	{
		is_number=true;
	}
	else if (ident == DOUBLE)
	{
		is_number=true;
	}
	return is_number;
}

bool TypeDefinition::is_integer()
{
	bool is_integer = false;
	if (ident == INT8)
	{
		is_integer=true;
	}
	else if (ident == INT16)
	{
		is_integer=true;
	}
	else if (ident == INT32)
	{
		is_integer=true;
	}
	else if (ident == INT64)
	{
		is_integer=true;
	}
	else if (ident == UINT8)
	{
		is_integer=true;
	}
	else if (ident == UINT16)
	{
		is_integer=true;
	}
	else if (ident == UINT32)
	{
		is_integer=true;
	}
	else if (ident == UINT64)
	{
		is_integer=true;
	}
	return is_integer;
}

bool TypeDefinition::is_real()
{
	bool is_real = false;
	if (ident == FLOAT)
	{
		is_real=true;
	}
	else if (ident == DOUBLE)
	{
		is_real=true;
	}
	return is_real;
}

bool TypeDefinition::is_bool()
{
	return ident == BOOL;
}

bool TypeDefinition::is_string()
{
	return ident == STRING;
}

bool TypeDefinition::is_char()
{
	return ident == CHAR;
}

bool TypeDefinition::is_array_of_struct(ProgramStructure *ps)
{
	return elem_type != nullptr && elem_type->is_struct(ps);
}

bool TypeDefinition::is_array_of_enum(ProgramStructure *ps)
{
	return elem_type != nullptr && elem_type->is_enum(ps);
}

bool TypeDefinition::is_array_of_base_type()
{
	return elem_type != nullptr && elem_type->is_base_type();
}

bool TypeDefinition::is_array_of_number()
{
	return elem_type != nullptr && elem_type->is_number();
}

bool TypeDefinition::is_array_of_integer()
{
	return elem_type != nullptr && elem_type->is_integer();
}

bool TypeDefinition::is_array_of_real()
{
	return elem_type != nullptr && elem_type->is_real();
}

bool TypeDefinition::is_array_of_bool()
{
	return elem_type != nullptr && elem_type->is_bool();
}

bool TypeDefinition::is_array_of_string()
{
	return elem_type != nullptr && elem_type->is_string();
}

bool TypeDefinition::is_array_of_char()
{
	return elem_type != nullptr && elem_type->is_char();
}

bool TypeDefinition::is_optional()
{
	return ident.find("std::optional<") == 0;
}

TypeDefinition &TypeDefinition::element_type()
{
	if (elem_type == nullptr)
	{
		elem_type = new TypeDefinition();
	}
	return *elem_type;
}

inja::json TypeDefinition::to_json(ProgramStructure*ps, Generator* generator)
{
    inja::json j;
    j["identifier"] = ident;
	std::string identifierCamel = ident;
	identifierCamel[0] = toupper(identifierCamel[0]);
	j["identifierCamel"] = identifierCamel;
    j["defaulted"] = defaulted;
    if (elem_type != nullptr)
    {
        j["elem_type"] = elem_type->to_json(ps,generator);
    }
    else
    {
        j["elem_type"] = nullptr;
    }
	j["estimated"]=generator->convert_to_local_type(ps,*this);
	if(generator->generators.size()>0){
		for(auto gen:generator->generators){
			j[gen->name+"_estimated"]= gen->convert_to_local_type(ps,*this);
		}
	}

    j["is_array"] = is_array();
	j["is_struct"]=is_struct(ps);
	j["is_enum"]=is_enum(ps);
	j["is_base_type"]=is_base_type();
	j["is_number"]=is_number();
	j["is_integer"]=is_integer();
	j["is_real"]=is_real();
	j["is_bool"]=is_bool();
	j["is_string"]=is_string();
	j["is_char"]=is_char();
	j["is_array_of_struct"]=is_array_of_struct(ps);
	j["is_array_of_enum"]=is_array_of_enum(ps);
	j["is_array_of_base_type"]=is_array_of_base_type();
	j["is_array_of_number"]=is_array_of_number();
	j["is_array_of_integer"]=is_array_of_integer();
	j["is_array_of_real"]=is_array_of_real();
	j["is_array_of_bool"]=is_array_of_bool();
	j["is_array_of_string"]=is_array_of_string();
	j["is_array_of_char"]=is_array_of_char();
	j["is_optional"]=is_optional();

    return j;
}

void TypeDefinition::from_json(inja::json j)
{
	ident = j["identifier"].get<std::string>();
	defaulted = j["defaulted"].get<bool>();
	if (!j["elem_type"].is_null())
	{
		elem_type = new TypeDefinition();
		elem_type->from_json(j["elem_type"]);
	}
	else
	{
		elem_type = nullptr;
	}
}
