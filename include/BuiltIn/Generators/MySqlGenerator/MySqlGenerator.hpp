#pragma once
#include <ForwardDeclerations.hpp>
#include <ProgramStructure/ProgramStructure.hpp>
#include <Generator/Generator.hpp>

class MysqlGenerator : public Generator
{

	bool add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
	{
		return true;
	}

	std::string convert_to_local_type(ProgramStructure *ps, TypeDefinition type)
	{
		// convert int types to "INTEGER"
		if (type.is_integer())
		{
			return "INTEGER";
		}
		// convert float types to "REAL"
		if (type.is_real())
		{
			return "REAL";
		}
		// convert bool to "BOOLEAN"
		if (type.is_bool())
		{
			return "BOOLEAN";
		}
		// convert string to "TEXT"
		if (type.is_string())
		{
			return "TEXT";
		}
		// convert char to "CHAR"
		if (type.is_char())
		{
			return "CHAR";
		}
		// convert array to "TEXT"
		if (type.is_array())
		{
			return "TEXT";
		}
		return type.identifier();
	}

	bool generate_files(ProgramStructure ps, std::string out_path)
	{
		if (!std::filesystem::exists(out_path))
		{
			std::filesystem::create_directories(out_path);
		}
		return true;
	}
};