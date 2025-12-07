#include {{format_include("SQLiteQueryBuilder.hpp")}}
#include {{format_include("SQLiteDB.hpp")}}
{% for include in includes %}
#include {{include}}
{% endfor %}

// Specializations for each struct type
{% for struct in structs %}
template<>
std::vector<std::shared_ptr<{{struct.identifier}}Schema>> SQLiteQueryBuilder::Exec<{{struct.identifier}}Schema>() {
    if (!db->isConnected()) {
        throw std::runtime_error("SQLiteQueryBuilder::Exec() - Database not connected");
    }
    
    std::vector<std::shared_ptr<{{struct.identifier}}Schema>> results;
    std::string sql = BuildQuery();
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db->getDB(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("SQLiteQueryBuilder::Exec() - Failed to prepare statement: " + std::string(sqlite3_errmsg(db->getDB())) + "\nSQL: " + sql);
    }
    
    BindParameters(stmt);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        auto obj = std::make_shared<{{struct.identifier}}Schema>();
        int col = 0;
{% for field in struct.member_variables %}
{% if not field.type.is_array %}
        // Set {{field.identifier}}
{% if field.type.required %}
{% if field.type.identifier == "string" or field.type.identifier == "std::string" %}
        const char* {{field.identifier}}_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col++));
        if ({{field.identifier}}_text) {
            obj->set{{field.identifierCamel}}(std::string({{field.identifier}}_text));
        }
{% else if field.type.identifier == "int64_t" or field.type.identifier == "long" %}
        obj->set{{field.identifierCamel}}(sqlite3_column_int64(stmt, col++));
{% else if field.type.is_integer %}
        obj->set{{field.identifierCamel}}(sqlite3_column_int(stmt, col++));
{% else if field.type.is_real %}
        obj->set{{field.identifierCamel}}(sqlite3_column_double(stmt, col++));
{% else if field.type.is_bool %}
        obj->set{{field.identifierCamel}}(sqlite3_column_int(stmt, col++) != 0);
{% else if field.type.is_enum %}
        obj->set{{field.identifierCamel}}(static_cast<{{field.type.identifier}}Schema>(sqlite3_column_int(stmt, col++)));
{% else if field.type.is_struct %}
        // If this is a struct, we need to handle it differently
        int64_t {{field.identifier}}_id_value = sqlite3_column_int64(stmt, col++);
        if ({{field.identifier}}_id_value > 0) {
            obj->set{{field.identifierCamel}}(db->select{{field.type.identifier}}ById({{field.identifier}}_id_value));
        }
{% else %}
        // Need to handle type {{field.type.identifier}} here in generated code
        // For now, just skip it
        col++;
{% endif %}
{% else %}
        if(sqlite3_column_type(stmt, col)!=SQLITE_NULL){
{% if field.type.identifier == "string" or field.type.identifier == "std::string" %}
        const char* {{field.identifier}}_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col++));
        if ({{field.identifier}}_text) {
            obj->set{{field.identifierCamel}}(std::string({{field.identifier}}_text));
        }
{% else if field.type.identifier == "int64_t" or field.type.identifier == "long" %}
            obj->set{{field.identifierCamel}}(sqlite3_column_int64(stmt, col++));
{% else if field.type.is_integer %}
            obj->set{{field.identifierCamel}}(sqlite3_column_int(stmt, col++));
{% else if field.type.is_real %}
            obj->set{{field.identifierCamel}}(sqlite3_column_double(stmt, col++));
{% else if field.type.is_bool %}
            obj->set{{field.identifierCamel}}(sqlite3_column_int(stmt, col++) != 0);
{% else if field.type.is_enum %}
            obj->set{{field.identifierCamel}}(static_cast<{{field.type.identifier}}Schema>(sqlite3_column_int(stmt, col++)));
{% else if field.type.is_struct %}
            // If this is a struct, we need to handle it differently
            int64_t {{field.identifier}}_id_value = sqlite3_column_int64(stmt, col++);
            if ({{field.identifier}}_id_value > 0) {
                obj->set{{field.identifierCamel}}(db->select{{field.type.identifier}}ById({{field.identifier}}_id_value));
            }
{% else %}
            // Need to handle type {{field.type.identifier}} here in generated code
            // For now, just skip it
            col++;
{% endif %}
        }
        else
        {
            obj->set{{field.identifierCamel}}(std::nullopt);
            col++;
        }
{% endif %}
{% endif %}
{% endfor %}

        //get primitive arrays if any
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
        {
            const char* array_sql = "SELECT value FROM {{struct.identifier}}_{{mv.identifier}} WHERE {{struct.identifier}}_id = ? ORDER BY sequence";
            sqlite3_stmt* array_stmt;
            if (sqlite3_prepare_v2(db->getDB(), array_sql, -1, &array_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int64(array_stmt, 1, obj->getId());
                while (sqlite3_step(array_stmt) == SQLITE_ROW) {
{% if mv.type.elem_type.is_string %}
                    const char* value_text = reinterpret_cast<const char*>(sqlite3_column_text(array_stmt, 0));
                    if (value_text) {
                        obj->addTo{{mv.identifierCamel}}(std::string(value_text));
                    }
{% else if mv.type.elem_type.identifier == "int64_t" or mv.type.elem_type.identifier == "long" %}
                    obj->addTo{{mv.identifierCamel}}(sqlite3_column_int64(array_stmt, 0));
{% else if mv.type.elem_type.is_integer %}
                    obj->addTo{{mv.identifierCamel}}(sqlite3_column_int(array_stmt, 0));
{% else if mv.type.elem_type.is_real %}
                    obj->addTo{{mv.identifierCamel}}(static_cast<{{mv.type.elem_type.estimated}}>(sqlite3_column_double(array_stmt, 0)));
{% else if mv.type.elem_type.is_bool %}
                    obj->addTo{{mv.identifierCamel}}(sqlite3_column_int(array_stmt, 0) != 0);
{% else if mv.type.elem_type.is_char %}
                    const char* char_text = reinterpret_cast<const char*>(sqlite3_column_text(array_stmt, 0));
                    if (char_text && char_text[0]) {
                        obj->addTo{{mv.identifierCamel}}(char_text[0]);
                    }
{% endif %}
                }
                sqlite3_finalize(array_stmt);
            }
        }
{% endif %}
{% endfor %}

        results.push_back(obj);
    }
    
    sqlite3_finalize(stmt);
    return results;
}

template<>
std::shared_ptr<{{struct.identifier}}Schema> SQLiteQueryBuilder::First<{{struct.identifier}}Schema>() {
    Limit(1);
    auto results = Exec<{{struct.identifier}}Schema>();
    return results.empty() ? nullptr : results[0];
}
{% endfor %}

bool SQLiteQueryBuilder::Exists() {
    std::string sql = BuildQuery("1");
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db->getDB(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("SQLiteQueryBuilder::Exists() - Failed to prepare statement: " + std::string(sqlite3_errmsg(db->getDB())) + "\nSQL: " + sql);
    }
    
    BindParameters(stmt);
    
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    
    return exists;
}

int SQLiteQueryBuilder::Count() {
    std::string sql = BuildQuery("COUNT(*)");
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db->getDB(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("SQLiteQueryBuilder::Count() - Failed to prepare statement: " + std::string(sqlite3_errmsg(db->getDB())) + "\nSQL: " + sql);
    }
    
    BindParameters(stmt);
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    return count;
}

std::vector<SqliteResult> SQLiteQueryBuilder::ExecCustom() {
    if (!db->isConnected()) {
        throw std::runtime_error("SQLiteQueryBuilder::ExecCustom() - Database not connected");
    }
    
    std::vector<SqliteResult> results;
    std::string sql = BuildQuery();
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db->getDB(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("SQLiteQueryBuilder::ExecCustom() - Failed to prepare statement: " + std::string(sqlite3_errmsg(db->getDB())) + "\nSQL: " + sql);
    }
    
    BindParameters(stmt);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SqliteResult result;
        int columnCount = sqlite3_column_count(stmt);
        
        for (int i = 0; i < columnCount; ++i) {
            const char* columnName = sqlite3_column_name(stmt, i);
            if (columnName) {
                result.addColumn(std::string(columnName));
            }
            
            int columnType = sqlite3_column_type(stmt, i);
            switch (columnType) {
                case SQLITE_INTEGER:
                    result.addValue(static_cast<int64_t>(sqlite3_column_int64(stmt, i)));
                    break;
                case SQLITE_FLOAT:
                    result.addValue(static_cast<double>(sqlite3_column_double(stmt, i)));
                    break;
                case SQLITE_TEXT: {
                    const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                    result.addValue(text ? std::string(text) : std::string(""));
                    break;
                }
                case SQLITE_BLOB:
                    // For now, convert blob to string representation
                    result.addValue(std::string("[BLOB]"));
                    break;
                case SQLITE_NULL:
                default:
                    result.addNull();
                    break;
            }
        }
        results.push_back(result);
    }
    
    sqlite3_finalize(stmt);
    return results;
}

SqliteResult SQLiteQueryBuilder::FirstCustom() {
    Limit(1);
    auto results = ExecCustom();
    if (results.empty()) {
        return SqliteResult(); // Return empty result
    }
    return results[0];
}

// Generic query builder implementations
std::vector<SqliteResult> GenericSQLiteQueryBuilder::Exec() {
    if (!db->isConnected()) {
        throw std::runtime_error("GenericSQLiteQueryBuilder::Exec() - Database not connected");
    }
    
    std::vector<SqliteResult> results;
    std::string sql = BuildQuery();
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db->getDB(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("GenericSQLiteQueryBuilder::Exec() - Failed to prepare statement: " + std::string(sqlite3_errmsg(db->getDB())) + "\nSQL: " + sql);
    }
    
    BindParameters(stmt);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SqliteResult result;
        int columnCount = sqlite3_column_count(stmt);
        
        for (int i = 0; i < columnCount; ++i) {
            const char* columnName = sqlite3_column_name(stmt, i);
            if (columnName) {
                result.addColumn(std::string(columnName));
            }
            
            int columnType = sqlite3_column_type(stmt, i);
            switch (columnType) {
                case SQLITE_INTEGER:
                    result.addValue(static_cast<int64_t>(sqlite3_column_int64(stmt, i)));
                    break;
                case SQLITE_FLOAT:
                    result.addValue(static_cast<double>(sqlite3_column_double(stmt, i)));
                    break;
                case SQLITE_TEXT: {
                    const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                    result.addValue(text ? std::string(text) : std::string(""));
                    break;
                }
                case SQLITE_BLOB:
                    result.addValue(std::string("[BLOB]"));
                    break;
                case SQLITE_NULL:
                default:
                    result.addNull();
                    break;
            }
        }
        results.push_back(result);
    }
    
    sqlite3_finalize(stmt);
    return results;
}

SqliteResult GenericSQLiteQueryBuilder::First() {
    Limit(1);
    auto results = Exec();
    if (results.empty()) {
        return SqliteResult();
    }
    return results[0];
}

bool GenericSQLiteQueryBuilder::Exists() {
    std::string sql = BuildQuery("1");
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db->getDB(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("GenericSQLiteQueryBuilder::Exists() - Failed to prepare statement: " + std::string(sqlite3_errmsg(db->getDB())) + "\nSQL: " + sql);
    }
    
    BindParameters(stmt);
    
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    
    return exists;
}

int GenericSQLiteQueryBuilder::Count() {
    std::string sql = BuildQuery("COUNT(*)");
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db->getDB(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("GenericSQLiteQueryBuilder::Count() - Failed to prepare statement: " + std::string(sqlite3_errmsg(db->getDB())) + "\nSQL: " + sql);
    }
    
    BindParameters(stmt);
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    return count;
}