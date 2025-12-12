#include <BuiltInGenerators/SqliteGenerator.hpp>

// Function to add foreign key columns for array relationships
void SqliteGenerator::add_foreign_key_columns_for_arrays(std::shared_ptr<ProgramStructure>ps)
{
	// Iterate through all structs
	for (auto &parent_struct : ps->getStructs())
	{
		// Look for array fields in this struct
		for (auto &member_var : parent_struct.getMemberVariables())
		{
			if (member_var.type.is_array())
			{
				// Get the element type of the array
				TypeDefinition element_type = member_var.type.element_type();

				// Check if the element type is a struct
				if (element_type.is_struct(ps))
				{
					// Find the target struct
					std::string target_struct_name = element_type.identifier();

					// Find the target struct in the program structure
					for (auto &target_struct : ps->getStructs())
					{
						if (target_struct.getIdentifier() == target_struct_name)
						{
							// Add foreign key column to the target struct
							MemberVariableDefinition reference_column;
							reference_column.identifier = parent_struct.getIdentifier() + "Id";
							reference_column.type = TypeDefinition("int64");
							reference_column.type.setRequired(member_var.type.is_required()); // If array is required, reference is NOT NULL
							reference_column.reference.struct_name = parent_struct.getIdentifier();
							reference_column.reference.variable_name = "id"; // Assuming parent has 'id' as primary key
							reference_column.description = "Foreign key reference to " + parent_struct.getIdentifier() + " table";

							// Check if this foreign key column already exists
							bool reference_exists = false;
							for (auto &existing_var : target_struct.getMemberVariables())
							{
								if (existing_var.identifier == reference_column.identifier)
								{
									reference_exists = true;
									break;
								}
							}

							// Only add if it doesn't already exist
							if (!reference_exists)
							{
								target_struct.add_member_variable(reference_column);
							}
							break;
						}
					}
				}
			}
		}
	}
}

std::string generate_bind(std::shared_ptr<Generator>gen, std::shared_ptr<ProgramStructure>ps, MemberVariableDefinition mv, int i)
{
	std::string ret = "\tsqlite3_bind_";
	if (mv.type.is_struct(ps))
	{
		return "";
	}
	else if (mv.type.is_enum(ps))
	{
		return "";
	}
	else if (mv.type.is_integer())
	{
		std::string local_type = gen->convert_to_local_type(ps, mv.type);
		local_type = local_type.substr(0, local_type.size() - 2);
		if (mv.type.is_required())
		{
			ret += "int(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ");\n";
		}
		else
		{
			ret += "int(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".has_value() ? " + mv.identifier + ".value() : 0);\n";
		}
	}
	else if (mv.type.is_real())
	{
		if (mv.type.is_required())
		{
			ret += "double(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ");\n";
		}
		else
		{
			ret += "double(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".has_value() ? " + mv.identifier + ".value() : 0.0);\n";
		}
	}
	else if (mv.type.is_bool())
	{
		if (mv.type.is_required())
		{
			ret += "int(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ");\n";
		}
		else
		{
			ret += "int(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".has_value() ? " + mv.identifier + ".value() : 0);\n";
		}
	}
	else if (mv.type.is_string())
	{
		if (mv.type.is_required())
		{
			ret += "text(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".c_str(), -1, SQLITE_STATIC);\n";
		}
		else
		{
			ret += "text(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".has_value() ? " + mv.identifier + ".value().c_str() : nullptr, -1, SQLITE_STATIC);\n";
		}
	}
	else if (mv.type.is_char())
	{
		if (mv.type.is_required())
		{
			ret += "text(stmt, " + std::to_string(i + 1) + ", std::string(1, " + mv.identifier + ").c_str(), -1, SQLITE_STATIC);\n";
		}
		else
		{
			ret += "text(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".has_value() ? std::string(1, " + mv.identifier + ".value()).c_str() : nullptr, -1, SQLITE_STATIC);\n";
		}
	}
	else if (mv.type.is_array())
	{
		if (mv.type.is_required())
		{
			ret += "text(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".c_str());\n";
		}
		else
		{
			ret += "text(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".has_value() ? " + mv.identifier + ".value().c_str() : nullptr);\n";
		}
	}
	else if (!mv.type.is_required())
	{
		throw std::runtime_error("Optional types are not supported in SQLite generator.");
	}
	else
	{
		ret += gen->convert_to_local_type(ps, mv.type) + "(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ");\n";
	}
	return ret;
}

std::string SqliteGenerator::escape_string(std::string str)
{
	// escape quotes using regex
	std::regex quote_regex("\"");
	std::string escaped = std::regex_replace(str, quote_regex, "\\\"");
	// escape new lines
	std::regex newline_regex("\n");
	escaped = std::regex_replace(escaped, newline_regex, "\\n");

	return escaped;
}

SqliteGenerator::SqliteGenerator()
{
	name = "SQLite";
	enabled = false;
	// base_class.getIdentifier() = "Sqlite";
	// base_class.add_include("<sqlite3.h>");
	// base_class.add_include("<iostream>");
	// base_class.add_include("<string>");
	// base_class.add_include("<vector>");
}

std::string SqliteGenerator::convert_to_local_type(std::shared_ptr<ProgramStructure>ps, TypeDefinition type)
{
	// convert int types to "INTEGER"
	if (type.is_struct(ps))
	{
		return "INTEGER";
	}

	if (type.is_enum(ps))
	{
		return "INTEGER";
	}

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
	// convert array to foreign key relationship - arrays don't create columns in the parent table
	if (type.is_array())
	{
		// Arrays are handled by adding foreign key columns to the child table
		// No column is created in the parent table for arrays
		return ""; // Return empty string to indicate no column should be created
	}
	return type.identifier();
}

bool SqliteGenerator::add_generator_specific_content_to_struct(std::shared_ptr<Generator>gen, std::shared_ptr<ProgramStructure>ps, StructDefinition &s)
{
	return true;
}

#include <inja/inja.hpp>
#include <SchemaLangShared_Resources/SchemaLangShared_ResourcesEmbeddedVFS.hpp>

bool SqliteGenerator::generate_files(std::shared_ptr<ProgramStructure> ps, std::string out_path)
{
	if (!std::filesystem::exists(out_path))
	{
		std::filesystem::create_directories(out_path);
	}

	// Add foreign key columns for array relationships before generating files
	add_foreign_key_columns_for_arrays(ps);

	// for (auto &s : ps.getStructs())
	// {
	// 	if (!generate_struct_files(&ps, s, out_path))
	// 	{
	// 		PLOGE << "Failed to generate sqlite file for struct: " << s.getIdentifier() << std::endl;
	// 		return false;
	// 	}
	// }

	inja::Environment env= getEnv(shared_from_this(), ps);
	env.set_trim_blocks(true);
	env.set_lstrip_blocks(true);

	std::map<std::string, std::string> struct_name_content_pairs;
	// open file
	std::vector<std::string> files = listSchemaLangShared_ResourcesEmbeddedFiles("/SQLite/struct/");
	for (auto &file : files)
	{
		std::string content(reinterpret_cast<const char *>(loadSchemaLangShared_ResourcesEmbeddedFile(("/SQLite/struct/" + file).c_str()).data()), loadSchemaLangShared_ResourcesEmbeddedFile(("/SQLite/struct/" + file).c_str()).size());
		std::string filename = std::filesystem::path(file).filename().string();
		struct_name_content_pairs[filename] = content;
	}

	std::map<std::string, std::string> enum_name_content_pairs;
	files = listSchemaLangShared_ResourcesEmbeddedFiles("/SQLite/enum/");
	for (auto &file : files)
	{
		std::string content(reinterpret_cast<const char *>(loadSchemaLangShared_ResourcesEmbeddedFile(("/SQLite/enum/" + file).c_str()).data()), loadSchemaLangShared_ResourcesEmbeddedFile(("/SQLite/enum/" + file).c_str()).size());
		std::string filename = std::filesystem::path(file).filename().string();
		enum_name_content_pairs[filename] = content;
	}

	for (auto &s : ps->getStructs())
	{
		inja::json data = ps->to_json(shared_from_this());
		inja::json struct_data=s.to_json(ps, shared_from_this());
		for (auto& [key, value] : struct_data.items()) {
		    data[key] = value;
		}

		try
		{
			for (auto &file : struct_name_content_pairs)
			{
				std::ofstream of(env.render(out_path + "/" + file.first, data));
				if (!of.is_open())
				{
					PLOGE << "Failed to open file: " << env.render(out_path + "/" + file.first, data);
				}
				of << env.render(file.second, data);
				of.close();
				PLOGI << "Generated file: " << env.render(out_path + "/" + file.first, data) << std::endl;
			}
		}
		catch (const std::exception &e)
		{
			PLOGE << "Error generating file for struct " << s.getIdentifier() << ": " << e.what() << std::endl;
			return false;
		}
	}

	for (auto &e : ps->getEnums())
	{
		inja::json data=e.to_json(ps,shared_from_this());

		try
		{
			for (auto &file : enum_name_content_pairs)
			{
				std::ofstream of(env.render(out_path + "/" + file.first, data));
				if (!of.is_open())
				{
					PLOGE << "Failed to open file: " << env.render(out_path + "/" + file.first, data);
				}
				of << env.render(file.second, data);
				of.close();
				PLOGI << "Generated file: " << env.render(out_path + "/" + file.first, data) << std::endl;
			}
		}
		catch (const std::exception &ex)
		{
			PLOGE << "Error generating file for enum " << e.identifier << ": " << ex.what() << std::endl;
			return false;
		}
	}

	// Generate schema version tracking table
	try
	{
		std::string version_table_content(
			reinterpret_cast<const char *>(loadSchemaLangShared_ResourcesEmbeddedFile("/SQLite/_schema_versions_create_table.sql").data()),
			loadSchemaLangShared_ResourcesEmbeddedFile("/SQLite/_schema_versions_create_table.sql").size()
		);
		
		std::ofstream version_file(out_path + "/_schema_versions_create_table.sql");
		if (!version_file.is_open())
		{
			PLOGE << "Failed to create version tracking table file";
			return false;
		}
		version_file << version_table_content;
		version_file.close();
		PLOGI << "Generated version tracking table: " << out_path << "/_schema_versions_create_table.sql" << std::endl;
	}
	catch (const std::exception &ex)
	{
		PLOGE << "Error generating version tracking table: " << ex.what() << std::endl;
		return false;
	}

	return true;
}

bool SqliteGenerator::generate_migration_files(std::shared_ptr<ProgramStructure> ps, std::string out_path)
{
	// Migration SQL is now embedded in the generated C++ code via template
	// The migration functions are generated as part of SQLiteDB.cpp
	// No separate SQL files are needed
	PLOGI << "Migrations will be embedded in generated C++ code" << std::endl;
	return true;
}