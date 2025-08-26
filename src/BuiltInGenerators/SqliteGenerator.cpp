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
	std::string sql = "CREATE TABLE IF NOT EXISTS " + s.getIdentifier() + " (\n";
	bool first_column = true;
	
	for (int i = 0; i < s.getMemberVariables().size(); i++)
	{
		if(!s.getMemberVariables()[i].second.enabled_for_generators.empty()||
			!s.getMemberVariables()[i].second.disabled_for_generators.empty()){
			if(std::find(s.getMemberVariables()[i].second.enabled_for_generators.begin(), s.getMemberVariables()[i].second.enabled_for_generators.end(), name) == s.getMemberVariables()[i].second.enabled_for_generators.end()){
				continue;
			}
			if(std::find(s.getMemberVariables()[i].second.disabled_for_generators.begin(), s.getMemberVariables()[i].second.disabled_for_generators.end(), name) != s.getMemberVariables()[i].second.disabled_for_generators.end()){
				continue;
			}
		}

		// Skip array fields - they don't create columns in the parent table
		if (s.getMemberVariables()[i].second.type.is_array())
		{
			continue;
		}
		
		if (!first_column)
		{
			sql += ",\n";
		}
		first_column = false;
		
		sql += "\t" + s.getMemberVariables()[i].second.identifier + " ";
		// add type
		sql+= convert_to_local_type(ps, s.getMemberVariables()[i].second.type);
		// add constraints
		if (s.getMemberVariables()[i].second.required)
		{
			sql += " NOT NULL";
		}
		if (s.getMemberVariables()[i].second.unique)
		{
			sql += " UNIQUE";
		}
		if (s.getMemberVariables()[i].second.primary_key)
		{
			sql += " PRIMARY KEY";
		}
		if (!s.getMemberVariables()[i].second.reference.variable_name.empty())
		{
			sql += " REFERENCES " + s.getMemberVariables()[i].second.reference.struct_name + "(" + s.getMemberVariables()[i].second.reference.variable_name + ")";
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
		for (auto & [generator, member_var] : parent_struct.getMemberVariables())
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
							reference_column.required = member_var.required; // If array is required, reference is NOT NULL
							reference_column.reference.struct_name = parent_struct.getIdentifier();
							reference_column.reference.variable_name = "id"; // Assuming parent has 'id' as primary key
							reference_column.description = "Foreign key reference to " + parent_struct.getIdentifier() + " table";
							
							// Check if this foreign key column already exists
							bool reference_exists = false;
							for (auto &[generator,existing_var] : target_struct.getMemberVariables())
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

std::string SqliteGenerator::generate_select_all_statement_string_member_variable(StructDefinition &s, MemberVariableDefinition &mv)
{
	std::string sql = "SELECT * FROM " + s.getIdentifier() + " WHERE " + mv.identifier + " = ?;";
	return sql;
}

std::vector<std::string> SqliteGenerator::genrate_select_all_statements_string_struct(StructDefinition &s)
{
	std::vector<std::string> sqls;
	for (int i = 0; i < s.getMemberVariables().size(); i++)
	{
		if (s.getMemberVariables()[i].second.primary_key)
		{
			continue;
		}
		sqls.push_back(generate_select_all_statement_string_member_variable(s, s.getMemberVariables()[i].second));
	}
	return sqls;
}

std::string SqliteGenerator::generate_select_by_member_variable_statement_string(StructDefinition &s, MemberVariableDefinition &mv_1, std::vector<int> &criteria)
{
	std::string sql = "SELECT " + mv_1.identifier + " FROM " + s.getIdentifier() + " WHERE ";
	for (int i = 0; i < criteria.size(); i++)
	{
		MemberVariableDefinition mv = s.getMemberVariables()[criteria[i]].second;
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
	std::string sql = "INSERT INTO " + s.getIdentifier() + " (";
	bool first = true;
	for (int i = 0; i < s.getMemberVariables().size(); i++)
	{
		// Skip array fields - arrays are handled by foreign key relationships
		if (s.getMemberVariables()[i].second.type.is_array())
		{
			continue;
		}
		// Skip reference fields - TODO: Update to handle reference field updates properly
		if (!s.getMemberVariables()[i].second.reference.variable_name.empty())
		{
			continue;
		}
		if (!first)
		{
			sql += ", ";
		}
		sql += s.getMemberVariables()[i].second.identifier;
		first = false;
	}
	sql += ") VALUES (";
	first = true;
	for (int i = 0; i < s.getMemberVariables().size(); i++)
	{
		// Skip array fields - arrays are handled by foreign key relationships
		if (s.getMemberVariables()[i].second.type.is_array())
		{
			continue;
		}
		// Skip reference fields - TODO: Update to handle reference field updates properly
		if (!s.getMemberVariables()[i].second.reference.variable_name.empty())
		{
			continue;
		}
		if (!first)
		{
			sql += ", ";
		}
		sql += "?";
		first = false;
	}
	sql += ")";
	return sql;
}

std::string SqliteGenerator::generate_update_all_statement_string_struct(StructDefinition &s)
{
	std::string sql = "UPDATE " + s.getIdentifier() + " SET ";
	for (int i = 0; i < s.getMemberVariables().size(); i++)
	{
		if (s.getMemberVariables()[i].second.primary_key)
		{
			continue;
		}
		// Skip array fields - arrays are handled by foreign key relationships
		if (s.getMemberVariables()[i].second.type.is_array())
		{
			continue;
		}
		// Skip reference fields - TODO: Update to handle reference field updates properly
		if (!s.getMemberVariables()[i].second.reference.variable_name.empty())
		{
			continue;
		}
		sql += s.getMemberVariables()[i].second.identifier + " = ?";
		if (i < s.getMemberVariables().size() - 1)
		{
			sql += ", ";
		}
	}
	sql += " WHERE ";
	bool has_primary_key = false;
	for (int i = 0; i < s.getMemberVariables().size(); i++)
	{
		if (s.getMemberVariables()[i].second.primary_key)
		{
			sql += s.getMemberVariables()[i].second.identifier + " = ?";
			has_primary_key = true;
			break;
		}
	}
	if (!has_primary_key)
	{
		sql += s.getMemberVariables()[0].second.identifier + " = ?";
	}
	return sql;
}

std::string SqliteGenerator::generate_delete_statement_string_struct(StructDefinition &s)
{
	std::string sql = "DELETE FROM " + s.getIdentifier() + " WHERE ";
	bool has_primary_key = false;
	for (int i = 0; i < s.getMemberVariables().size(); i++)
	{
		if (s.getMemberVariables()[i].second.primary_key)
		{
			sql += s.getMemberVariables()[i].second.identifier + " = ?";
			has_primary_key = true;
			break;
		}
	}
	if (!has_primary_key)
	{
		sql += s.getMemberVariables()[0].second.identifier + " = ?";
	}
	return sql;
}

std::string generate_bind(Generator *gen,ProgramStructure *ps, MemberVariableDefinition mv, int i)
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
		if(mv.required){
			ret+= "int(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ");\n";
		}else{
			ret+= "int(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".has_value() ? " + mv.identifier + ".value() : 0);\n";
		}
	}
	else if (mv.type.is_real())
	{
		if(mv.required){
			ret+= "double(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ");\n";
		}
		else{
			ret+= "double(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".has_value() ? " + mv.identifier + ".value() : 0.0);\n";
		}
	}
	else if (mv.type.is_bool())
	{
		if(mv.required){
			ret+= "int(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ");\n";
		}
		else{
			ret+= "int(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".has_value() ? " + mv.identifier + ".value() : 0);\n";
		}
	}
	else if (mv.type.is_string())
	{
		if(mv.required){
			ret+= "text(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".c_str(), -1, SQLITE_STATIC);\n";
		}else{
			ret+= "text(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".has_value() ? " + mv.identifier + ".value().c_str() : nullptr, -1, SQLITE_STATIC);\n";
		}
	}
	else if (mv.type.is_char())
	{
		if(mv.required){
			ret+= "text(stmt, " + std::to_string(i + 1) + ", std::string(1, " + mv.identifier + ").c_str(), -1, SQLITE_STATIC);\n";
		}else{
			ret+= "text(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".has_value() ? std::string(1, " + mv.identifier + ".value()).c_str() : nullptr, -1, SQLITE_STATIC);\n";
		}
	}
	else if (mv.type.is_array())
	{
		if(mv.required){
			ret+= "text(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".c_str());\n";
		}
		else{
			ret+= "text(stmt, " + std::to_string(i + 1) + ", " + mv.identifier + ".has_value() ? " + mv.identifier + ".value().c_str() : nullptr);\n";
		}
	}
	else if (!mv.required){
		throw std::runtime_error("Optional types are not supported in SQLite generator.");
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
	select_all_statement.return_type.identifier() = "std::vector<" + s.getIdentifier() + "Schema*>";
	select_all_statement.static_function = true;
	select_all_statement.parameters.push_back(std::make_pair(sqlite_db, "db"));
	if(mv.required){
		select_all_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));
	}else{
		select_all_statement.parameters.push_back(std::make_pair(TypeDefinition("std::optional<" + gen->convert_to_local_type(ps, mv.type) + ">"), mv.identifier));
	}

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

		structFile << "\tstd::vector<" << s.getIdentifier() << "Schema*> results;\n";
		structFile << "\tchar *zErrMsg = 0;\n";
		structFile << "\tstd::string sql = \"";
		structFile << generate_select_all_statement_string_member_variable(s, mv);
		structFile << "\";\n";
		structFile << "\tsqlite3_stmt *stmt;\n";
		structFile << "\tsqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);\n";
		structFile << generate_bind(gen,ps,mv, 0);
		structFile << "\twhile(sqlite3_step(stmt) == SQLITE_ROW){\n";
		structFile << "\t\t" << s.getIdentifier() << "Schema* result = new " << s.getIdentifier() << "Schema();\n";
		for (auto &[generator,mv_field] : s.getMemberVariables())
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
				//std::string local_type = gen->convert_to_local_type(ps, mv_field.type);
				//local_type = local_type.substr(0, local_type.size() - 2);
				structFile << "\t\tresult->set" << mv_field.identifier << "(sqlite3_column_int(stmt, " << mv_field.identifier << "_index));\n";
			}
			else if (mv_field.type.is_real())
			{
				structFile << "\t\tresult->set" << mv_field.identifier << "(sqlite3_column_double(stmt, " << mv_field.identifier << "_index));\n";
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
	s.add_function(select_all_statement);
}

void SqliteGenerator::generate_select_all_statement_functions_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	for (int i = 0; i < s.getMemberVariables().size(); i++)
	{
		generate_select_all_statement_function_member_variable(gen, ps, s, s.getMemberVariables()[i].second);
	}
}

void SqliteGenerator::generate_select_member_variable_function_statement(Generator *gen, ProgramStructure *ps, StructDefinition &s, MemberVariableDefinition &mv_1, std::vector<int> &criteria)
{
	FunctionDefinition select_statement;
	select_statement.generator = "SQLite";
	select_statement.identifier = "SQLiteSelect" + s.getIdentifier() + "By";
	select_statement.return_type = mv_1.type;
	for (int i = 0; i < criteria.size(); i++)
	{
		MemberVariableDefinition mv = s.getMemberVariables()[criteria[i]].second;
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
			structFile << generate_bind(gen,ps,s.getMemberVariables()[criteria[i]].second, i);
		}
		structFile << "\tif(sqlite3_step(stmt) == SQLITE_ROW){\n";
		structFile << "\t\t" << mv_1.identifier << " = sqlite3_column_" << mv_1.type.identifier() << "(stmt, 0);\n";
		structFile << "\t}\n";
		structFile << "\tsqlite3_finalize(stmt);\n";
		return true;
	};
	s.add_function(select_statement);
}

void SqliteGenerator::generate_select_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	generate_select_all_statement_functions_struct(gen, ps, s);
	// std::vector<std::vector<int>> combinations = comb(s.getMemberVariables().size());
	// for (int i = 0; i < s.getMemberVariables().size(); i++)
	// {
	// 	if (s.getMemberVariables()[i].second.primary_key)
	// 	{
	// 		continue;
	// 	}
	// 	for (int j = 0; j < combinations.size(); j++)
	// 	{
	// 		sqls.push_back(generate_select_member_variable_statement(s, s.getMemberVariables()[i].second, combinations[j]));
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
	
	// First, add all required parameters
	for (auto& [generator, mv] : s.getMemberVariables())
	{
		// Skip array fields - arrays are handled by foreign key relationships
		if (mv.type.is_array())
		{
			continue;
		}
		// Skip reference fields - TODO: Update to handle reference field updates properly
		if (!mv.reference.variable_name.empty())
		{
			continue;
		}
		// Only add required parameters in this pass
		if (mv.required)
		{
			insert_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));
		}
	}
	
	// Then, add all optional parameters with default values
	for (auto& [generator, mv] : s.getMemberVariables())
	{
		// Skip array fields - arrays are handled by foreign key relationships
		if (mv.type.is_array())
		{
			continue;
		}
		// Skip reference fields - TODO: Update to handle reference field updates properly
		if (!mv.reference.variable_name.empty())
		{
			continue;
		}
		// Only add optional parameters in this pass
		if (!mv.required)
		{
			std::string param_type = "std::optional<" + gen->convert_to_local_type(ps, mv.type) + ">";
			insert_statement.parameters.push_back(std::make_pair(TypeDefinition(param_type,true), mv.identifier));
		}
	}
	insert_statement.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\tchar *zErrMsg = 0;\n";
		structFile << "\tstd::string sql = \"";
		structFile << generat_insert_statement_string_struct(s);
		structFile << "\";\n";
		structFile << "\tsqlite3_stmt *stmt;\n";
		structFile << "\tsqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);\n";
		for (int i = 0; i < s.getMemberVariables().size(); i++)
		{
			if (s.getMemberVariables()[i].second.type.is_enum(ps))
			{
				continue;
			}
			if (s.getMemberVariables()[i].second.type.is_struct(ps))
			{
				continue;
			}
			if (s.getMemberVariables()[i].second.type.is_array())
			{
				continue;
			}
			// Skip reference fields - TODO: Update to handle reference field updates properly
			if (!s.getMemberVariables()[i].second.reference.variable_name.empty())
			{
				continue;
			}
			structFile << generate_bind(gen,ps,s.getMemberVariables()[i].second, i);
		}
		structFile << "\tif(sqlite3_step(stmt) != SQLITE_DONE){\n";
		structFile << "\t\tsqlite3_finalize(stmt);\n";
		structFile << "\t\treturn false;\n";
		structFile << "\t}\n";
		structFile << "\tsqlite3_finalize(stmt);\n";
		structFile << "\treturn true;\n";
		return true;
	};
	s.add_function(insert_statement);

	FunctionDefinition insert_statement_no_args;
	insert_statement_no_args.generator = "SQLite";
	insert_statement_no_args.identifier = "SQLiteInsert";
	insert_statement_no_args.return_type.identifier() = BOOL;
	insert_statement_no_args.static_function = false;
	insert_statement_no_args.parameters.push_back(std::make_pair(TypeDefinition("sqlite3 *"), "db"));
	insert_statement_no_args.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\tchar *zErrMsg = 0;\n";
		structFile << "\tstd::string sql = \"";
		structFile << generat_insert_statement_string_struct(s);
		structFile << "\";\n";
		structFile << "\tsqlite3_stmt *stmt;\n";
		structFile << "\tsqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);\n";
		for (int i = 0; i < s.getMemberVariables().size(); i++)
		{
			if (s.getMemberVariables()[i].second.type.is_enum(ps))
			{
				continue;
			}
			if (s.getMemberVariables()[i].second.type.is_struct(ps))
			{
				continue;
			}
			if (s.getMemberVariables()[i].second.type.is_array())
			{
				continue;
			}
			// Skip reference fields - TODO: Update to handle reference field updates properly
			if (!s.getMemberVariables()[i].second.reference.variable_name.empty())
			{
				continue;
			}
			structFile << generate_bind(gen,ps,s.getMemberVariables()[i].second, i);
		}
		structFile << "\tif(sqlite3_step(stmt) != SQLITE_DONE){\n";
		structFile << "\t\tsqlite3_finalize(stmt);\n";
		structFile << "\t\treturn false;\n";
		structFile << "\t}\n";
		structFile << "\tsqlite3_finalize(stmt);\n";
		structFile << "\treturn true;\n";
		return true;
	};
	s.add_function(insert_statement_no_args);
}

void SqliteGenerator::generate_update_all_statement_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	FunctionDefinition update_all_statement;
	update_all_statement.generator = "SQLite";
	update_all_statement.identifier = "SQLiteUpdate" + s.getIdentifier();
	update_all_statement.return_type.identifier() = BOOL;
	for (auto& [generator, mv] : s.getMemberVariables())
	{
		// Skip array fields - arrays are handled by foreign key relationships
		if (mv.type.is_array())
		{
			continue;
		}
		// Skip reference fields - TODO: Update to handle reference field updates properly
		if (!mv.reference.variable_name.empty())
		{
			continue;
		}
		// For optional fields, use std::optional<T> with default std::nullopt
		if (!mv.required)
		{
			std::string param_type = "std::optional<" + gen->convert_to_local_type(ps, mv.type) + ">";
			update_all_statement.parameters.push_back(std::make_pair(TypeDefinition(param_type,true), mv.identifier));
		}
		else
		{
			update_all_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));
		}
	}
	update_all_statement.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\tchar *zErrMsg = 0;\n";
		structFile << "\tstd::string sql = \"";
		structFile << generate_update_all_statement_string_struct(s);
		structFile << "\";\n";
		structFile << "\tsqlite3_stmt *stmt;\n";
		structFile << "\tsqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);\n";
		for (int i = 0; i < s.getMemberVariables().size(); i++)
		{
			structFile << generate_bind(gen,ps,s.getMemberVariables()[i].second, i);
		}
		structFile << "\tif(sqlite3_step(stmt) != SQLITE_DONE){\n";
		structFile << "\t\tsqlite3_finalize(stmt);\n";
		structFile << "\t\treturn false;\n";
		structFile << "\t}\n";
		structFile << "\tsqlite3_finalize(stmt);\n";
		structFile << "\treturn true;\n";
		return true;
	};
	s.add_function(update_all_statement);
}

void SqliteGenerator::generate_update_statements_function_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	FunctionDefinition update_statement;
	update_statement.generator = "SQLite";
	update_statement.identifier = "SQLiteUpdate";
	update_statement.return_type.identifier() = "bool";
	update_statement.static_function = true;
	update_statement.parameters.push_back(std::make_pair(sqlite_db, "db"));
	
	// First, add all required parameters
	for (auto& [generator, mv] : s.getMemberVariables())
	{
		if (mv.type.is_array())
		{
			continue;
		}
		// Skip reference fields - TODO: Update to handle reference field updates properly
		if (!mv.reference.variable_name.empty())
		{
			continue;
		}
		// Only add required parameters in this pass
		if (mv.required)
		{
			update_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, mv.type), mv.identifier));
		}
	}
	
	// Then, add all optional parameters with default values
	for (auto& [generator, mv] : s.getMemberVariables())
	{
		if (mv.type.is_array())
		{
			continue;
		}
		// Skip reference fields - TODO: Update to handle reference field updates properly
		if (!mv.reference.variable_name.empty())
		{
			continue;
		}
		// Only add optional parameters in this pass
		if (!mv.required)
		{
			std::string param_type = "std::optional<" + gen->convert_to_local_type(ps, mv.type) + ">";
			update_statement.parameters.push_back(std::make_pair(TypeDefinition(param_type,true), mv.identifier));
		}
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
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			if (mv.type.is_array() || mv.primary_key)
			{
				continue;
			}
			// Skip reference fields - TODO: Update to handle reference field updates properly
			if (!mv.reference.variable_name.empty())
			{
				continue;
			}
			structFile << generate_bind(gen, ps, mv, param_index - 1);
			param_index++;
		}
		
		// Bind primary key parameter for WHERE clause
		for (auto& [generator, mv] : s.getMemberVariables())
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
	s.add_function(update_statement);

	// Instance method version
	FunctionDefinition update_statement_no_args;
	update_statement_no_args.generator = "SQLite";
	update_statement_no_args.identifier = "SQLiteUpdate";
	update_statement_no_args.return_type.identifier() = "bool";
	update_statement_no_args.static_function = false;
	update_statement_no_args.generate_function = [this](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
				structFile << "\tchar *zErrMsg = 0;\n";
		structFile << "\tstd::string sql = \"";
		structFile << generate_update_all_statement_string_struct(s);
		structFile << "\";\n";
		structFile << "\tsqlite3_stmt *stmt;\n";
		structFile << "\tsqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);\n";
		
		int param_index = 1;
		// Bind all non-array parameters for SET clause
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			if (mv.type.is_array() || mv.primary_key)
			{
				continue;
			}
			// Skip reference fields - TODO: Update to handle reference field updates properly
			if (!mv.reference.variable_name.empty())
			{
				continue;
			}
			structFile << generate_bind(gen, ps, mv, param_index - 1);
			param_index++;
		}
		
		// Bind primary key parameter for WHERE clause
		for (auto& [generator, mv] : s.getMemberVariables())
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
	s.add_function(update_statement_no_args);
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
	for (auto& [generator, mv] : s.getMemberVariables())
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
		delete_statement.parameters.push_back(std::make_pair(gen->convert_to_local_type(ps, s.getMemberVariables()[0].second.type), s.getMemberVariables()[0].second.identifier));
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
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			if (mv.primary_key)
			{
				structFile << generate_bind(gen, ps, mv, 0);
				break;
			}
		}
		if (!s.getMemberVariables().empty() && !s.getMemberVariables()[0].second.primary_key)
		{
			// If no primary key found, use first field
			bool has_primary_key = false;
			for (auto& [generator, mv] : s.getMemberVariables())
			{
				if (mv.primary_key)
				{
					has_primary_key = true;
					break;
				}
			}
			if (!has_primary_key)
			{
				structFile << generate_bind(gen, ps, s.getMemberVariables()[0].second, 0);
			}
		}
		
		structFile << "\tint result = sqlite3_step(stmt);\n";
		structFile << "\tsqlite3_finalize(stmt);\n";
		structFile << "\treturn result == SQLITE_DONE;\n";
		return true;
	};
	s.add_function(delete_statement);

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
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			if (mv.primary_key)
			{
				structFile << ", " << mv.identifier;
				break;
			}
		}
		// If no primary key, use first field
		if (!s.getMemberVariables().empty())
		{
			bool has_primary_key = false;
			for (auto& [generator, mv] : s.getMemberVariables())
			{
				if (mv.primary_key)
				{
					has_primary_key = true;
					break;
				}
			}
			if (!has_primary_key)
			{
				structFile << ", " << s.getMemberVariables()[0].second.identifier;
			}
		}
		structFile << ");\n";
		return true;
	};
	s.add_function(delete_statement_no_args);
}

// file generation functions
bool SqliteGenerator::generate_create_table_file(ProgramStructure *ps, StructDefinition &s, std::string out_path)
{
	std::ofstream structFile(out_path + "/" + s.getIdentifier() + "_create_table.sql");
	if (!structFile.is_open())
	{
		std::cout << "Failed to open file: " << out_path + "/" + s.getIdentifier() + "_create_table.sql" << std::endl;
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
		std::ofstream structFile(out_path + "/" + s.getIdentifier() + "_select_all_" + s.getMemberVariables()[i].second.identifier + ".sql");
		if (!structFile.is_open())
		{
			std::cout << "Failed to open file: " << out_path + "/" + s.getIdentifier() + "_select_all_" + s.getMemberVariables()[i].second.identifier + ".sql" << std::endl;
			return false;
		}
		structFile << sqls[i] << std::endl;
		structFile.close();
	}
	return true;
}

bool SqliteGenerator::generate_select_files(ProgramStructure *ps, StructDefinition &s, std::string out_path)
{
	std::vector<std::vector<int>> combinations = comb(s.getMemberVariables().size());
	for (int i = 0; i < s.getMemberVariables().size(); i++)
	{
		if (s.getMemberVariables()[i].second.primary_key)
		{
			continue;
		}
		for (int j = 0; j < combinations.size(); j++)
		{
			std::ofstream structFile(out_path + "/" + s.getIdentifier() + "_select_" + s.getMemberVariables()[i].second.identifier + "_by_");
			for (int k = 0; k < combinations[j].size(); k++)
			{
				structFile << s.getMemberVariables()[combinations[j][k]].second.identifier;
				if (k < combinations[j].size() - 1)
				{
					structFile << "_";
				}
			}
			structFile << ".sql";
			if (!structFile.is_open())
			{
				std::cout << "Failed to open file: " << out_path + "/" + s.getIdentifier() + "_select_" + s.getMemberVariables()[i].second.identifier + "_by_";
				for (int k = 0; k < combinations[j].size(); k++)
				{
					structFile << s.getMemberVariables()[combinations[j][k]].second.identifier;
					if (k < combinations[j].size() - 1)
					{
						structFile << "_";
					}
				}
				structFile << ".sql" << std::endl;
				return false;
			}
			structFile << generate_select_by_member_variable_statement_string(s, s.getMemberVariables()[i].second, combinations[j]) << std::endl;
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
		std::cout << "Failed to generate create table file for struct: " << s.getIdentifier() << std::endl;
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
	// base_class.getIdentifier() = "Sqlite";
	// base_class.add_include("<sqlite3.h>");
	// base_class.add_include("<iostream>");
	// base_class.add_include("<string>");
	// base_class.add_include("<vector>");
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
		s.add_include("<sqlite3.h>","SQLite");
		s.add_include("<iostream>","SQLite");
		s.add_include("<string>","SQLite");
		s.add_include("<vector>","SQLite");

		// add index private variables for each member variable
		bool has_primary_key = false;

		int i = 0;
		for (auto& [generator, mv] : s.getMemberVariables())
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
			s.add_private_variable(index,"SQLite");
			i++;
		}

		PrivateVariableDefinition database;
		database.type = TypeDefinition("sqlite3 *");
		database.identifier = "db";
		database.static_member = true;
		s.add_private_variable(database,"SQLite");

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
		s.add_function(getCreateTableStatement);

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
		s.add_function(createTable);

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
		s.add_function(registerUpdateListener);

		s.add_before_setter_line("if(!SQLiteUpdate()){\n"
			"\t\tstd::cerr << \"Failed to update " + s.getIdentifier() + " in SQLite database.\" << std::endl;\n"
			"\t\treturn;\n"
			"\t}\n",name);
	}else{
		std::cout << "Warning: SqliteGenerator only supports C++ code generation. cant add generator specific content to struct for generator: " << gen->name << std::endl;
	}

	return true;
}

#include <inja/inja.hpp>
#include <EmbeddedResources/EmbeddedResourcesEmbeddedVFS.hpp>

bool SqliteGenerator::generate_files(ProgramStructure ps, std::string out_path)
{
	if (!std::filesystem::exists(out_path))
	{
		std::filesystem::create_directories(out_path);
	}

	// Add foreign key columns for array relationships before generating files
	add_foreign_key_columns_for_arrays(&ps);

	// for (auto &s : ps.getStructs())
	// {
	// 	if (!generate_struct_files(&ps, s, out_path))
	// 	{
	// 		std::cout << "Failed to generate sqlite file for struct: " << s.getIdentifier() << std::endl;
	// 		return false;
	// 	}
	// }

	inja::Environment env;
	//env.set_trim_blocks(true);
	//env.set_lstrip_blocks(false);

	std::map<std::string, std::string> struct_name_content_pairs;
	// open file
	std::vector<std::string> files = listEmbeddedResourcesEmbeddedFiles("/SQLite/struct/");
	for (auto &file : files)
	{
		std::string content(reinterpret_cast<const char *>(loadEmbeddedResourcesEmbeddedFile(("/SQLite/struct/" + file).c_str()).data()), loadEmbeddedResourcesEmbeddedFile(("/SQLite/struct/" + file).c_str()).size());
		std::string filename = std::filesystem::path(file).filename().string();
		struct_name_content_pairs[filename] = content;
	}

	std::map<std::string, std::string> enum_name_content_pairs;
	files = listEmbeddedResourcesEmbeddedFiles("/SQLite/enum/");
	for (auto &file : files)
	{
		std::string content(reinterpret_cast<const char *>(loadEmbeddedResourcesEmbeddedFile(("/SQLite/enum/" + file).c_str()).data()), loadEmbeddedResourcesEmbeddedFile(("/SQLite/enum/" + file).c_str()).size());
		std::string filename = std::filesystem::path(file).filename().string();
		enum_name_content_pairs[filename] = content;
	}

	for (auto &s : ps.getStructs())
	{
		inja::json data;
		data["struct"] = s.getIdentifier();
		data["fields"] = inja::json::array();
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			// Create fields data for SQLite template compatibility
			inja::json field_data;
			field_data["name"] = mv.identifier;
			field_data["enabled"] = true;
			if(!mv.enabled_for_generators.empty() || !mv.disabled_for_generators.empty()){
				if(std::find(mv.enabled_for_generators.begin(), mv.enabled_for_generators.end(), name) != mv.enabled_for_generators.end()){
					field_data["enabled"] = true;
				}else{
					field_data["enabled"] = false;
				}

				if(std::find(mv.disabled_for_generators.begin(), mv.disabled_for_generators.end(), name) != mv.disabled_for_generators.end()){
					field_data["enabled"] = false;
				}
			}
			bool convert_to_reference = mv.type.is_struct(&ps)|| mv.type.is_enum(&ps)|| (mv.type.is_array() && (mv.type.element_type().is_struct(&ps)|| mv.type.element_type().is_enum(&ps)));
			field_data["convert_to_reference"] = convert_to_reference;
			if(convert_to_reference){
				if(mv.type.is_array()){
					field_data["type"] = mv.type.element_type().identifier();
				}else if(mv.type.is_struct(&ps)|| mv.type.is_enum(&ps)){
					field_data["type"] = mv.type.identifier();
				}
			}else{
				field_data["type"] = convert_to_local_type(&ps, mv.type);
			}
			field_data["required"] = mv.required;
			field_data["unique"] = mv.unique;
			field_data["primary_key"] = mv.primary_key;
			field_data["auto_increment"] = mv.auto_increment;
			
			// Handle reference data
			if((mv.reference.struct_name.empty() || mv.reference.variable_name.empty())&&!convert_to_reference)
			{
				field_data["reference"] = false; // Empty object if no reference
			}else
			{
				inja::json reference_data;
				if(convert_to_reference){
					if(mv.type.is_array()){
						reference_data["struct_name"] = mv.type.element_type().identifier();
					}else if(mv.type.is_struct(&ps)|| mv.type.is_enum(&ps)){
						reference_data["struct_name"] = mv.type.identifier();
					} 
					reference_data["variable_name"] = "id"; // Use the member variable identifier as the reference variable name
				}else{
					reference_data["struct_name"] = mv.reference.struct_name;
					reference_data["variable_name"] = mv.reference.variable_name;
				}
				field_data["reference"] = reference_data;
			}
			
			if (!mv.default_value.empty())
			{
				field_data["default_value"] = mv.default_value;
			}else
			{
				field_data["default_value"] = false; // Default to false if no default value is set
			}
			
			data["fields"].push_back(field_data);
		}

		try
		{
			for (auto &file : struct_name_content_pairs)
			{
				std::ofstream of(env.render(out_path + "/" + file.first, data));
				if (!of.is_open())
				{
					std::cout << "Failed to open file: " << env.render(out_path + "/" + file.first, data);
				}
				of << env.render(file.second, data);
				of.close();
				std::cout << "Generated file: " << env.render(out_path + "/" + file.first, data) << std::endl;
			}
		}
		catch (const std::exception &e)
		{
			std::cout << "Error generating file for struct " << s.getIdentifier() << ": " << e.what() << std::endl;
			return false;
		}
	}

	for (auto &e : ps.getEnums())
	{
		inja::json data;
		data["enum"] = e.identifier;

		data["values"] = inja::json::array();
		for (auto &v : e.values)
		{
			inja::json value_data;
			value_data["name"] = v.first;
			value_data["value"] = v.second;
			data["values"].push_back(value_data);
		}

		try
		{
			for (auto &file : enum_name_content_pairs)
			{
				std::ofstream of(env.render(out_path + "/" + file.first, data));
				if (!of.is_open())
				{
					std::cout << "Failed to open file: " << env.render(out_path + "/" + file.first, data);
				}
				of << env.render(file.second, data);
				of.close();
				std::cout << "Generated file: " << env.render(out_path + "/" + file.first, data) << std::endl;
			}
		}
		catch (const std::exception &ex)
		{
			std::cout << "Error generating file for enum " << e.identifier << ": " << ex.what() << std::endl;
			return false;
		}
	}

	return true;
}