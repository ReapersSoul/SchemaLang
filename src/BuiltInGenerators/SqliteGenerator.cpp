#include <BuiltInGenerators/SqliteGenerator.hpp>

// utility functions
std::vector<std::vector<int>> SqliteGenerator::comb(int N, int K)
{
	std::string bitmask(K, 1); // K leading 1's
	bitmask.resize(N, 0);	   // N-K trailing 0's

	// print integers and permute bitmask
	std::vector<std::vector<int>> combinations;
	do
	{
		std::vector<int> indexes;
		for (int i = 0; i < N; ++i) // [0..N-1] integers
		{
			if (bitmask[i])
				indexes.push_back(i);
		}
		combinations.push_back(indexes);
	} while (std::prev_permutation(bitmask.begin(), bitmask.end()));
	return combinations;
}

std::vector<std::vector<int>> SqliteGenerator::comb(int N)
{
	std::vector<std::vector<int>> combinations;
	for (int i = 1; i < N + 1; i++)
	{
		std::vector<std::vector<int>> combinations_i = comb(N, i);
		for (auto &c : combinations_i)
		{
			combinations.push_back(c);
		}
	}
	return combinations;
}

// sql string generation functions
std::string SqliteGenerator::generate_create_table_statement_string_struct(ProgramStructure *ps, StructDefinition &s)
{
	std::string sql = "CREATE TABLE IF NOT EXISTS " + s.identifier + " (\n";
	bool first_column = true;
	
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
		
		sql += "\t" + s.member_variables[i].identifier + " ";
		// add type
		sql+= convert_to_local_type(ps, s.member_variables[i].type);
		// add constraints
		if (s.member_variables[i].required)
		{
			sql += " NOT NULL";
		}
		if (s.member_variables[i].unique)
		{
			sql += " UNIQUE";
		}
		if (s.member_variables[i].primary_key)
		{
			sql += " PRIMARY KEY";
		}
		if (!s.member_variables[i].reference.variable_name.empty())
		{
			sql += " REFERENCES " + s.member_variables[i].reference.struct_name + "(" + s.member_variables[i].reference.variable_name + ")";
		}
	}
	sql += "\n);\n";
	return sql;
}

// Function to add foreign key columns for array relationships
void SqliteGenerator::add_foreign_key_columns_for_arrays(ProgramStructure *ps)
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
							MemberVariableDefinition reference_column;
							reference_column.identifier = parent_struct.identifier + "Id";
							reference_column.type = TypeDefinition("int64");
							reference_column.required = member_var.required; // If array is required, reference is NOT NULL
							reference_column.reference.struct_name = parent_struct.identifier;
							reference_column.reference.variable_name = "id"; // Assuming parent has 'id' as primary key
							reference_column.description = "Foreign key reference to " + parent_struct.identifier + " table";
							
							// Check if this foreign key column already exists
							bool reference_exists = false;
							for (auto &existing_var : target_struct.member_variables)
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
								target_struct.member_variables.push_back(reference_column);
							}
							break;
						}
					}
				}
			}
		}
	}
}

std::string SqliteGenerator::generate_select_all_statement_string_member_variable(StructDefinition &s, MemberVariableDefinition &mv)
{
	std::string sql = "SELECT * FROM " + s.identifier + " WHERE " + mv.identifier + " = ?;";
	return sql;
}

std::vector<std::string> SqliteGenerator::genrate_select_all_statements_string_struct(StructDefinition &s)
{
	std::vector<std::string> sqls;
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		if (s.member_variables[i].primary_key)
		{
			continue;
		}
		sqls.push_back(generate_select_all_statement_string_member_variable(s, s.member_variables[i]));
	}
	return sqls;
}

std::string SqliteGenerator::generate_select_by_member_variable_statement_string(StructDefinition &s, MemberVariableDefinition &mv_1, std::vector<int> &criteria)
{
	std::string sql = "SELECT " + mv_1.identifier + " FROM " + s.identifier + " WHERE ";
	for (int i = 0; i < criteria.size(); i++)
	{
		MemberVariableDefinition mv = s.member_variables[criteria[i]];
		sql += mv.identifier + " = ? ";
		if (i < criteria.size() - 1)
		{
			sql += "AND ";
		}
	}
	return sql;
}

std::string SqliteGenerator::generat_insert_statement_string_struct(StructDefinition &s)
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
	sql += ")";
	return sql;
}

std::string SqliteGenerator::generate_update_all_statement_string_struct(StructDefinition &s)
{
	std::string sql = "UPDATE " + s.identifier + " SET ";
	for (int i = 0; i < s.member_variables.size(); i++)
	{
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
		sql += s.member_variables[0].identifier + " = ?";
	}
	return sql;
}

std::string SqliteGenerator::generate_delete_statement_string_struct(StructDefinition &s)
{
	std::string sql = "DELETE FROM " + s.identifier + " WHERE ";
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
		sql += s.member_variables[0].identifier + " = ?";
	}
	return sql;
}

std::string generate_bind(Generator *gen,ProgramStructure *ps, MemberVariableDefinition mv, int i)
{
	std::string ret = "\tsqlite3_bind_";
	if (mv.type.is_struct(ps))
	{
	}
	else if (mv.type.is_enum(ps))
	{
	}
	else if (mv.type.is_integer())
	{
		std::string local_type = gen->convert_to_local_type(ps, mv.type);
		local_type = local_type.substr(0, local_type.size() - 2);
		ret+= local_type + "(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ");\n";
	}
	else if (mv.type.is_real())
	{
		ret+= gen->convert_to_local_type(ps, mv.type) + "(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ");\n";
	}
	else if (mv.type.is_bool())
	{
		ret+= "int(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ");\n";
	}
	else if (mv.type.is_string())
	{
		ret+= "text(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".c_str(), -1, SQLITE_STATIC);\n";
	}
	else if (mv.type.is_char())
	{
		ret+= "text(stmt, " + std::to_string(i + 1) + ", std::string(1, " + mv.identifier + ").c_str(), -1, SQLITE_STATIC);\n";
	}
	else if (mv.type.is_array())
	{
		ret+= "text(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".c_str());\n";
	}
	else
	{
		ret+= gen->convert_to_local_type(ps, mv.type) + "(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ");\n";
	}
	return ret;
}

// functions for c++ code generation
void SqliteGenerator::generate_select_all_statement_function_member_variable(Generator *gen, ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv)
{
	if (mv.type.is_array())
	{
		return;
	}
	if (mv.type.is_struct(ps))
	{
		return;
	}
	else if (mv.type.is_enum(ps))
	{
		return;
	}
	FunctionDefinition select_all_statement;
	select_all_statement.generator = "SQLite";
	select_all_statement.identifier = "SQLiteSelectBy" + mv.identifier;
	select_all_statement.return_type.identifier() = "std::vector<" + s.identifier + "Schema*>";
	select_all_statement.static_function = true;
	select_all_statement.parameters.push_back(std::make_pair(sqlite_db, "db"));
	select_all_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));

	select_all_statement.generate_function = [this, &mv](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		if (mv.type.is_array())
		{
			return true;
		}
		if (mv.type.is_struct(ps))
		{
			return true;
		}
		else if (mv.type.is_enum(ps))
		{
			return true;
		}

		structFile << "\tstd::vector<" << s.identifier << "Schema*> results;\n";
		structFile << "\tchar *zErrMsg = 0;\n";
		structFile << "\tstd::string sql = \"";
		structFile << generate_select_all_statement_string_member_variable(s, mv);
		structFile << "\";\n";
		structFile << "\tsqlite3_stmt *stmt;\n";
		structFile << "\tsqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);\n";
		structFile << generate_bind(gen,ps,mv, 0);
		structFile << "\twhile(sqlite3_step(stmt) == SQLITE_ROW){\n";
		structFile << "\t\t" << s.identifier << "Schema* result = new " << s.identifier << "Schema();\n";
		for (auto &mv_field : s.member_variables)
		{
			if (mv_field.type.is_array())
			{
				// Skip arrays for now - they need special handling
				continue;
			}
			else if (mv_field.type.is_struct(ps))
			{
				// Skip struct references for now - they need special handling
				continue;
			}
			else if (mv_field.type.is_enum(ps))
			{
				continue;
			}
			else if (mv_field.type.is_integer())
			{
				std::string local_type = gen->convert_to_local_type(ps, mv_field.type);
				local_type = local_type.substr(0, local_type.size() - 2);
				structFile << "\t\tresult->set" << mv_field.identifier << "(sqlite3_column_" << local_type << "(stmt, " << mv_field.identifier << "_index));\n";
			}
			else if (mv_field.type.is_real())
			{
				structFile << "\t\tresult->set" << mv_field.identifier << "(sqlite3_column_" << gen->convert_to_local_type(ps, mv_field.type) << "(stmt, " << mv_field.identifier << "_index));\n";
			}
			else if (mv_field.type.is_bool())
			{
				structFile << "\t\tresult->set" << mv_field.identifier << "(sqlite3_column_int(stmt, " << mv_field.identifier << "_index));\n";
			}
			else if (mv_field.type.is_string())
			{
				structFile << "\t\tresult->set" << mv_field.identifier << "(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, " << mv_field.identifier << "_index))));\n";
			}
			else if (mv_field.type.is_char())
			{
				structFile << "\t\tresult->set" << mv_field.identifier << "(sqlite3_column_text(stmt, " << mv_field.identifier << "_index)[0]);\n";
			}
			else
			{
				structFile << "\t\tresult->set" << mv_field.identifier << "(sqlite3_column_" << gen->convert_to_local_type(ps, mv_field.type) << "(stmt, " << mv_field.identifier << "_index));\n";
			}
		}
		structFile << "\t\tresults.push_back(result);\n";
		structFile << "\t}\n";
		structFile << "\tsqlite3_finalize(stmt);\n";
		structFile << "\treturn results;\n";
		return true;
	};
	s.functions.push_back(select_all_statement);
}

void SqliteGenerator::generate_select_all_statement_functions_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		generate_select_all_statement_function_member_variable(gen, ps, s, s.member_variables[i]);
	}
}

void SqliteGenerator::generate_select_member_variable_function_statement(Generator *gen, ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv_1, std::vector<int> &criteria)
{
	FunctionDefinition select_statement;
	select_statement.generator = "SQLite";
	select_statement.identifier = "SQLiteSelect" + s.identifier + "By";
	select_statement.return_type = mv_1.type;
	for (int i = 0; i < criteria.size(); i++)
	{
		MemberVariableDefinition mv = s.member_variables[criteria[i]];
		select_statement.parameters.push_back(std::make_pair(mv.type.identifier(), mv.identifier));
	}
	select_statement.generate_function = [this, &mv_1, &criteria](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\tchar *zErrMsg = 0;\n";
		structFile << "\tstd::string sql = \"";
		structFile << generate_select_by_member_variable_statement_string(s, mv_1, criteria);
		structFile << "\";\n";
		structFile << "\tsqlite3_stmt *stmt;\n";
		structFile << "\tsqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);\n";
		for (int i = 0; i < criteria.size(); i++)
		{
			structFile << generate_bind(gen,ps,s.member_variables[criteria[i]], i);
		}
		structFile << "\tif(sqlite3_step(stmt) == SQLITE_ROW){\n";
		structFile << "\t\t" << mv_1.identifier << " = sqlite3_column_" << mv_1.type.identifier() << "(stmt, 0);\n";
		structFile << "\t}\n";
		structFile << "\tsqlite3_finalize(stmt);\n";
		return true;
	};
	s.functions.push_back(select_statement);
}

void SqliteGenerator::generate_select_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	generate_select_all_statement_functions_struct(gen, ps, s);
	// std::vector<std::vector<int>> combinations = comb(s.member_variables.size());
	// for (int i = 0; i < s.member_variables.size(); i++)
	// {
	// 	if (s.member_variables[i].primary_key)
	// 	{
	// 		continue;
	// 	}
	// 	for (int j = 0; j < combinations.size(); j++)
	// 	{
	// 		sqls.push_back(generate_select_member_variable_statement(s, s.member_variables[i], combinations[j]));
	// 	}
	// }
}

void SqliteGenerator::generate_insert_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	FunctionDefinition insert_statement;
	insert_statement.generator = "SQLite";
	insert_statement.identifier = "SQLiteInsert";
	insert_statement.return_type.identifier() = BOOL;
	insert_statement.static_function = true;
	insert_statement.parameters.push_back(std::make_pair(sqlite_db, "db"));
	for (auto &mv : s.member_variables)
	{
		insert_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));
	}
	insert_statement.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\tchar *zErrMsg = 0;\n";
		structFile << "\tstd::string sql = \"";
		structFile << generat_insert_statement_string_struct(s);
		structFile << "\";\n";
		structFile << "\tsqlite3_stmt *stmt;\n";
		structFile << "\tsqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);\n";
		for (int i = 0; i < s.member_variables.size(); i++)
		{
			if (s.member_variables[i].type.is_enum(ps))
			{
				continue;
			}
			if (s.member_variables[i].type.is_struct(ps))
			{
				continue;
			}
			if (s.member_variables[i].type.is_array())
			{
				continue;
			}
			structFile << generate_bind(gen,ps,s.member_variables[i], i);
		}
		structFile << "\tif(sqlite3_step(stmt) != SQLITE_DONE){\n";
		structFile << "\t\tsqlite3_finalize(stmt);\n";
		structFile << "\t\treturn false;\n";
		structFile << "\t}\n";
		structFile << "\tsqlite3_finalize(stmt);\n";
		structFile << "\treturn true;\n";
		return true;
	};
	s.functions.push_back(insert_statement);

	FunctionDefinition insert_statement_no_args;
	insert_statement_no_args.generator = "SQLite";
	insert_statement_no_args.identifier = "SQLiteInsert";
	insert_statement_no_args.return_type.identifier() = BOOL;
	insert_statement_no_args.static_function = false;
	insert_statement_no_args.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\tchar *zErrMsg = 0;\n";
		structFile << "\tstd::string sql = \"";
		structFile << generat_insert_statement_string_struct(s);
		structFile << "\";\n";
		structFile << "\tsqlite3_stmt *stmt;\n";
		structFile << "\tsqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);\n";
		for (int i = 0; i < s.member_variables.size(); i++)
		{
			if (s.member_variables[i].type.is_enum(ps))
			{
				continue;
			}
			if (s.member_variables[i].type.is_struct(ps))
			{
				continue;
			}
			if (s.member_variables[i].type.is_array())
			{
				continue;
			}
			structFile << generate_bind(gen,ps,s.member_variables[i], i);
		}
		structFile << "\tif(sqlite3_step(stmt) != SQLITE_DONE){\n";
		structFile << "\t\tsqlite3_finalize(stmt);\n";
		structFile << "\t\treturn false;\n";
		structFile << "\t}\n";
		structFile << "\tsqlite3_finalize(stmt);\n";
		structFile << "\treturn true;\n";
		return true;
	};
	s.functions.push_back(insert_statement_no_args);
}

void SqliteGenerator::generate_update_all_statement_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	FunctionDefinition update_all_statement;
	update_all_statement.generator = "SQLite";
	update_all_statement.identifier = "SQLiteUpdate" + s.identifier;
	update_all_statement.return_type.identifier() = BOOL;
	for (auto &mv : s.member_variables)
	{
		update_all_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));
	}
	update_all_statement.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\tchar *zErrMsg = 0;\n";
		structFile << "\tstd::string sql = \"";
		structFile << generate_update_all_statement_string_struct(s);
		structFile << "\";\n";
		structFile << "\tsqlite3_stmt *stmt;\n";
		structFile << "\tsqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);\n";
		for (int i = 0; i < s.member_variables.size(); i++)
		{
			structFile << generate_bind(gen,ps,s.member_variables[i], i);
		}
		structFile << "\tif(sqlite3_step(stmt) != SQLITE_DONE){\n";
		structFile << "\t\tsqlite3_finalize(stmt);\n";
		structFile << "\t\treturn false;\n";
		structFile << "\t}\n";
		structFile << "\tsqlite3_finalize(stmt);\n";
		structFile << "\treturn true;\n";
		return true;
	};
	s.functions.push_back(update_all_statement);
}

void SqliteGenerator::generate_update_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	FunctionDefinition update_statement;
	update_statement.generator = "SQLite";
	update_statement.identifier = "SQLiteUpdate";
	update_statement.return_type.identifier() = "bool";
	update_statement.static_function = true;
	update_statement.parameters.push_back(std::make_pair(sqlite_db, "db"));
	for (auto &mv : s.member_variables)
	{
		if (mv.type.is_array())
		{
			continue;
		}
		update_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));
	}
	update_statement.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\tchar *zErrMsg = 0;\n";
		structFile << "\tstd::string sql = \"";
		structFile << generate_update_all_statement_string_struct(s);
		structFile << "\";\n";
		structFile << "\tsqlite3_stmt *stmt;\n";
		structFile << "\tsqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);\n";
		
		int param_index = 1;
		// Bind all non-array parameters for SET clause
		for (auto &mv : s.member_variables)
		{
			if (mv.type.is_array() || mv.primary_key)
			{
				continue;
			}
			structFile << generate_bind(gen, ps, mv, param_index - 1);
			param_index++;
		}
		
		// Bind primary key parameter for WHERE clause
		for (auto &mv : s.member_variables)
		{
			if (mv.primary_key)
			{
				structFile << generate_bind(gen, ps, mv, param_index - 1);
				break;
			}
		}
		
		structFile << "\tint result = sqlite3_step(stmt);\n";
		structFile << "\tsqlite3_finalize(stmt);\n";
		structFile << "\treturn result == SQLITE_DONE;\n";
		return true;
	};
	s.functions.push_back(update_statement);

	// Instance method version
	FunctionDefinition update_statement_no_args;
	update_statement_no_args.generator = "SQLite";
	update_statement_no_args.identifier = "SQLiteUpdate";
	update_statement_no_args.return_type.identifier() = "bool";
	update_statement_no_args.static_function = false;
	update_statement_no_args.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\treturn SQLiteUpdate(db";
		for (auto &mv : s.member_variables)
		{
			if (mv.type.is_array())
			{
				continue;
			}
			structFile << ", " << mv.identifier;
		}
		structFile << ");\n";
		return true;
	};
	s.functions.push_back(update_statement_no_args);
}

void SqliteGenerator::generate_delete_statement_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	FunctionDefinition delete_statement;
	delete_statement.generator = "SQLite";
	delete_statement.identifier = "SQLiteDelete";
	delete_statement.return_type.identifier() = "bool";
	delete_statement.static_function = true;
	delete_statement.parameters.push_back(std::make_pair(sqlite_db, "db"));
	
	// Find primary key parameter
	for (auto &mv : s.member_variables)
	{
		if (mv.primary_key)
		{
			delete_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));
			break;
		}
	}
	// If no primary key, use first field
	if (delete_statement.parameters.size() == 1)
	{
		delete_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, s.member_variables[0].type), s.member_variables[0].identifier));
	}
	
	delete_statement.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\tchar *zErrMsg = 0;\n";
		structFile << "\tstd::string sql = \"";
		structFile << generate_delete_statement_string_struct(s);
		structFile << "\";\n";
		structFile << "\tsqlite3_stmt *stmt;\n";
		structFile << "\tsqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);\n";
		
		// Bind the primary key (or first field) parameter
		for (auto &mv : s.member_variables)
		{
			if (mv.primary_key)
			{
				structFile << generate_bind(gen, ps, mv, 0);
				break;
			}
		}
		if (!s.member_variables.empty() && !s.member_variables[0].primary_key)
		{
			// If no primary key found, use first field
			bool has_primary_key = false;
			for (auto &mv : s.member_variables)
			{
				if (mv.primary_key)
				{
					has_primary_key = true;
					break;
				}
			}
			if (!has_primary_key)
			{
				structFile << generate_bind(gen, ps, s.member_variables[0], 0);
			}
		}
		
		structFile << "\tint result = sqlite3_step(stmt);\n";
		structFile << "\tsqlite3_finalize(stmt);\n";
		structFile << "\treturn result == SQLITE_DONE;\n";
		return true;
	};
	s.functions.push_back(delete_statement);

	// Instance method version
	FunctionDefinition delete_statement_no_args;
	delete_statement_no_args.generator = "SQLite";
	delete_statement_no_args.identifier = "SQLiteDelete";
	delete_statement_no_args.return_type.identifier() = "bool";
	delete_statement_no_args.static_function = false;
	delete_statement_no_args.parameters.push_back(std::make_pair(sqlite_db, "db"));
	delete_statement_no_args.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\treturn SQLiteDelete(db";
		// Find primary key
		for (auto &mv : s.member_variables)
		{
			if (mv.primary_key)
			{
				structFile << ", " << mv.identifier;
				break;
			}
		}
		// If no primary key, use first field
		if (!s.member_variables.empty())
		{
			bool has_primary_key = false;
			for (auto &mv : s.member_variables)
			{
				if (mv.primary_key)
				{
					has_primary_key = true;
					break;
				}
			}
			if (!has_primary_key)
			{
				structFile << ", " << s.member_variables[0].identifier;
			}
		}
		structFile << ");\n";
		return true;
	};
	s.functions.push_back(delete_statement_no_args);
}

// file generation functions
bool SqliteGenerator::generate_create_table_file(ProgramStructure *ps, StructDefinition &s, std::string out_path)
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

bool SqliteGenerator::generate_select_all_files(ProgramStructure *ps, StructDefinition &s, std::string out_path)
{
	std::vector<std::string> sqls = genrate_select_all_statements_string_struct(s);
	for (int i = 0; i < sqls.size(); i++)
	{
		std::ofstream structFile(out_path + "/" + s.identifier + "_select_all_" + s.member_variables[i].identifier + ".sql");
		if (!structFile.is_open())
		{
			std::cout << "Failed to open file: " << out_path + "/" + s.identifier + "_select_all_" + s.member_variables[i].identifier + ".sql" << std::endl;
			return false;
		}
		structFile << sqls[i] << std::endl;
		structFile.close();
	}
	return true;
}

bool SqliteGenerator::generate_select_files(ProgramStructure *ps, StructDefinition &s, std::string out_path)
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
			std::ofstream structFile(out_path + "/" + s.identifier + "_select_" + s.member_variables[i].identifier + "_by_");
			for (int k = 0; k < combinations[j].size(); k++)
			{
				structFile << s.member_variables[combinations[j][k]].identifier;
				if (k < combinations[j].size() - 1)
				{
					structFile << "_";
				}
			}
			structFile << ".sql";
			if (!structFile.is_open())
			{
				std::cout << "Failed to open file: " << out_path + "/" + s.identifier + "_select_" + s.member_variables[i].identifier + "_by_";
				for (int k = 0; k < combinations[j].size(); k++)
				{
					structFile << s.member_variables[combinations[j][k]].identifier;
					if (k < combinations[j].size() - 1)
					{
						structFile << "_";
					}
				}
				structFile << ".sql" << std::endl;
				return false;
			}
			structFile << generate_select_by_member_variable_statement_string(s, s.member_variables[i], combinations[j]) << std::endl;
			structFile.close();
		}
	}
	return true;
}

bool SqliteGenerator::generate_struct_files(ProgramStructure *ps, StructDefinition &s, std::string out_path)
{
	if (!std::filesystem::exists(out_path))
	{
		std::filesystem::create_directories(out_path);
	}

	if (!generate_create_table_file(ps, s, out_path))
	{
		std::cout << "Failed to generate create table file for struct: " << s.identifier << std::endl;
		return false;
	}

	return true;
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
	// base_class.identifier = "Sqlite";
	// base_class.includes.push_back("<sqlite3.h>");
	// base_class.includes.push_back("<iostream>");
	// base_class.includes.push_back("<string>");
	// base_class.includes.push_back("<vector>");
}

std::string SqliteGenerator::convert_to_local_type(ProgramStructure *ps, TypeDefinition type)
{
	// convert int types to "INTEGER"
	if (type.is_struct(ps))
	{
		return "INTEGER";
	}

	if (type.is_enum(ps))
	{
		return "TEXT";
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

bool SqliteGenerator::add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	if(gen->name=="Cpp"){
		s.includes.insert({"SQLite","<sqlite3.h>"});
		s.includes.insert({"","<iostream>"});
		s.includes.insert({"","<string>"});
		s.includes.insert({"","<vector>"});

		// add index private variables for each member variable
		bool has_primary_key = false;

		int i = 0;
		for (auto &mv : s.member_variables)
		{
			if (mv.primary_key)
			{
				PrivateVariableDefinition primary_key_index;
				primary_key_index.type = TypeDefinition("int");
				primary_key_index.identifier = "primary_key_index";
				primary_key_index.in_class_init = true;
				primary_key_index.static_member = true;
				primary_key_index.const_member = true;
				primary_key_index.generate_initializer = [i](ProgramStructure *ps, PrivateVariableDefinition &mv, std::ostream &structFile)
				{
					structFile << std::to_string(i);
					return true;
				};
			}
			PrivateVariableDefinition index;
			index.type = TypeDefinition("int");
			index.identifier = mv.identifier + "_index";
			index.in_class_init = true;
			index.static_member = true;
			index.const_member = true;
			index.generate_initializer = [i](ProgramStructure *ps, PrivateVariableDefinition &mv, std::ostream &structFile)
			{
				structFile << std::to_string(i);
				return true;
			};
			s.private_variables.push_back(index);
			i++;
		}

		PrivateVariableDefinition database;
		database.type = TypeDefinition("sqlite3 *");
		database.identifier = "db";
		database.static_member = true;

		// add select statements
		generate_select_statements_function_struct(gen, ps, s);
		// add insert statement
		generate_insert_statements_function_struct(gen, ps, s);
		// add update statements
		generate_update_statements_function_struct(gen, ps, s);
		// add delete statements
		generate_delete_statement_function_struct(gen, ps, s);

		FunctionDefinition getCreateTableStatement;
		getCreateTableStatement.generator = name;
		getCreateTableStatement.identifier = "getSQLiteCreateTableStatement";
		getCreateTableStatement.static_function = true;
		getCreateTableStatement.return_type.identifier() = STRING;
		getCreateTableStatement.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
		{
			structFile << "\treturn \"" << escape_string(generate_create_table_statement_string_struct(ps, s)) << "\";\n";
			return true;
		};
		s.functions.push_back(getCreateTableStatement);

		FunctionDefinition createTable;
		createTable.generator = name;
		createTable.identifier = "SQLiteCreateTable";
		createTable.static_function = true;
		createTable.return_type.identifier() = BOOL;
		createTable.parameters.push_back(std::make_pair(TypeDefinition("sqlite3 *"), "db"));
		createTable.static_function = true;
		createTable.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
		{
			structFile << "\tchar *zErrMsg = 0;\n";
			structFile << "\tint rc = sqlite3_exec(db, getSQLiteCreateTableStatement().c_str(), NULL, 0, &zErrMsg);\n";
			structFile << "\tif(rc != SQLITE_OK){\n";
			structFile << "\t\tstd::cout << \"SQL error: \" << zErrMsg << std::endl;\n";
			structFile << "\t\tsqlite3_free(zErrMsg);\n";
			structFile << "\t\treturn false;\n";
			structFile << "\t}\n";
			structFile << "\treturn true;\n";
			return true;
		};
		s.functions.push_back(createTable);

		//register update listener
		FunctionDefinition registerUpdateListener;
		registerUpdateListener.generator = name;
		registerUpdateListener.identifier = "SQLiteRegisterUpdateListener";
		registerUpdateListener.static_function = true;
		registerUpdateListener.return_type.identifier() = "bool";
		registerUpdateListener.parameters.push_back(std::make_pair(TypeDefinition("sqlite3 *"), "db"));
		registerUpdateListener.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
		{
			structFile << "\t// Register update listener for SQLite database\n";
			structFile << "\t// This is a placeholder for actual implementation\n";
			structFile << "\treturn true;\n";
			return true;
		};
		s.functions.push_back(registerUpdateListener);

		s.before_setter_lines.push_back({name,"if(!SQLiteUpdate(db, *this)){\n"
			"\t\tstd::cerr << \"Failed to update " + s.identifier + " in SQLite database.\" << std::endl;\n"
			"\t\treturn;\n"
			"\t}\n"});
	}else{
		std::cout << "Warning: SqliteGenerator only supports C++ code generation." << std::endl;
	}

	return true;
}

bool SqliteGenerator::generate_files(ProgramStructure ps, std::string out_path)
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
			std::cout << "Failed to generate sqlite file for struct: " << s.identifier << std::endl;
			return false;
		}
	}

	return true;
}