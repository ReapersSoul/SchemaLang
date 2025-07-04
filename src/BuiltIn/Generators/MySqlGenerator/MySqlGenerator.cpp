#include <BuiltIn/Generators/MySqlGenerator/MySqlGenerator.hpp>

// utility functions
std::vector<std::vector<int>> MysqlGenerator::comb(int N, int K)
{
	std::string bitmask(K, 1); // K leading 1's
	bitmask.resize(N, 0);	   // N-K trailing 0's

	// print integers and permute bitmask
	std::vector<std::vector<int>> combinations;
	do
	{
		std::vector<int> combination;
		for (int i = 0; i < N; ++i) // [0..N-1] integers
		{
			if (bitmask[i])
			{
				combination.push_back(i);
			}
		}
		combinations.push_back(combination);
	} while (std::prev_permutation(bitmask.begin(), bitmask.end()));
	return combinations;
}

std::vector<std::vector<int>> MysqlGenerator::comb(int N)
{
	std::vector<std::vector<int>> combinations;
	for (int i = 1; i < N + 1; i++)
	{
		std::vector<std::vector<int>> temp = comb(N, i);
		combinations.insert(combinations.end(), temp.begin(), temp.end());
	}
	return combinations;
}

// sql string generation functions
std::string MysqlGenerator::generate_create_table_statement_string_struct(ProgramStructure *ps, StructDefinition &s)
{
	std::string sql = "CREATE TABLE IF NOT EXISTS " + s.identifier + " (\n";
	bool first_column = true;
	std::vector<std::string> foreign_keys; // Collect foreign key constraints
	
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		// Skip array fields - they don't create columns in the parent table
		if (s.member_variables[i].type.is_array())
		{
			continue;
		}
		
		if (!first_column)
		{
			sql += ",\n";
		}
		first_column = false;
		
		sql += "  " + s.member_variables[i].identifier + " " + convert_to_local_type(ps, s.member_variables[i].type);
		
		// Add MySQL-specific constraints
		if (s.member_variables[i].primary_key)
		{
			sql += " PRIMARY KEY";
		}
		if (s.member_variables[i].auto_increment)
		{
			sql += " AUTO_INCREMENT";
		}
		if (s.member_variables[i].required)
		{
			sql += " NOT NULL";
		}
		if (s.member_variables[i].unique)
		{
			sql += " UNIQUE";
		}
		if (!s.member_variables[i].default_value.empty())
		{
			sql += " DEFAULT '" + s.member_variables[i].default_value + "'";
		}
		
		// Collect foreign key constraints to add later
		if (!s.member_variables[i].fk.variable_name.empty())
		{
			std::string fk_constraint = "  FOREIGN KEY (" + s.member_variables[i].identifier + ") REFERENCES " + 
				s.member_variables[i].fk.struct_name + "(" + s.member_variables[i].fk.variable_name + ")";
			foreign_keys.push_back(fk_constraint);
		}
	}
	
	// Add foreign key constraints
	for (const auto& fk : foreign_keys)
	{
		sql += ",\n" + fk;
	}
	
	sql += "\n);";
	return sql;
}

// Function to add foreign key columns for array relationships
void MysqlGenerator::add_foreign_key_columns_for_arrays(ProgramStructure *ps)
{
	// Iterate through all structs
	for (auto &parent_struct : ps->getStructs())
	{
		// Look for array fields in this struct
		for (auto &member_var : parent_struct.member_variables)
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
						if (target_struct.identifier == target_struct_name)
						{
							// Add foreign key column to the target struct
							MemberVariableDefinition fk_column;
							fk_column.identifier = parent_struct.identifier + "Id";
							fk_column.type = TypeDefinition("int64");
							fk_column.required = member_var.required; // If array is required, FK is NOT NULL
							fk_column.fk.struct_name = parent_struct.identifier;
							fk_column.fk.variable_name = "id"; // Assuming parent has 'id' as primary key
							fk_column.description = "Foreign key reference to " + parent_struct.identifier + " table";
							
							// Check if this foreign key column already exists
							bool fk_exists = false;
							for (auto &existing_var : target_struct.member_variables)
							{
								if (existing_var.identifier == fk_column.identifier)
								{
									fk_exists = true;
									break;
								}
							}
							
							// Only add if it doesn't already exist
							if (!fk_exists)
							{
								target_struct.member_variables.push_back(fk_column);
							}
							break;
						}
					}
				}
			}
		}
	}
}

std::string MysqlGenerator::generate_select_all_statement_string_member_variable(StructDefinition &s, MemberVariableDefinition &mv)
{
	std::string sql = "SELECT * FROM " + s.identifier + " WHERE " + mv.identifier + " = ?;";
	return sql;
}

std::vector<std::string> MysqlGenerator::generate_select_all_statements_string_struct(StructDefinition &s)
{
	std::vector<std::string> sqls;
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		sqls.push_back(generate_select_all_statement_string_member_variable(s, s.member_variables[i]));
	}
	return sqls;
}

std::string MysqlGenerator::generate_select_by_member_variable_statement_string(StructDefinition &s, MemberVariableDefinition &mv_1, std::vector<int> &criteria)
{
	std::string sql = "SELECT " + mv_1.identifier + " FROM " + s.identifier + " WHERE ";
	for (int i = 0; i < criteria.size(); i++)
	{
		sql += s.member_variables[criteria[i]].identifier + " = ?";
		if (i < criteria.size() - 1)
		{
			sql += " AND ";
		}
	}
	sql += ";";
	return sql;
}

std::string MysqlGenerator::generate_insert_statement_string_struct(StructDefinition &s)
{
	std::string sql = "INSERT INTO " + s.identifier + " (";
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		sql += s.member_variables[i].identifier;
		if (i < s.member_variables.size() - 1)
		{
			sql += ", ";
		}
	}
	sql += ") VALUES (";
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		sql += "?";
		if (i < s.member_variables.size() - 1)
		{
			sql += ", ";
		}
	}
	sql += ");";
	return sql;
}

std::string MysqlGenerator::generate_update_all_statement_string_struct(StructDefinition &s)
{
	std::string sql = "UPDATE " + s.identifier + " SET ";
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		if (s.member_variables[i].primary_key)
		{
			continue;
		}
		sql += s.member_variables[i].identifier + " = ?";
		if (i < s.member_variables.size() - 1)
		{
			sql += ", ";
		}
	}
	sql += " WHERE ";
	bool has_primary_key = false;
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		if (s.member_variables[i].primary_key)
		{
			sql += s.member_variables[i].identifier + " = ?";
			has_primary_key = true;
			break;
		}
	}
	if (!has_primary_key)
	{
		// Use the first column as identifier if no primary key
		sql += s.member_variables[0].identifier + " = ?";
	}
	sql += ";";
	return sql;
}

// functions for c++ code generation
void MysqlGenerator::generate_select_all_statement_function_member_variable(Generator *gen, ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv)
{
	if (mv.type.is_array())
	{
		return; // Skip arrays for simple select operations
	}
	if (mv.type.is_struct(ps))
	{
		return; // Skip structs for simple select operations
	}
	
	FunctionDefinition select_all_statement;
	select_all_statement.identifier = "MySQLSelectBy" + mv.identifier;
	select_all_statement.return_type.identifier() = "std::vector<" + s.identifier + "Schema*>";
	select_all_statement.static_function = true;
	select_all_statement.parameters.push_back(std::make_pair(mysql_session, "session"));
	select_all_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));

	select_all_statement.generate_function = [this, &mv](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
	{
		std::string sql = generate_select_all_statement_string_member_variable(s, mv);
		structFile << "\tstd::vector<" << s.identifier << "Schema*> results;\n";
		structFile << "\ttry {\n";
		structFile << "\t\tmysqlx::Schema db = session.getSchema(\"" << s.identifier << "_db\");\n";
		structFile << "\t\tmysqlx::Table table = db.getTable(\"" << s.identifier << "\");\n";
		structFile << "\t\tmysqlx::RowResult result = table.select(\"*\")\n";
		structFile << "\t\t\t.where(\"" << mv.identifier << " = :param\")\n";
		structFile << "\t\t\t.bind(\"param\", " << mv.identifier << ")\n";
		structFile << "\t\t\t.execute();\n";
		structFile << "\t\tfor (auto row : result) {\n";
		structFile << "\t\t\t" << s.identifier << "Schema *obj = new " << s.identifier << "Schema();\n";
		structFile << "\t\t\t// Populate object from row data\n";
		structFile << "\t\t\tresults.push_back(obj);\n";
		structFile << "\t\t}\n";
		structFile << "\t} catch (const mysqlx::Error &err) {\n";
		structFile << "\t\tstd::cerr << \"MySQL Error: \" << err.what() << std::endl;\n";
		structFile << "\t}\n";
		structFile << "\treturn results;\n";
		return true;
	};
	s.functions.push_back(select_all_statement);
}

void MysqlGenerator::generate_select_all_statement_functions_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		generate_select_all_statement_function_member_variable(gen, ps, s, s.member_variables[i]);
	}
}

void MysqlGenerator::generate_select_member_variable_function_statement(Generator *gen, ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv_1, std::vector<int> &criteria)
{
	FunctionDefinition select_statement;
	select_statement.identifier = "MySQLSelect" + s.identifier + "By";
	select_statement.return_type = mv_1.type;
	select_statement.static_function = true;
	select_statement.parameters.push_back(std::make_pair(mysql_session, "session"));
	for (int i = 0; i < criteria.size(); i++)
	{
		select_statement.identifier += s.member_variables[criteria[i]].identifier;
		select_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, s.member_variables[criteria[i]].type), s.member_variables[criteria[i]].identifier));
	}
	select_statement.generate_function = [this, &mv_1, &criteria](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
	{
		structFile << "\ttry {\n";
		structFile << "\t\tmysqlx::Schema db = session.getSchema(\"" << s.identifier << "_db\");\n";
		structFile << "\t\tmysqlx::Table table = db.getTable(\"" << s.identifier << "\");\n";
		structFile << "\t\tstd::string where_clause = \"\";\n";
		
		for (int i = 0; i < criteria.size(); i++)
		{
			structFile << "\t\tif (i > 0) where_clause += \" AND \";\n";
			structFile << "\t\twhere_clause += \"" << s.member_variables[criteria[i]].identifier << " = :param" << i << "\";\n";
		}
		
		structFile << "\t\tmysqlx::RowResult result = table.select(\"" << mv_1.identifier << "\")\n";
		structFile << "\t\t\t.where(where_clause)\n";
		
		for (int i = 0; i < criteria.size(); i++)
		{
			structFile << "\t\t\t.bind(\"param" << i << "\", " << s.member_variables[criteria[i]].identifier << ")\n";
		}
		
		structFile << "\t\t\t.execute();\n";
		structFile << "\t\t// Process result and return appropriate value\n";
		structFile << "\t\treturn " << gen->convert_to_local_type(ps, mv_1.type) << "();\n";
		structFile << "\t} catch (const mysqlx::Error &err) {\n";
		structFile << "\t\tstd::cerr << \"MySQL Error: \" << err.what() << std::endl;\n";
		structFile << "\t\treturn " << gen->convert_to_local_type(ps, mv_1.type) << "();\n";
		structFile << "\t}\n";
		return true;
	};
	s.functions.push_back(select_statement);
}

void MysqlGenerator::generate_select_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	generate_select_all_statement_functions_struct(gen, ps, s);
	// Additional select combinations can be generated here
}

void MysqlGenerator::generate_insert_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	FunctionDefinition insert_statement;
	insert_statement.identifier = "MySQLInsert";
	insert_statement.return_type.identifier() = "bool";
	insert_statement.static_function = true;
	insert_statement.parameters.push_back(std::make_pair(mysql_session, "session"));
	for (auto &mv : s.member_variables)
	{
		insert_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));
	}
	insert_statement.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
	{
		structFile << "\ttry {\n";
		structFile << "\t\tmysqlx::Schema db = session.getSchema(\"" << s.identifier << "_db\");\n";
		structFile << "\t\tmysqlx::Table table = db.getTable(\"" << s.identifier << "\");\n";
		structFile << "\t\ttable.insert(";
		for (int i = 0; i < s.member_variables.size(); i++)
		{
			structFile << "\"" << s.member_variables[i].identifier << "\"";
			if (i < s.member_variables.size() - 1)
			{
				structFile << ", ";
			}
		}
		structFile << ")\n";
		structFile << "\t\t\t.values(";
		for (int i = 0; i < s.member_variables.size(); i++)
		{
			structFile << s.member_variables[i].identifier;
			if (i < s.member_variables.size() - 1)
			{
				structFile << ", ";
			}
		}
		structFile << ")\n";
		structFile << "\t\t\t.execute();\n";
		structFile << "\t\treturn true;\n";
		structFile << "\t} catch (const mysqlx::Error &err) {\n";
		structFile << "\t\tstd::cerr << \"MySQL Error: \" << err.what() << std::endl;\n";
		structFile << "\t\treturn false;\n";
		structFile << "\t}\n";
		return true;
	};
	s.functions.push_back(insert_statement);

	FunctionDefinition insert_statement_no_args;
	insert_statement_no_args.identifier = "MySQLInsert";
	insert_statement_no_args.return_type.identifier() = "bool";
	insert_statement_no_args.static_function = false;
	insert_statement_no_args.parameters.push_back(std::make_pair(mysql_session, "session"));
	insert_statement_no_args.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
	{
		structFile << "\ttry {\n";
		structFile << "\t\tmysqlx::Schema db = session.getSchema(\"" << s.identifier << "_db\");\n";
		structFile << "\t\tmysqlx::Table table = db.getTable(\"" << s.identifier << "\");\n";
		structFile << "\t\ttable.insert(";
		for (int i = 0; i < s.member_variables.size(); i++)
		{
			structFile << "\"" << s.member_variables[i].identifier << "\"";
			if (i < s.member_variables.size() - 1)
			{
				structFile << ", ";
			}
		}
		structFile << ")\n";
		structFile << "\t\t\t.values(";
		for (int i = 0; i < s.member_variables.size(); i++)
		{
			structFile << "this->" << s.member_variables[i].identifier;
			if (i < s.member_variables.size() - 1)
			{
				structFile << ", ";
			}
		}
		structFile << ")\n";
		structFile << "\t\t\t.execute();\n";
		structFile << "\t\treturn true;\n";
		structFile << "\t} catch (const mysqlx::Error &err) {\n";
		structFile << "\t\tstd::cerr << \"MySQL Error: \" << err.what() << std::endl;\n";
		structFile << "\t\treturn false;\n";
		structFile << "\t}\n";
		return true;
	};
	s.functions.push_back(insert_statement_no_args);
}

void MysqlGenerator::generate_update_all_statement_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	FunctionDefinition update_all_statement;
	update_all_statement.identifier = "MySQLUpdate" + s.identifier;
	update_all_statement.return_type.identifier() = "bool";
	update_all_statement.static_function = true;
	update_all_statement.parameters.push_back(std::make_pair(mysql_session, "session"));
	for (auto &mv : s.member_variables)
	{
		update_all_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));
	}
	update_all_statement.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
	{
		structFile << "\ttry {\n";
		structFile << "\t\tmysqlx::Schema db = session.getSchema(\"" << s.identifier << "_db\");\n";
		structFile << "\t\tmysqlx::Table table = db.getTable(\"" << s.identifier << "\");\n";
		
		// Find primary key for WHERE clause
		bool has_primary_key = false;
		std::string primary_key_field;
		for (auto &mv : s.member_variables)
		{
			if (mv.primary_key)
			{
				has_primary_key = true;
				primary_key_field = mv.identifier;
				break;
			}
		}
		
		structFile << "\t\tmysqlx::TableUpdate update = table.update();\n";
		for (auto &mv : s.member_variables)
		{
			if (!mv.primary_key)
			{
				structFile << "\t\tupdate.set(\"" << mv.identifier << "\", " << mv.identifier << ");\n";
			}
		}
		
		if (has_primary_key)
		{
			structFile << "\t\tupdate.where(\"" << primary_key_field << " = :pk\").bind(\"pk\", " << primary_key_field << ");\n";
		}
		else
		{
			structFile << "\t\tupdate.where(\"" << s.member_variables[0].identifier << " = :pk\").bind(\"pk\", " << s.member_variables[0].identifier << ");\n";
		}
		
		structFile << "\t\tupdate.execute();\n";
		structFile << "\t\treturn true;\n";
		structFile << "\t} catch (const mysqlx::Error &err) {\n";
		structFile << "\t\tstd::cerr << \"MySQL Error: \" << err.what() << std::endl;\n";
		structFile << "\t\treturn false;\n";
		structFile << "\t}\n";
		return true;
	};
	s.functions.push_back(update_all_statement);
}

void MysqlGenerator::generate_update_statements_function_struct()
{
	// Implementation for additional update statements
}

void MysqlGenerator::generate_delete_statement_function_struct()
{
	// Implementation for delete statements
}

// file generation functions
bool MysqlGenerator::generate_create_table_file(ProgramStructure *ps, StructDefinition &s, std::string out_path)
{
	std::ofstream structFile(out_path + "/" + s.identifier + "_create_table.sql");
	if (!structFile.is_open())
	{
		std::cout << "Failed to open file: " << out_path + "/" + s.identifier + "_create_table.sql" << std::endl;
		return false;
	}
	structFile << generate_create_table_statement_string_struct(ps, s) << std::endl;
	structFile.close();
	return true;
}

bool MysqlGenerator::generate_select_all_files(ProgramStructure *ps, StructDefinition &s, std::string out_path)
{
	std::vector<std::string> sqls = generate_select_all_statements_string_struct(s);
	for (int i = 0; i < sqls.size(); i++)
	{
		std::ofstream structFile(out_path + "/" + s.identifier + "_select_by_" + s.member_variables[i].identifier + ".sql");
		if (!structFile.is_open())
		{
			std::cout << "Failed to open file: " << out_path + "/" + s.identifier + "_select_by_" + s.member_variables[i].identifier + ".sql" << std::endl;
			return false;
		}
		structFile << sqls[i] << std::endl;
		structFile.close();
	}
	return true;
}

bool MysqlGenerator::generate_select_files(ProgramStructure *ps, StructDefinition &s, std::string out_path)
{
	std::vector<std::vector<int>> combinations = comb(s.member_variables.size());
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		if (s.member_variables[i].primary_key)
		{
			continue;
		}
		for (int j = 0; j < combinations.size(); j++)
		{
			std::string filename = out_path + "/" + s.identifier + "_select_" + s.member_variables[i].identifier + "_by";
			std::string sql = generate_select_by_member_variable_statement_string(s, s.member_variables[i], combinations[j]);
			for (int k = 0; k < combinations[j].size(); k++)
			{
				filename += "_" + s.member_variables[combinations[j][k]].identifier;
			}
			filename += ".sql";
			std::ofstream structFile(filename);
			if (!structFile.is_open())
			{
				std::cout << "Failed to open file: " << filename << std::endl;
				return false;
			}
			structFile << sql << std::endl;
			structFile.close();
		}
	}
	return true;
}

bool MysqlGenerator::generate_struct_files(ProgramStructure *ps, StructDefinition &s, std::string out_path)
{
	if (!std::filesystem::exists(out_path))
	{
		std::filesystem::create_directories(out_path);
	}

	if (!generate_create_table_file(ps, s, out_path))
	{
		return false;
	}


	if(gen_select_all_files){
		if (!generate_select_all_files(ps, s, out_path))
		{
			return false;
		}
	}

	if(gen_select_files){
		if (!generate_select_files(ps, s, out_path))
		{
			return false;
		}
	}

	if (gen_insert_files)
	{
		// if (!generate_insert_statement_files(ps, s, out_path))
		// {
		// 	return false;
		// }
	}

	if (gen_update_files)
	{
		// if (!generate_update_all_statement_files(ps, s, out_path))
		// {
		// 	return false;
		// }
	}

	if (gen_delete_files)
	{
		// if (!generate_delete_statement_files(ps, s, out_path))
		// {
		// 	return false;
		// }
	}

	return true;
}

std::string MysqlGenerator::escape_string(std::string str)
{
	// escape quotes using regex
	std::regex quote_regex("\"");
	std::string escaped = std::regex_replace(str, quote_regex, "\\\"");
	// escape new lines and other special characters for MySQL
	std::regex newline_regex("\n");
	escaped = std::regex_replace(escaped, newline_regex, "\\n");
	std::regex carriage_return_regex("\r");
	escaped = std::regex_replace(escaped, carriage_return_regex, "\\r");
	std::regex tab_regex("\t");
	escaped = std::regex_replace(escaped, tab_regex, "\\t");
	std::regex backslash_regex("\\\\");
	escaped = std::regex_replace(escaped, backslash_regex, "\\\\");
	return escaped;
}

MysqlGenerator::MysqlGenerator()
{
	// base_class.identifier = "MySQL";
	// base_class.includes.push_back("<mysqlx/xdevapi.h>");
	// base_class.includes.push_back("<string>");
	// base_class.includes.push_back("<vector>");
	// base_class.includes.push_back("<regex>");
	// base_class.includes.push_back("<filesystem>");
	// base_class.includes.push_back("<fstream>");
	// base_class.includes.push_back("<iostream>");
	
	// base_class.before_lines.push_back("using namespace mysqlx;");
}

std::string MysqlGenerator::convert_to_local_type(ProgramStructure *ps, TypeDefinition type)
{
	// MySQL-specific type conversions
	if (type.identifier() == INT8)
	{
		return "TINYINT";
	}
	if (type.identifier() == INT16)
	{
		return "SMALLINT";
	}
	if (type.identifier() == INT32)
	{
		return "INT";
	}
	if (type.identifier() == INT64)
	{
		return "BIGINT";
	}
	if (type.identifier() == UINT8)
	{
		return "TINYINT UNSIGNED";
	}
	if (type.identifier() == UINT16)
	{
		return "SMALLINT UNSIGNED";
	}
	if (type.identifier() == UINT32)
	{
		return "INT UNSIGNED";
	}
	if (type.identifier() == UINT64)
	{
		return "BIGINT UNSIGNED";
	}
	if (type.identifier() == FLOAT)
	{
		return "FLOAT";
	}
	if (type.identifier() == DOUBLE)
	{
		return "DOUBLE";
	}
	if (type.identifier() == BOOL)
	{
		return "BOOLEAN";
	}
	if (type.identifier() == STRING)
	{
		return "VARCHAR(255)";
	}
	if (type.identifier() == CHAR)
	{
		return "CHAR(1)";
	}
	if (type.identifier() == ARRAY)
	{
		// Arrays are handled by adding foreign key columns to the child table
		// No column is created in the parent table for arrays
		return ""; // Return empty string to indicate no column should be created
	}

	// if the type is a enum, return VARCHAR
	if (ps->tokenIsEnum(type.identifier()))
	{
		return "VARCHAR(50)";
	}

	// if the type is a struct, return JSON
	if (ps->tokenIsStruct(type.identifier()))
	{
		return "JSON";
	}

	return type.identifier();
}

bool MysqlGenerator::add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	// Add necessary includes for MySQL X DevAPI
	s.includes.push_back("<mysqlx/xdevapi.h>");
	s.includes.push_back("<iostream>");
	s.includes.push_back("<string>");
	s.includes.push_back("<vector>");

	generate_select_statements_function_struct(gen, ps, s);
	generate_insert_statements_function_struct(gen, ps, s);
	generate_update_all_statement_function_struct(gen, ps, s);

	// Add MySQL create table functions
	FunctionDefinition getMySQLCreateTableStatement;
	getMySQLCreateTableStatement.identifier = "getMySQLCreateTableStatement";
	getMySQLCreateTableStatement.static_function = true;
	getMySQLCreateTableStatement.return_type.identifier() = STRING;
	getMySQLCreateTableStatement.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
	{
		structFile << "\treturn \"" << escape_string(generate_create_table_statement_string_struct(ps, s)) << "\";\n";
		return true;
	};
	s.functions.push_back(getMySQLCreateTableStatement);

	FunctionDefinition createMySQLTable;
	createMySQLTable.identifier = "MySQLCreateTable";
	createMySQLTable.static_function = true;
	createMySQLTable.return_type.identifier() = BOOL;
	createMySQLTable.parameters.push_back(std::make_pair(mysql_session, "session"));
	createMySQLTable.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
	{
		structFile << "\ttry {\n";
		structFile << "\t\tsession.sql(getMySQLCreateTableStatement()).execute();\n";
		structFile << "\t\treturn true;\n";
		structFile << "\t} catch (const mysqlx::Error &err) {\n";
		structFile << "\t\tstd::cout << \"MySQL error: \" << err.what() << std::endl;\n";
		structFile << "\t\treturn false;\n";
		structFile << "\t}\n";
		return true;
	};
	s.functions.push_back(createMySQLTable);

	return true;
}

bool MysqlGenerator::generate_files(ProgramStructure ps, std::string out_path)
{
	if (!std::filesystem::exists(out_path))
	{
		std::filesystem::create_directories(out_path);
	}

	// Add foreign key columns for array relationships before generating files
	add_foreign_key_columns_for_arrays(&ps);

	for (auto &s : ps.getStructs())
	{
		if (!generate_struct_files(&ps, s, out_path))
		{
			return false;
		}
	}
	return true;
}
