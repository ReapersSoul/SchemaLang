#pragma once
#include {{format_include("SQLiteDB.hpp")}}
{% for include in includes %}
#include {{include}}
{% endfor %}


SQLiteDB::~SQLiteDB() {}

SQLiteDB::SQLiteDB(std::filesystem::path db_path){
    this->db_path = db_path;
    this->db = nullptr;
};

void SQLiteDB::connect(){
    if (isConnected()) {
        return; // Already connected
    }
    // Ensure parent directory exists
    std::error_code ec;
    const auto parent = db_path.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            throw std::runtime_error(
                "SQLiteDB::connect() - Failed to create database directory '" + parent.string() +
                "' for database '" + db_path.string() + "': " + ec.message() + " (error code: " + std::to_string(ec.value()) + ")"
            );
        }
    }

    // Open DB with explicit flags; allow creation if it doesn't exist.
    // FULLMUTEX for thread-safety if you use SQLite from multiple threads.
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    sqlite3* handle = nullptr;
    int rc = sqlite3_open_v2(db_path.string().c_str(), &handle, flags, nullptr);

    if (rc != SQLITE_OK) {
        // Prefer sqlite3_errmsg if we have a handle, otherwise sqlite3_errstr(rc)
        std::string msg = handle ? sqlite3_errmsg(handle) : sqlite3_errstr(rc);
        if (handle) {
            sqlite3_close(handle);
        }
        handle = nullptr;
        throw std::runtime_error(
            "SQLiteDB::connect() - Failed to open database '" + db_path.string() + "': " + msg + " (SQLite error code: " + std::to_string(rc) + ", flags: " + std::to_string(flags) + ")"
        );
    }

    db = handle;

    // Optional: log the path we opened (swap to your logging system if you have one)
    // fprintf(stderr, "Opened SQLite DB at: %s\n", db_path.string().c_str());

    // Foreign keys and update hook
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    sqlite3_update_hook(db, SQLiteDB::updateCallback, this);

{% for struct in structs %}
        create{{struct.identifierCamel}}Table();
{% endfor %}
};

void SQLiteDB::disconnect(){
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
};

bool SQLiteDB::isConnected() const{
    return db != nullptr;
};

sqlite3* SQLiteDB::getDB(){
    if (!db) {
        throw std::runtime_error("SQLiteDB::getDB() - Database is not connected. Call connect() first.");
    }
    return db;
};

{% for struct in structs %}

void SQLiteDB::create{{struct.identifierCamel}}Table() {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::create{{struct.identifierCamel}}Table() - Database not connected. Call connect() first.");
    }
    const char* create_table_sql = R"(
{% set additional_field_count = 0 %}
{% for inner_struct in structs %}
{% for mv in inner_struct.member_variables %}
{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}
{% set additional_field_count = additional_field_count + 1 %}
{% endif %}
{% endfor %}
{% endfor %}
CREATE TABLE IF NOT EXISTS {{struct.identifier}} (
{% set field_count = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}
{% if not field.type.is_array %}{% set current_field = current_field + 1 %}
    {{ field.identifier }} {{ SQLite_convert_to_local_type(field.type) }} {% if field.type.required %} NOT NULL {% endif %}{% if field.unique %} UNIQUE {% endif %}{% if field.primary_key %} PRIMARY KEY {% endif %}{% if field.auto_increment %} AUTOINCREMENT {% endif %}{% if field.reference.struct_name!="" %} REFERENCES {{field.reference.struct_name}}({{field.reference.variable_name}}) {% endif %}{% if field.default_value != "" %} DEFAULT {{SQLite_format_default(field.type, field.default_value)}}{% endif %}{% if current_field < field_count or additional_field_count > 0 %},{% endif %}
        
{% endif %}
{% endfor %}{% set current_field = 0 %}
{% for inner_struct in structs %}
{% for mv in inner_struct.member_variables %}
{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set current_field = current_field + 1 %}
    {{inner_struct.identifier}}_id INTEGER REFERENCES {{inner_struct.identifier}}(id) ON DELETE CASCADE{% if current_field < additional_field_count %},{% endif %}
{% endif %}
{% endfor %}
{% endfor %}
);
        )";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, create_table_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string error = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("SQLiteDB::create{{struct.identifierCamel}}Table() - SQL execution failed: " + error + "\nSQL: " + std::string(create_table_sql));
    }
    
    // Create child tables for primitive arrays
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
    {
        const char* child_table_sql = R"(
CREATE TABLE IF NOT EXISTS {{struct.identifier}}_{{mv.identifier}} (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    {{struct.identifier}}_id INTEGER NOT NULL REFERENCES {{struct.identifier}}(id) ON DELETE CASCADE,
    sequence INTEGER NOT NULL,
    value {{ SQLite_convert_to_local_type(mv.type.elem_type) }} NOT NULL{% if mv.unique %},
    UNIQUE({{struct.identifier}}_id, value){% endif %},
    UNIQUE({{struct.identifier}}_id, sequence)
);
CREATE INDEX IF NOT EXISTS idx_{{struct.identifier}}_{{mv.identifier}}_parent_id ON {{struct.identifier}}_{{mv.identifier}}({{struct.identifier}}_id);
        )";
        if (sqlite3_exec(db, child_table_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::string error = errMsg ? errMsg : "Unknown error";
            sqlite3_free(errMsg);
            throw std::runtime_error("SQLiteDB::create{{struct.identifierCamel}}Table() - Failed to create child table {{struct.identifier}}_{{mv.identifier}}: " + error);
        }
    }
{% endif %}
{% endfor %}
}

std::vector<std::shared_ptr<{{struct.identifier}}Schema>> SQLiteDB::selectAll{{struct.identifierCamel}}() {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::selectAll{{struct.identifierCamel}}() - Database not connected. Call connect() first.");
    }
    
    std::vector<std::shared_ptr<{{struct.identifier}}Schema>> results;
    const char* sql = R"(SELECT 
    {% set field_count = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set additional_field_count = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set additional_field_count = additional_field_count + 1 %}{% endif %}{% endfor %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set current_field = current_field + 1 %}{{field.identifier}}{% if current_field < field_count or additional_field_count > 0 %}, {% endif %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set current_field = current_field + 1 %}{{inner_struct.identifier}}_id{% if current_field < additional_field_count %}, {% endif %}{% endif %}{% endfor %}{% endfor %}
    
    FROM {{struct.identifier}};)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        auto obj = std::make_shared<{{struct.identifier}}Schema>();
        int col = 0;
{% for field in struct.member_variables %}
{% if not field.type.is_array %}
        // Set {{field.identifier}}
{% if field.type.is_string %}
        const char* {{field.identifier}}_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col++));
        if ({{field.identifier}}_text) {
            obj->set{{field.identifierCamel}}(std::string({{field.identifier}}_text));
        }
{% else if field.type.is_integer %}
        obj->set{{field.identifierCamel}}(sqlite3_column_int(stmt, col++));
{% else if field.type.is_real %}
        obj->set{{field.identifierCamel}}(static_cast<float>(sqlite3_column_double(stmt, col++)));
{% else if field.type.is_bool %}
        obj->set{{field.identifierCamel}}(sqlite3_column_int(stmt, col++) != 0);
{% else if field.type.is_enum %}
        obj->set{{field.identifierCamel}}(static_cast<{{field.type.identifier}}Schema>(sqlite3_column_int(stmt, col++)));
{% else if field.type.is_struct %}
        // If this is a struct, we need to handle it differently
        int64_t {{field.identifier}}_id_value = sqlite3_column_int64(stmt, col++);
        if ({{field.identifier}}_id_value > 0) {
            obj->set{{field.identifierCamel}}(select{{field.type.identifier}}ById({{field.identifier}}_id_value));
        }
{% else %}
        // Need to handle type {{field.type.identifier}} here in generated code
        // For now, just skip it
        col++;
{% endif %}
{% endif %}
{% endfor %}

        // Set parent ID fields
{% for inner_struct in structs %}
{% for mv in inner_struct.member_variables %}
{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}
        // Set {{inner_struct.identifier}}_id
        int64_t {{inner_struct.identifier}}_id_value = sqlite3_column_int64(stmt, col++);
        if ({{inner_struct.identifier}}_id_value != 0) {
            obj->set{{inner_struct.identifier}}Id({{inner_struct.identifier}}_id_value);
        }
{% endif %}
{% endfor %}
{% endfor %}

        //get nested arrays if any
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.elem_type.is_struct %}
        auto nested_{{mv.type.elem_type.identifier}}_items = select{{mv.type.elem_type.identifier}}By{{struct.identifier}}_id(obj->getId());
        for (const auto& item : nested_{{mv.type.elem_type.identifier}}_items) {
            obj->addTo{{mv.identifierCamel}}WithParent(item);
        }
{% endif %}
{% endfor %}

        //get primitive arrays if any
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
        {
            const char* array_sql = "SELECT value FROM {{struct.identifier}}_{{mv.identifier}} WHERE {{struct.identifier}}_id = ? ORDER BY sequence";
            sqlite3_stmt* array_stmt;
            if (sqlite3_prepare_v2(db, array_sql, -1, &array_stmt, nullptr) == SQLITE_OK) {
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

std::shared_ptr<{{struct.identifier}}Schema> SQLiteDB::select{{struct.identifierCamel}}ById(int64_t id){
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::select{{struct.identifierCamel}}ById(" + std::to_string(id) + ") - Database not connected. Call connect() first.");
    }

    const char* sql = R"(
    SELECT
   {% set field_count = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set current_field = current_field + 1 %}{{field.identifier}}{% if current_field < field_count %}, {% endif %}{% endif %}{% endfor %}
    
    FROM {{struct.identifier}} WHERE id = ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    
    sqlite3_bind_int64(stmt, 1, id);
    
    {% for field in struct.member_variables %}
    {% if field.type.is_struct %}
    int64_t {{field.identifier}}_id_value = 0;
    {% endif %}
    {% endfor %}

    std::shared_ptr<{{struct.identifier}}Schema> result = nullptr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = std::make_shared<{{struct.identifier}}Schema>();
        int col = 0;
{% for field in struct.member_variables %}
        // Set {{field.identifier}}
{% if field.type.is_string %}
        const char* {{field.identifier}}_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col++));
        if ({{field.identifier}}_text) {
            result->set{{field.identifierCamel}}(std::string({{field.identifier}}_text));
        }
{% else if field.type.is_integer %}
        result->set{{field.identifierCamel}}(sqlite3_column_int(stmt, col++));
{% else if field.type.is_real %}
        result->set{{field.identifierCamel}}(sqlite3_column_double(stmt, col++));
{% else if field.type.is_bool %}
        result->set{{field.identifierCamel}}(sqlite3_column_int(stmt, col++) != 0);
{% else if field.type.is_enum %}
        result->set{{field.identifierCamel}}(static_cast<{{field.type.identifier}}Schema>(sqlite3_column_int(stmt, col++)));
{% else if field.type.is_struct %}
        {{field.identifier}}_id_value = sqlite3_column_int64(stmt, col++);
        if ({{field.identifier}}_id_value > 0) {
            result->set{{field.identifierCamel}}(select{{field.type.identifier}}ById({{field.identifier}}_id_value));
        }
{% else %}
        // Need to handle type {{field.type.identifier}} here in generated code
        // For now, just skip it
        col++;
{% endif %}
{% endfor %}
    }

    // If we have struct references, fetch and set them
{% for field in struct.member_variables %}
{% if field.type.is_struct %}
    if ({{field.identifier}}_id_value > 0) {
        result->set{{field.identifierCamel}}(select{{field.type.identifier}}ById({{field.identifier}}_id_value));
    }
{% endif %}
{% endfor %}

    // Load primitive arrays if any
    if (result) {
{% for mv in struct.member_variables %}\
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
        {
            const char* array_sql = "SELECT value FROM {{struct.identifier}}_{{mv.identifier}} WHERE {{struct.identifier}}_id = ? ORDER BY sequence";
            sqlite3_stmt* array_stmt;
            if (sqlite3_prepare_v2(db, array_sql, -1, &array_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int64(array_stmt, 1, result->getId());
                while (sqlite3_step(array_stmt) == SQLITE_ROW) {
{% if mv.type.elem_type.is_string %}
                    const char* value_text = reinterpret_cast<const char*>(sqlite3_column_text(array_stmt, 0));
                    if (value_text) {
                        result->addTo{{mv.identifierCamel}}(std::string(value_text));
                    }
{% else if mv.type.elem_type.identifier == "int64_t" or mv.type.elem_type.identifier == "long" %}
                    result->addTo{{mv.identifierCamel}}(sqlite3_column_int64(array_stmt, 0));
{% else if mv.type.elem_type.is_integer %}
                    result->addTo{{mv.identifierCamel}}(sqlite3_column_int(array_stmt, 0));
{% else if mv.type.elem_type.is_real %}
                    result->addTo{{mv.identifierCamel}}(static_cast<{{mv.type.elem_type.estimated}}>(sqlite3_column_double(array_stmt, 0)));
{% else if mv.type.elem_type.is_bool %}
                    result->addTo{{mv.identifierCamel}}(sqlite3_column_int(array_stmt, 0) != 0);
{% else if mv.type.elem_type.is_char %}
                    const char* char_text = reinterpret_cast<const char*>(sqlite3_column_text(array_stmt, 0));
                    if (char_text && char_text[0]) {
                        result->addTo{{mv.identifierCamel}}(char_text[0]);
                    }
{% endif %}
                }
                sqlite3_finalize(array_stmt);
            }
        }
{% endif %}
{% endfor %}
    }

    sqlite3_finalize(stmt);
    return result;
}

{% for mv in struct.member_variables %}
{% if not (mv.type.is_array or mv.type.is_struct or mv.type.is_enum) %}
{% if mv.identifier != "id" %}
std::vector<std::shared_ptr<{{struct.identifier}}Schema>> SQLiteDB::select{{struct.identifierCamel}}By{{mv.identifierCamel}}({{mv.type.estimated}} value) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::select{{struct.identifierCamel}}By{{mv.identifierCamel}}() - Database not connected. Call connect() first.");
    }
    
    std::vector<std::shared_ptr<{{struct.identifier}}Schema>> results;
    const char* sql = R"(    SELECT
        {% set field_count = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set current_field = current_field + 1 %}{{field.identifier}}{% if current_field < field_count %}, {% endif %}{% endif %}{% endfor %}

FROM {{struct.identifier}} WHERE {{mv.identifier}} = ?;)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    
    // Bind the parameter
    {% if mv.type.is_string %}
    sqlite3_bind_text(stmt, 1, value.c_str(), -1, SQLITE_STATIC);
    {% else if mv.type.is_integer %}
    sqlite3_bind_int(stmt, 1, value);
    {% else if mv.type.is_real %}
    sqlite3_bind_double(stmt, 1, value);
    {% else if mv.type.is_bool %}
    sqlite3_bind_int(stmt, 1, value ? 1 : 0);
    {% else %}
    // Need to handle type {{mv.type.identifier}} here in generated code
    // For now, just bind a default value
    sqlite3_bind_text(stmt, 1, "", -1, SQLITE_STATIC);
    {% endif %}
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        auto obj = std::make_shared<{{struct.identifier}}Schema>();
        int col = 0;
{% for field in struct.member_variables %}
{% if not field.type.is_array %}
        // Set {{field.identifier}}
{% if field.type.is_string %}
        const char* {{field.identifier}}_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col++));
        if ({{field.identifier}}_text) {
            obj->set{{field.identifierCamel}}(std::string({{field.identifier}}_text));
        }
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
            obj->set{{field.identifierCamel}}(select{{field.type.identifier}}ById({{field.identifier}}_id_value));
        }
{% else %}
        // Need to handle type {{field.type.identifier}} here in generated code
        // For now, just skip it
        col++;
{% endif %}
{% endif %}
{% endfor %}

        //get primitive arrays if any
{% for arr_mv in struct.member_variables %}
{% if arr_mv.type.is_array and arr_mv.type.is_array_of_base_type and not arr_mv.type.is_array_of_enum %}
        {
            const char* array_sql = "SELECT value FROM {{struct.identifier}}_{{arr_mv.identifier}} WHERE {{struct.identifier}}_id = ? ORDER BY sequence";
            sqlite3_stmt* array_stmt;
            if (sqlite3_prepare_v2(db, array_sql, -1, &array_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int64(array_stmt, 1, obj->getId());
                while (sqlite3_step(array_stmt) == SQLITE_ROW) {
{% if arr_mv.type.elem_type.is_string %}
                    const char* value_text = reinterpret_cast<const char*>(sqlite3_column_text(array_stmt, 0));
                    if (value_text) {
                        obj->addTo{{arr_mv.identifierCamel}}(std::string(value_text));
                    }
{% else if arr_mv.type.elem_type.identifier == "int64_t" or arr_mv.type.elem_type.identifier == "long" %}
                    obj->addTo{{arr_mv.identifierCamel}}(sqlite3_column_int64(array_stmt, 0));
{% else if arr_mv.type.elem_type.is_integer %}
                    obj->addTo{{arr_mv.identifierCamel}}(sqlite3_column_int(array_stmt, 0));
{% else if arr_mv.type.elem_type.is_real %}
                    obj->addTo{{arr_mv.identifierCamel}}(static_cast<{{arr_mv.type.elem_type.estimated}}>(sqlite3_column_double(array_stmt, 0)));
{% else if arr_mv.type.elem_type.is_bool %}
                    obj->addTo{{arr_mv.identifierCamel}}(sqlite3_column_int(array_stmt, 0) != 0);
{% else if arr_mv.type.elem_type.is_char %}
                    const char* char_text = reinterpret_cast<const char*>(sqlite3_column_text(array_stmt, 0));
                    if (char_text && char_text[0]) {
                        obj->addTo{{arr_mv.identifierCamel}}(char_text[0]);
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
{% endif %}
{% endif %}
{% endfor %}

{% for mv in struct.member_variables %}
{% if mv.type.is_array %}
{# This struct has an array, but we need to find structs that have arrays of THIS struct type #}
{% endif %}
{% endfor %}

{# Generate selectBy methods for parent relationships #}
{% for other_struct in structs %}
{% for other_mv in other_struct.member_variables %}
{% if other_mv.type.is_array and other_mv.type.elem_type.is_struct and other_mv.type.elem_type.identifier == struct.identifier %}
std::vector<std::shared_ptr<{{struct.identifier}}Schema>> SQLiteDB::select{{struct.identifierCamel}}By{{other_struct.identifier}}_id(int64_t value)
{    
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::select{{struct.identifierCamel}}By{{other_struct.identifier}}_id(" + std::to_string(value) + ") - Database not connected. Call connect() first.");
    }

    std::vector<std::shared_ptr<{{struct.identifier}}Schema>> results;
    const char * sql = R"(
    SELECT
        {% set field_count = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set current_field = current_field + 1 %}{{field.identifier}}{% if current_field < field_count %}, {% endif %}{% endif %}{% endfor %}
        
        FROM {{struct.identifier}} WHERE {{other_struct.identifier}}_id = ?;
    )";
    sqlite3_stmt * stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_int64(stmt, 1, value);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        auto obj = std::make_shared<{{struct.identifier}}Schema>();
        int col = 0;
{% for field in struct.member_variables %}
{% if not field.type.is_array %}
        // Set {{field.identifier}}
{% if field.type.is_string %}
        const char* {{field.identifier}}_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col++));
        if ({{field.identifier}}_text) {
            obj->set{{field.identifierCamel}}(std::string({{field.identifier}}_text));
        }
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
            obj->set{{field.identifierCamel}}(select{{field.type.identifier}}ById({{field.identifier}}_id_value));
        }
{% else %}
        // Need to handle type {{field.type.identifier}} here in generated code
        // For now, just skip it
        col++;
{% endif %}
{% endif %}
{% endfor %}

        //get primitive arrays if any
{% for arr_mv in struct.member_variables %}
{% if arr_mv.type.is_array and arr_mv.type.is_array_of_base_type and not arr_mv.type.is_array_of_enum %}
        {
            const char* array_sql = "SELECT value FROM {{struct.identifier}}_{{arr_mv.identifier}} WHERE {{struct.identifier}}_id = ? ORDER BY sequence";
            sqlite3_stmt* array_stmt;
            if (sqlite3_prepare_v2(db, array_sql, -1, &array_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int64(array_stmt, 1, obj->getId());
                while (sqlite3_step(array_stmt) == SQLITE_ROW) {
{% if arr_mv.type.elem_type.is_string %}
                    const char* value_text = reinterpret_cast<const char*>(sqlite3_column_text(array_stmt, 0));
                    if (value_text) {
                        obj->addTo{{arr_mv.identifierCamel}}(std::string(value_text));
                    }
{% else if arr_mv.type.elem_type.identifier == "int64_t" or arr_mv.type.elem_type.identifier == "long" %}
                    obj->addTo{{arr_mv.identifierCamel}}(sqlite3_column_int64(array_stmt, 0));
{% else if arr_mv.type.elem_type.is_integer %}
                    obj->addTo{{arr_mv.identifierCamel}}(sqlite3_column_int(array_stmt, 0));
{% else if arr_mv.type.elem_type.is_real %}
                    obj->addTo{{arr_mv.identifierCamel}}(static_cast<{{arr_mv.type.elem_type.estimated}}>(sqlite3_column_double(array_stmt, 0)));
{% else if arr_mv.type.elem_type.is_bool %}
                    obj->addTo{{arr_mv.identifierCamel}}(sqlite3_column_int(array_stmt, 0) != 0);
{% else if arr_mv.type.elem_type.is_char %}
                    const char* char_text = reinterpret_cast<const char*>(sqlite3_column_text(array_stmt, 0));
                    if (char_text && char_text[0]) {
                        obj->addTo{{arr_mv.identifierCamel}}(char_text[0]);
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
{% endif %}
{% endfor %}
{% endfor %}

// Fluent query builder - now creates builder and calls From() automatically
SQLiteQueryBuilder SQLiteDB::Select{{struct.identifierCamel}}() {
    return SQLiteQueryBuilder(this).From("{{struct.identifier}}");
}

SQLiteQueryBuilder SQLiteDB::Query{{struct.identifierCamel}}(){
    return SQLiteQueryBuilder(this);
}

//updateInsert
int64_t SQLiteDB::insertOrUpdate{{struct.identifierCamel}}(std::shared_ptr<{{struct.identifier}}Schema> obj, bool force_id) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::insertOrUpdate{{struct.identifierCamel}}() - Database not connected. Call connect() first.");
    }
    
    if (!obj) {
        throw std::runtime_error("SQLiteDB::insertOrUpdate{{struct.identifierCamel}}() - Object parameter is null. Cannot insert/update null {{struct.identifier}}Schema object.");
    }
    
    // Use INSERT OR REPLACE to handle both insert and update in one statement
    const char* sql_filter_id = "INSERT OR REPLACE INTO {{struct.identifier}} ({% set field_count = 0 %}{% for field in struct.member_variables %}{% if field.identifier != "id" and not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set additional_field_count = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set additional_field_count = additional_field_count + 1 %}{% endif %}{% endfor %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}{% if field.identifier != "id" and not field.type.is_array %}{% set current_field = current_field + 1 %}{{field.identifier}}{% if current_field < field_count or additional_field_count > 0 %}, {% endif %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set current_field = current_field + 1 %}{{inner_struct.identifier}}_id{% if current_field < additional_field_count %}, {% endif %}{% endif %}{% endfor %}{% endfor %}) VALUES ({% set current_param = 0 %}{% for field in struct.member_variables %}{% if field.identifier != "id" and not field.type.is_array %}{% set current_param = current_param + 1 %}?{% if current_param < field_count or additional_field_count > 0 %}, {% endif %}{% endif %}{% endfor %}{% set current_param = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set current_param = current_param + 1 %}?{% if current_param < additional_field_count %}, {% endif %}{% endif %}{% endfor %}{% endfor %})";
    const char* sql = "INSERT OR REPLACE INTO {{struct.identifier}} ({% set field_count = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set additional_field_count = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set additional_field_count = additional_field_count + 1 %}{% endif %}{% endfor %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set current_field = current_field + 1 %}{{field.identifier}}{% if current_field < field_count or additional_field_count > 0 %}, {% endif %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set current_field = current_field + 1 %}{{inner_struct.identifier}}_id{% if current_field < additional_field_count %}, {% endif %}{% endif %}{% endfor %}{% endfor %}) VALUES ({% set current_param = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set current_param = current_param + 1 %}?{% if current_param < field_count or additional_field_count > 0 %}, {% endif %}{% endif %}{% endfor %}{% set current_param = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set current_param = current_param + 1 %}?{% if current_param < additional_field_count %}, {% endif %}{% endif %}{% endfor %}{% endfor %})";
    
    if (obj->getId() <= 0&& !force_id) {
        sql = sql_filter_id;
    }

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    
    int param = 1;
{% for field in struct.member_variables %}
{% if not field.type.is_array %}
{% if field.identifier == "id" %}
    // Only bind id if it's greater than 0 (update case)
    if (obj->getId() > 0 || force_id) {
{% endif %}
    // Bind {{field.identifier}}
{% if field.type.required %}
{% if field.type.is_string %}
    sqlite3_bind_text(stmt, param++, obj->get{{field.identifierCamel}}().c_str(), -1, SQLITE_STATIC);
{% else if field.type.identifier == "int64_t" or field.type.identifier == "long" %}
    sqlite3_bind_int64(stmt, param++, obj->get{{field.identifierCamel}}());
{% else if field.type.is_integer %}
    sqlite3_bind_int(stmt, param++, obj->get{{field.identifierCamel}}());
{% else if field.type.is_real %}
    sqlite3_bind_double(stmt, param++, static_cast<double>(obj->get{{field.identifierCamel}}()));
{% else if field.type.is_bool %}
    sqlite3_bind_int(stmt, param++, obj->get{{field.identifierCamel}}() ? 1 : 0);
{% else if field.type.is_enum %}
    sqlite3_bind_int(stmt, param++, static_cast<int>(obj->get{{field.identifierCamel}}()));
{% else if field.type.is_struct %}
    if(obj->get{{field.identifierCamel}}()) {
        sqlite3_bind_int64(stmt, param++, obj->get{{field.identifierCamel}}()->getId());
    } else {
        param++;
    }
{% else %}
    // Need to handle type {{field.type.identifier}} here in generated code
    // For now, just bind null
    sqlite3_bind_null(stmt, param++);
{% endif %}
{% else %}
    // Optional field - check if has value and bind accordingly
{% if field.type.is_string %}
    if (obj->get{{field.identifierCamel}}().has_value()) {
        sqlite3_bind_text(stmt, param++, obj->get{{field.identifierCamel}}().value().c_str(), -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, param++);
    }
{% else if field.type.identifier == "int64_t" or field.type.identifier == "long" %}
    if (obj->get{{field.identifierCamel}}().has_value()) {
        sqlite3_bind_int64(stmt, param++, obj->get{{field.identifierCamel}}().value());
    } else {
        sqlite3_bind_null(stmt, param++);
    }
{% else if field.type.is_integer %}
    if (obj->get{{field.identifierCamel}}().has_value()) {
        sqlite3_bind_int(stmt, param++, obj->get{{field.identifierCamel}}().value());
    } else {
        sqlite3_bind_null(stmt, param++);
    }
{% else if field.type.is_real %}
    if (obj->get{{field.identifierCamel}}().has_value()) {
        sqlite3_bind_double(stmt, param++, static_cast<double>(obj->get{{field.identifierCamel}}().value()));
    } else {
        sqlite3_bind_null(stmt, param++);
    }
{% else if field.type.is_bool %}
    if (obj->get{{field.identifierCamel}}().has_value()) {
        sqlite3_bind_int(stmt, param++, obj->get{{field.identifierCamel}}().value() ? 1 : 0);
    } else {
        sqlite3_bind_null(stmt, param++);
    }
{% else if field.type.is_enum %}
    if (obj->get{{field.identifierCamel}}().has_value()) {
        sqlite3_bind_int(stmt, param++, static_cast<int>(obj->get{{field.identifierCamel}}().value()));
    } else {
        sqlite3_bind_null(stmt, param++);
    }
{% else if field.type.is_struct %}
    if (obj->get{{field.identifierCamel}}().has_value() && obj->get{{field.identifierCamel}}().value()) {
        sqlite3_bind_int64(stmt, param++, obj->get{{field.identifierCamel}}().value()->getId());
    } else {
        sqlite3_bind_null(stmt, param++);
    }
{% else %}
    // Need to handle type {{field.type.identifier}} here in generated code
    // For now, just bind null
    sqlite3_bind_null(stmt, param++);
{% endif %}
{% endif %}
{% if field.identifier == "id" %}
    }
{% endif %}
{% endif %}
{% endfor %}
    
    // Bind parent ID fields for array relationships
{% for inner_struct in structs %}
{% for mv in inner_struct.member_variables %}
{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}
    // Bind {{inner_struct.identifier}}_id
    if (obj->get{{inner_struct.identifier}}Id().has_value()) {
        sqlite3_bind_int64(stmt, param++, obj->get{{inner_struct.identifier}}Id().value());
    } else {
        sqlite3_bind_null(stmt, param++);
    }
{% endif %}
{% endfor %}
{% endfor %}
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error("SQLiteDB::insertOrUpdate{{struct.identifierCamel}}() - Failed to execute INSERT OR REPLACE statement for {{struct.identifier}} (ID: " + std::to_string(obj->getId()) + "): " + std::string(sqlite3_errmsg(db)) + "\nSQL: " + std::string(sql));
    }
    
    int64_t resultId = obj->getId();
    if (resultId <= 0) {
        // This was an insert, get the new ID
        resultId = sqlite3_last_insert_rowid(db);
        obj->setId(resultId);
    }
    
    sqlite3_finalize(stmt);
    
    // Handle primitive array fields
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
    // Delete existing {{mv.identifier}} array entries
    {
        const char* delete_sql = "DELETE FROM {{struct.identifier}}_{{mv.identifier}} WHERE {{struct.identifier}}_id = ?";
        sqlite3_stmt* delete_stmt;
        if (sqlite3_prepare_v2(db, delete_sql, -1, &delete_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(delete_stmt, 1, resultId);
            sqlite3_step(delete_stmt);
            sqlite3_finalize(delete_stmt);
        }
    }
    
    // Insert new {{mv.identifier}} array entries
{% if mv.type.required %}
    if (!obj->get{{mv.identifierCamel}}().empty()) {
{% else %}
    if (obj->get{{mv.identifierCamel}}().has_value() && !obj->get{{mv.identifierCamel}}().value().empty()) {
{% endif %}
        const char* insert_sql = "INSERT INTO {{struct.identifier}}_{{mv.identifier}} ({{struct.identifier}}_id, sequence, value) VALUES (?, ?, ?)";
        sqlite3_stmt* insert_stmt;
        if (sqlite3_prepare_v2(db, insert_sql, -1, &insert_stmt, nullptr) == SQLITE_OK) {
            int sequence = 0;
{% if mv.type.required %}
            for (const auto& item : obj->get{{mv.identifierCamel}}()) {
{% else %}
            for (const auto& item : obj->get{{mv.identifierCamel}}().value()) {
{% endif %}
                sqlite3_reset(insert_stmt);
                sqlite3_bind_int64(insert_stmt, 1, resultId);
                sqlite3_bind_int(insert_stmt, 2, sequence++);
{% if mv.type.elem_type.is_string %}
                sqlite3_bind_text(insert_stmt, 3, item.c_str(), -1, SQLITE_TRANSIENT);
{% else if mv.type.elem_type.identifier == "int64_t" or mv.type.elem_type.identifier == "long" %}
                sqlite3_bind_int64(insert_stmt, 3, item);
{% else if mv.type.elem_type.is_integer %}
                sqlite3_bind_int(insert_stmt, 3, item);
{% else if mv.type.elem_type.is_real %}
                sqlite3_bind_double(insert_stmt, 3, static_cast<double>(item));
{% else if mv.type.elem_type.is_bool %}
                sqlite3_bind_int(insert_stmt, 3, item ? 1 : 0);
{% else if mv.type.elem_type.is_char %}
                sqlite3_bind_text(insert_stmt, 3, std::string(1, item).c_str(), -1, SQLITE_TRANSIENT);
{% endif %}
                if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
                    sqlite3_finalize(insert_stmt);
                    throw std::runtime_error("Failed to insert {{mv.identifier}} array item: " + std::string(sqlite3_errmsg(db)));
                }
            }
            sqlite3_finalize(insert_stmt);
        }
    }
{% endif %}
{% endfor %}
    
    return resultId;
}

bool SQLiteDB::delete{{struct.identifierCamel}}ById(int64_t id) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::delete{{struct.identifierCamel}}ById(" + std::to_string(id) + ") - Database not connected. Call connect() first.");
    }
    
    const char* sql = "DELETE FROM {{struct.identifier}} WHERE id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("SQLiteDB::delete{{struct.identifierCamel}}ById(" + std::to_string(id) + ") - Failed to prepare DELETE statement: " + std::string(sqlite3_errmsg(db)) + "\nSQL: " + std::string(sql));
    }
    
    sqlite3_bind_int64(stmt, 1, id);
    
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    
    return success;
}

bool SQLiteDB::delete{{struct.identifierCamel}}ByIdCascade(int64_t id) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::delete{{struct.identifierCamel}}ByIdCascade(" + std::to_string(id) + ") - Database not connected. Call connect() first.");
    }
    
    // First, delete nested array items
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.elem_type.is_struct %}
    auto nested_{{mv.identifier}}_items = select{{mv.type.elem_type.identifier}}By{{struct.identifier}}_id(id);
    for (const auto& item : nested_{{mv.identifier}}_items) {
        delete{{mv.type.elem_type.identifierCamel}}ByIdCascade(item->getId());
    }
{% endif %}
{% endfor %}

    // Now delete the main object
    return delete{{struct.identifierCamel}}ById(id);
}

bool SQLiteDB::has{{struct.identifierCamel}}ById(int64_t id){
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::has{{struct.identifierCamel}}ById(" + std::to_string(id) + ") - Database not connected. Call connect() first.");
    }
    
    const char* sql = R"(SELECT 1 FROM {{struct.identifier}} WHERE id = ? LIMIT 1;)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    
    sqlite3_bind_int64(stmt, 1, id);
    
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    
    sqlite3_finalize(stmt);
    return exists;
}

{% for mv in struct.member_variables %}
{% if not (mv.type.is_array or mv.type.is_struct or mv.type.is_enum) %}
{% if mv.identifier != "id" %}
bool SQLiteDB::has{{struct.identifierCamel}}By{{mv.identifierCamel}}({{mv.type.estimated}} value){
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::has{{struct.identifierCamel}}By{{mv.identifierCamel}}() - Database not connected. Call connect() first.");
    }
    
    const char* sql = R"(SELECT 1 FROM {{struct.identifier}} WHERE {{mv.identifier}} = ? LIMIT 1;)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    
    // Bind the parameter
    {% if mv.type.is_string %}
    sqlite3_bind_text(stmt, 1, value.c_str(), -1, SQLITE_STATIC);
    {% else if mv.type.is_integer %}
    sqlite3_bind_int(stmt, 1, value);
    {% else if mv.type.is_real %}
    sqlite3_bind_double(stmt, 1, value);
    {% else if mv.type.is_bool %}
    sqlite3_bind_int(stmt, 1, value ? 1 : 0);
    {% else %}
    // Need to handle type {{mv.type.identifier}} here in generated code
    // For now, just bind null
    sqlite3_bind_text(stmt, 1, "", -1, SQLITE_STATIC);
    {% endif %}
    
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    
    sqlite3_finalize(stmt);
    return exists;
}
{% endif %}
{% endif %}
{% endfor %}

// Primitive array helper method implementations
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
std::vector<{{mv.type.elem_type.estimated}}> SQLiteDB::select{{struct.identifierCamel}}{{mv.identifierCamel}}(int64_t parent_id) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::select{{struct.identifierCamel}}{{mv.identifierCamel}}() - Database not connected. Call connect() first.");
    }
    
    std::vector<{{mv.type.elem_type.estimated}}> result;
    const char* sql = "SELECT value FROM {{struct.identifier}}_{{mv.identifier}} WHERE {{struct.identifier}}_id = ? ORDER BY sequence";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    
    sqlite3_bind_int64(stmt, 1, parent_id);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
{% if mv.type.elem_type.is_string %}
        const char* value_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (value_text) {
            result.push_back(std::string(value_text));
        }
{% else if mv.type.elem_type.identifier == "int64_t" or mv.type.elem_type.identifier == "long" %}
        result.push_back(sqlite3_column_int64(stmt, 0));
{% else if mv.type.elem_type.is_integer %}
        result.push_back(sqlite3_column_int(stmt, 0));
{% else if mv.type.elem_type.is_real %}
        result.push_back(static_cast<{{mv.type.elem_type.estimated}}>(sqlite3_column_double(stmt, 0)));
{% else if mv.type.elem_type.is_bool %}
        result.push_back(sqlite3_column_int(stmt, 0) != 0);
{% else if mv.type.elem_type.is_char %}
        const char* char_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (char_text && char_text[0]) {
            result.push_back(char_text[0]);
        }
{% endif %}
    }
    
    sqlite3_finalize(stmt);
    return result;
}

bool SQLiteDB::delete{{struct.identifierCamel}}{{mv.identifierCamel}}(int64_t parent_id) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::delete{{struct.identifierCamel}}{{mv.identifierCamel}}() - Database not connected. Call connect() first.");
    }
    
    const char* sql = "DELETE FROM {{struct.identifier}}_{{mv.identifier}} WHERE {{struct.identifier}}_id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    
    sqlite3_bind_int64(stmt, 1, parent_id);
    
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    
    return success;
}

bool SQLiteDB::update{{struct.identifierCamel}}{{mv.identifierCamel}}(int64_t parent_id, const std::vector<{{mv.type.elem_type.estimated}}>& values) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::update{{struct.identifierCamel}}{{mv.identifierCamel}}() - Database not connected. Call connect() first.");
    }
    
    // Delete existing entries
    delete{{struct.identifierCamel}}{{mv.identifierCamel}}(parent_id);
    
    // Insert new entries
    if (!values.empty()) {
        const char* sql = "INSERT INTO {{struct.identifier}}_{{mv.identifier}} ({{struct.identifier}}_id, sequence, value) VALUES (?, ?, ?)";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
        }
        
        int sequence = 0;
        for (const auto& value : values) {
            sqlite3_reset(stmt);
            sqlite3_bind_int64(stmt, 1, parent_id);
            sqlite3_bind_int(stmt, 2, sequence++);
{% if mv.type.elem_type.is_string %}
            sqlite3_bind_text(stmt, 3, value.c_str(), -1, SQLITE_TRANSIENT);
{% else if mv.type.elem_type.identifier == "int64_t" or mv.type.elem_type.identifier == "long" %}
            sqlite3_bind_int64(stmt, 3, value);
{% else if mv.type.elem_type.is_integer %}
            sqlite3_bind_int(stmt, 3, value);
{% else if mv.type.elem_type.is_real %}
            sqlite3_bind_double(stmt, 3, static_cast<double>(value));
{% else if mv.type.elem_type.is_bool %}
            sqlite3_bind_int(stmt, 3, value ? 1 : 0);
{% else if mv.type.elem_type.is_char %}
            sqlite3_bind_text(stmt, 3, std::string(1, value).c_str(), -1, SQLITE_TRANSIENT);
{% endif %}
            
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sqlite3_finalize(stmt);
                throw std::runtime_error("Failed to insert {{mv.identifier}} array value: " + std::string(sqlite3_errmsg(db)));
            }
        }
        
        sqlite3_finalize(stmt);
    }
    
    return true;
}
{% endif %}
{% endfor %}

{% endfor %}

// Generic query builder factory
GenericSQLiteQueryBuilder SQLiteDB::Query() {
    return GenericSQLiteQueryBuilder(this);
}

void SQLiteDB::updateHook(int operation, const char* dbName, const char* tableName, sqlite3_int64 rowid){
    const char* opName;
    switch(operation) {
        case SQLITE_INSERT: opName = "INSERT"; break;
        case SQLITE_UPDATE: opName = "UPDATE"; break;
        case SQLITE_DELETE: opName = "DELETE"; break;
        default: opName = "UNKNOWN"; break;
    }
    
    printf("%s on %s.%s, rowid: %lld\n", opName, dbName, tableName, rowid);
}