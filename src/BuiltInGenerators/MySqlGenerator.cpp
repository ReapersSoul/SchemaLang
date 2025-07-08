#include <BuiltInGenerators/MySqlGenerator.hpp>
#include <set>
#include <algorithm>
#include <cctype>

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
	std::string sql = "CREATE TABLE IF NOT EXISTS " + escape_identifier(s.identifier) + " (\n";
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
		
		sql += "  " + escape_identifier(s.member_variables[i].identifier) + " " + convert_to_local_type(ps, s.member_variables[i].type);
		
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
		if (!s.member_variables[i].reference.variable_name.empty())
		{
			std::string reference_constraint = "  FOREIGN KEY (" + escape_identifier(s.member_variables[i].identifier) + ") REFERENCES " + 
				escape_identifier(s.member_variables[i].reference.struct_name) + "(" + escape_identifier(s.member_variables[i].reference.variable_name) + ")";
			foreign_keys.push_back(reference_constraint);
		}
	}
	
	// Add foreign key constraints
	for (const auto& reference : foreign_keys)
	{
		sql += ",\n" + reference;
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

std::string MysqlGenerator::generate_select_all_statement_string_member_variable(StructDefinition &s, MemberVariableDefinition &mv)
{
	std::string sql = "SELECT * FROM " + escape_identifier(s.identifier) + " WHERE " + escape_identifier(mv.identifier) + " = ?;";
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
	std::string sql = "SELECT " + escape_identifier(mv_1.identifier) + " FROM " + escape_identifier(s.identifier) + " WHERE ";
	for (int i = 0; i < criteria.size(); i++)
	{
		sql += escape_identifier(s.member_variables[criteria[i]].identifier) + " = ?";
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
	std::string sql = "INSERT INTO " + escape_identifier(s.identifier) + " (";
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		sql += escape_identifier(s.member_variables[i].identifier);
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
	std::string sql = "UPDATE " + escape_identifier(s.identifier) + " SET ";
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		if (s.member_variables[i].primary_key)
		{
			continue;
		}
		sql += escape_identifier(s.member_variables[i].identifier) + " = ?";
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
			sql += escape_identifier(s.member_variables[i].identifier) + " = ?";
			has_primary_key = true;
			break;
		}
	}
	if (!has_primary_key)
	{
		// Use the first column as identifier if no primary key
		sql += escape_identifier(s.member_variables[0].identifier) + " = ?";
	}
	sql += ";";
	return sql;
}

std::string MysqlGenerator::generate_delete_statement_string_struct(StructDefinition &s)
{
	std::string sql = "DELETE FROM " + escape_identifier(s.identifier) + " WHERE ";
	bool has_primary_key = false;
	for (int i = 0; i < s.member_variables.size(); i++)
	{
		if (s.member_variables[i].primary_key)
		{
			sql += escape_identifier(s.member_variables[i].identifier) + " = ?";
			has_primary_key = true;
			break;
		}
	}
	if (!has_primary_key)
	{
		sql += escape_identifier(s.member_variables[0].identifier) + " = ?";
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
		structFile << "\t\tmysqlx::Table table = db.getTable(\"" << escape_identifier(s.identifier) << "\");\n";
		structFile << "\t\tmysqlx::RowResult result = table.select(\"*\")\n";
		structFile << "\t\t\t.where(\"" << escape_identifier(mv.identifier) << " = :param\")\n";
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
		structFile << "\t\tmysqlx::Table table = db.getTable(\"" << escape_identifier(s.identifier) << "\");\n";
		structFile << "\t\tstd::string where_clause = \"\";\n";
		
		for (int i = 0; i < criteria.size(); i++)
		{
			structFile << "\t\tif (i > 0) where_clause += \" AND \";\n";
			structFile << "\t\twhere_clause += \"" << escape_identifier(s.member_variables[criteria[i]].identifier) << " = :param" << i << "\";\n";
		}
		
		structFile << "\t\tmysqlx::RowResult result = table.select(\"" << escape_identifier(mv_1.identifier) << "\")\n";
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
		structFile << "\t\tmysqlx::Table table = db.getTable(\"" << escape_identifier(s.identifier) << "\");\n";
		structFile << "\t\ttable.insert(";
		for (int i = 0; i < s.member_variables.size(); i++)
		{
			structFile << "\"" << escape_identifier(s.member_variables[i].identifier) << "\"";
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
		structFile << "\t\tmysqlx::Table table = db.getTable(\"" << escape_identifier(s.identifier) << "\");\n";
		structFile << "\t\ttable.insert(";
		for (int i = 0; i < s.member_variables.size(); i++)
		{
			structFile << "\"" << escape_identifier(s.member_variables[i].identifier) << "\"";
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
		structFile << "\t\tmysqlx::Table table = db.getTable(\"" << escape_identifier(s.identifier) << "\");\n";
		
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
				structFile << "\t\tupdate.set(\"" << escape_identifier(mv.identifier) << "\", " << mv.identifier << ");\n";
			}
		}
		
		if (has_primary_key)
		{
			structFile << "\t\tupdate.where(\"" << escape_identifier(primary_key_field) << " = :pk\").bind(\"pk\", " << primary_key_field << ");\n";
		}
		else
		{
			structFile << "\t\tupdate.where(\"" << escape_identifier(s.member_variables[0].identifier) << " = :pk\").bind(\"pk\", " << s.member_variables[0].identifier << ");\n";
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

void MysqlGenerator::generate_update_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	FunctionDefinition update_statement;
	update_statement.identifier = "MySQLUpdate";
	update_statement.return_type.identifier() = "bool";
	update_statement.static_function = true;
	update_statement.parameters.push_back(std::make_pair(mysql_session, "session"));
	for (auto &mv : s.member_variables)
	{
		if (mv.type.is_array())
		{
			continue;
		}
		update_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));
	}
	update_statement.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
	{
		structFile << "\ttry {\n";
		structFile << "\t\tmysqlx::Schema db = session.getSchema(\"" << s.identifier << "_db\");\n";
		structFile << "\t\tmysqlx::Table table = db.getTable(\"" << escape_identifier(s.identifier) << "\");\n";
		structFile << "\t\tmysqlx::TableUpdate update = table.update();\n";
		
		// Set all non-primary key fields
		for (auto &mv : s.member_variables)
		{
			if (mv.type.is_array() || mv.primary_key)
			{
				continue;
			}
			structFile << "\t\tupdate.set(\"" << escape_identifier(mv.identifier) << "\", " << mv.identifier << ");\n";
		}
		
		// Add WHERE clause for primary key
		for (auto &mv : s.member_variables)
		{
			if (mv.primary_key)
			{
				structFile << "\t\tupdate.where(\"" << escape_identifier(mv.identifier) << " = :pk\").bind(\"pk\", " << mv.identifier << ");\n";
				break;
			}
		}
		
		structFile << "\t\tmysqlx::Result result = update.execute();\n";
		structFile << "\t\treturn result.getAffectedItemsCount() > 0;\n";
		structFile << "\t} catch (const std::exception& e) {\n";
		structFile << "\t\tstd::cerr << \"MySQL Update error: \" << e.what() << std::endl;\n";
		structFile << "\t\treturn false;\n";
		structFile << "\t}\n";
		return true;
	};
	s.functions.push_back(update_statement);

	// Instance method version
	FunctionDefinition update_statement_no_args;
	update_statement_no_args.identifier = "MySQLUpdate";
	update_statement_no_args.return_type.identifier() = "bool";
	update_statement_no_args.static_function = false;
	update_statement_no_args.parameters.push_back(std::make_pair(mysql_session, "session"));
	update_statement_no_args.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
	{
		structFile << "\treturn MySQLUpdate(session";
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

void MysqlGenerator::generate_delete_statement_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	FunctionDefinition delete_statement;
	delete_statement.identifier = "MySQLDelete";
	delete_statement.return_type.identifier() = "bool";
	delete_statement.static_function = true;
	delete_statement.parameters.push_back(std::make_pair(mysql_session, "session"));
	
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
	
	delete_statement.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
	{
		structFile << "\ttry {\n";
		structFile << "\t\tmysqlx::Schema db = session.getSchema(\"" << s.identifier << "_db\");\n";
		structFile << "\t\tmysqlx::Table table = db.getTable(\"" << escape_identifier(s.identifier) << "\");\n";
		
		// Find primary key field for WHERE clause
		for (auto &mv : s.member_variables)
		{
			if (mv.primary_key)
			{
				structFile << "\t\tmysqlx::Result result = table.remove()\n";
				structFile << "\t\t\t.where(\"" << escape_identifier(mv.identifier) << " = :pk\")\n";
				structFile << "\t\t\t.bind(\"pk\", " << mv.identifier << ")\n";
				structFile << "\t\t\t.execute();\n";
				break;
			}
		}
		
		// If no primary key found, use first field
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
				structFile << "\t\tmysqlx::Result result = table.remove()\n";
				structFile << "\t\t\t.where(\"" << escape_identifier(s.member_variables[0].identifier) << " = :param\")\n";
				structFile << "\t\t\t.bind(\"param\", " << s.member_variables[0].identifier << ")\n";
				structFile << "\t\t\t.execute();\n";
			}
		}
		
		structFile << "\t\treturn result.getAffectedItemsCount() > 0;\n";
		structFile << "\t} catch (const std::exception& e) {\n";
		structFile << "\t\tstd::cerr << \"MySQL Delete error: \" << e.what() << std::endl;\n";
		structFile << "\t\treturn false;\n";
		structFile << "\t}\n";
		return true;
	};
	s.functions.push_back(delete_statement);

	// Instance method version
	FunctionDefinition delete_statement_no_args;
	delete_statement_no_args.identifier = "MySQLDelete";
	delete_statement_no_args.return_type.identifier() = "bool";
	delete_statement_no_args.static_function = false;
	delete_statement_no_args.parameters.push_back(std::make_pair(mysql_session, "session"));
	delete_statement_no_args.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
	{
		structFile << "\treturn MySQLDelete(session";
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

bool MysqlGenerator::is_mysql_keyword(const std::string& word)
{
	// Convert to uppercase for comparison
	std::string upper_word = word;
	std::transform(upper_word.begin(), upper_word.end(), upper_word.begin(), ::toupper);
	
	// Common MySQL reserved keywords that might conflict with schema names
	static const std::set<std::string> mysql_keywords = {
		"CHARACTER", "CHAR", "VARCHAR", "TEXT", "BLOB", "JSON", "INT", "INTEGER", 
		"BIGINT", "SMALLINT", "TINYINT", "DECIMAL", "NUMERIC", "FLOAT", "DOUBLE",
		"DATE", "TIME", "DATETIME", "TIMESTAMP", "YEAR", "BOOL", "BOOLEAN",
		"TABLE", "INDEX", "KEY", "PRIMARY", "FOREIGN", "REFERENCES", "CONSTRAINT",
		"UNIQUE", "NULL", "NOT", "DEFAULT", "AUTO_INCREMENT", "UNSIGNED",
		"SELECT", "INSERT", "UPDATE", "DELETE", "FROM", "WHERE", "ORDER", "BY",
		"GROUP", "HAVING", "LIMIT", "OFFSET", "JOIN", "LEFT", "RIGHT", "INNER",
		"OUTER", "ON", "AS", "AND", "OR", "IN", "EXISTS", "LIKE", "BETWEEN",
		"IS", "DISTINCT", "ALL", "ANY", "SOME", "UNION", "INTERSECT", "EXCEPT",
		"CREATE", "ALTER", "DROP", "TRUNCATE", "RENAME", "SHOW", "DESCRIBE",
		"EXPLAIN", "USE", "DATABASE", "SCHEMA", "IF", "ELSE", "CASE", "WHEN",
		"THEN", "END", "LOOP", "WHILE", "FOR", "REPEAT", "UNTIL", "RETURN",
		"FUNCTION", "PROCEDURE", "TRIGGER", "EVENT", "VIEW", "TEMPORARY",
		"PERSISTENT", "VIRTUAL", "STORED", "GENERATED", "ALWAYS", "CURRENT_TIMESTAMP",
		"CURRENT_DATE", "CURRENT_TIME", "NOW", "TODAY", "YESTERDAY", "TOMORROW",
		"USER", "SESSION", "GLOBAL", "LOCAL", "MASTER", "SLAVE", "REPLICATION",
		"BACKUP", "RESTORE", "IMPORT", "EXPORT", "LOAD", "DATA", "INFILE",
		"OUTFILE", "FIELDS", "LINES", "TERMINATED", "ENCLOSED", "ESCAPED",
		"STARTING", "IGNORE", "REPLACE", "DUPLICATE", "CONFLICT", "ABORT",
		"FAIL", "ROLLBACK", "COMMIT", "BEGIN", "START", "TRANSACTION", "WORK",
		"SAVEPOINT", "RELEASE", "LOCK", "UNLOCK", "TABLES", "READ", "WRITE",
		"SHARE", "MODE", "WAIT", "NOWAIT", "SKIP", "LOCKED", "FOR", "UPDATE",
		"OF", "WITH", "WITHOUT", "RECURSIVE", "MATERIALIZED", "IMMEDIATE",
		"DEFERRED", "INITIALLY", "CASCADE", "RESTRICT", "SET", "ACTION",
		"MATCH", "FULL", "PARTIAL", "SIMPLE", "CROSS", "NATURAL", "USING",
		"STRAIGHT_JOIN", "FORCE", "USE", "IGNORE", "HIGH_PRIORITY", "LOW_PRIORITY",
		"DELAYED", "QUICK", "EXTENDED", "CHANGED", "FAST", "MEDIUM", "REPAIR",
		"OPTIMIZE", "ANALYZE", "CHECK", "CHECKSUM", "FLUSH", "RESET", "PURGE",
		"KILL", "PROCESSLIST", "STATUS", "VARIABLES", "ENGINES", "PLUGINS",
		"COLLATION", "CHARSET", "CHARACTER_SET", "BINARY", "ASCII", "UNICODE",
		"UTF8", "UTF8MB4", "LATIN1", "CP1251", "KOI8R", "TIS620", "EUCJPMS",
		"SJIS", "BIG5", "GBK", "GB2312", "MACROMAN", "MACCE", "KEYBCS2",
		"ARMSCII8", "GEOSTD8", "HEBREW", "GREEK", "SAMI", "DANISH", "FINNISH",
		"NORWEGIAN", "SWEDISH", "ICELANDIC", "CZECH", "SLOVAK", "POLISH",
		"HUNGARIAN", "SLOVENIAN", "CROATIAN", "SERBIAN", "BULGARIAN", "UKRAINIAN",
		"RUSSIAN", "ESTONIAN", "LATVIAN", "LITHUANIAN", "TURKISH", "ROMANIAN"
	};
	
	return mysql_keywords.find(upper_word) != mysql_keywords.end();
}

std::string MysqlGenerator::escape_identifier(const std::string& identifier)
{
	if (is_mysql_keyword(identifier))
	{
		return "`" + identifier + "`";
	}
	return identifier;
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
	s.includes.insert("<mysqlx/xdevapi.h>");
	s.includes.insert("<iostream>");
	s.includes.insert("<string>");
	s.includes.insert("<vector>");

	generate_select_statements_function_struct(gen, ps, s);
	generate_insert_statements_function_struct(gen, ps, s);
	generate_update_all_statement_function_struct(gen, ps, s);
	generate_update_statements_function_struct(gen, ps, s);
	generate_delete_statement_function_struct(gen, ps, s);

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
