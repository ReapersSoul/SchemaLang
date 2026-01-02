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
	try{
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

    // Enable extended error codes for more detailed error information
    sqlite3_extended_result_codes(db, 1);

    // Foreign keys
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    
    // Register all SQLite callbacks
    sqlite3_update_hook(db, SQLiteDB::updateCallback, this);
    sqlite3_commit_hook(db, SQLiteDB::commitCallback, this);
    sqlite3_rollback_hook(db, SQLiteDB::rollbackCallback, this);
    sqlite3_trace_v2(db, SQLITE_TRACE_STMT | SQLITE_TRACE_PROFILE, SQLiteDB::traceCallback, this);
    sqlite3_progress_handler(db, 1000, SQLiteDB::progressCallback, this); // Check every 1000 VM ops
    sqlite3_set_authorizer(db, SQLiteDB::authorizerCallback, this);

    // Create version tracking table
    const char* version_table_sql = R"(
CREATE TABLE IF NOT EXISTS _schema_versions (
    table_name TEXT PRIMARY KEY,
    version_major INTEGER NOT NULL,
    version_minor INTEGER NOT NULL,
    version_patch INTEGER NOT NULL,
    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    migration_file TEXT
);
    )";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, version_table_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string error = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to create version tracking table: " + error);
    }

{% for enum in enums %}
        create{{enum.identifierCamel}}Table();
{% endfor %}
{% for struct in structs %}
        create{{struct.identifierCamel}}Table();
{% endfor %}

    // Auto-apply migrations for each struct
{% for struct in structs %}
    {
        // Check current version of {{struct.identifier}}
        const char* check_version_sql = "SELECT version_major, version_minor, version_patch FROM _schema_versions WHERE table_name = '{{struct.identifier}}' LIMIT 1;";
        sqlite3_stmt* stmt;
        int current_major = 0, current_minor = 0, current_patch = 0;
        bool has_version = false;
        
        if (sqlite3_prepare_v2(db, check_version_sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                current_major = sqlite3_column_int(stmt, 0);
                current_minor = sqlite3_column_int(stmt, 1);
                current_patch = sqlite3_column_int(stmt, 2);
                has_version = true;
            }
            sqlite3_finalize(stmt);
        }
        
        // Schema version for {{struct.identifier}}
        int schema_major = {{struct.version.major}};
        int schema_minor = {{struct.version.minor}};
        int schema_patch = {{struct.version.patch}};
        
        // Check if migration is needed
        if (has_version) {
            // Compare versions - only allow forward migrations
            if (current_major > schema_major || 
                (current_major == schema_major && current_minor > schema_minor) ||
                (current_major == schema_major && current_minor == schema_minor && current_patch > schema_patch)) {
                throw std::runtime_error("Database version for {{struct.identifier}} (" + 
                    std::to_string(current_major) + "." + std::to_string(current_minor) + "." + std::to_string(current_patch) + 
                    ") is newer than schema version (" + 
                    std::to_string(schema_major) + "." + std::to_string(schema_minor) + "." + std::to_string(schema_patch) + 
                    "). Downgrade migrations are not supported.");
            }
            
            // Apply migration if versions don't match
            if (current_major != schema_major || current_minor != schema_minor || current_patch != schema_patch) {
                // Get embedded migration SQL by checking for matching migration function
                std::string migration_sql;
                {% for migration in migrations %}{% if migration.structName == struct.identifier %}
                if (current_major == {{migration.fromVersion.major}} && current_minor == {{migration.fromVersion.minor}} && current_patch == {{migration.fromVersion.patch}} &&
                    schema_major == {{migration.toVersion.major}} && schema_minor == {{migration.toVersion.minor}} && schema_patch == {{migration.toVersion.patch}}) {
                    migration_sql = migrate_{{migration.structName}}_table_{{migration.fromVersion.major}}_{{migration.fromVersion.minor}}_{{migration.fromVersion.patch}}_to_{{migration.toVersion.major}}_{{migration.toVersion.minor}}_{{migration.toVersion.patch}}();
                }{% endif %}{% endfor %}
                
                if (!migration_sql.empty()) {
                    // Execute migration (already wrapped in transaction by generator)
                    char* migration_err = nullptr;
                    if (sqlite3_exec(db, migration_sql.c_str(), nullptr, nullptr, &migration_err) != SQLITE_OK) {
                        std::string error = migration_err ? migration_err : "Unknown error";
                        sqlite3_free(migration_err);
                        throw std::runtime_error("Failed to apply migration for {{struct.identifier}} from " +
                            std::to_string(current_major) + "." + std::to_string(current_minor) + "." + std::to_string(current_patch) +
                            " to " + std::to_string(schema_major) + "." + std::to_string(schema_minor) + "." + std::to_string(schema_patch) +
                            ": " + error);
                    }
                    
                    printf("Applied migration for {{struct.identifier}}: %d.%d.%d -> %d.%d.%d\n",
                           current_major, current_minor, current_patch,
                           schema_major, schema_minor, schema_patch);
                } else {
                    printf("Warning: No migration function found for {{struct.identifier}} from %d.%d.%d to %d.%d.%d\n",
                           current_major, current_minor, current_patch,
                           schema_major, schema_minor, schema_patch);
                }
            }
        } else {
            // First time - insert initial version
            const char* insert_version_sql = "INSERT OR REPLACE INTO _schema_versions (table_name, version_major, version_minor, version_patch) VALUES ('{{struct.identifier}}', ?, ?, ?);";
            sqlite3_stmt* insert_stmt;
            if (sqlite3_prepare_v2(db, insert_version_sql, -1, &insert_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(insert_stmt, 1, schema_major);
                sqlite3_bind_int(insert_stmt, 2, schema_minor);
                sqlite3_bind_int(insert_stmt, 3, schema_patch);
                sqlite3_step(insert_stmt);
                sqlite3_finalize(insert_stmt);
            }
        }
    }
{% endfor %}
	}
	catch (const std::exception& e) {
		printf("SQLiteDB::connect() - Exception during connect: %s\n", e.what());
		disconnect();
		throw; // Re-throw the exception after cleanup
	}
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

{% for enum in enums %}

void SQLiteDB::create{{enum.identifierCamel}}Table() {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::create{{enum.identifierCamel}}Table() - Database not connected. Call connect() first.");
    }
    const char* create_table_sql = R"(
CREATE TABLE IF NOT EXISTS {{enum.identifier}} (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    name TEXT NOT NULL UNIQUE,
    value INTEGER NOT NULL
);

INSERT INTO {{enum.identifier}} (name, value)
VALUES {% for value in enum.values %}('{{value.name}}', {{value.value}}){% if not loop.is_last %}, {% endif %}{% endfor %}
ON CONFLICT(name) DO NOTHING;
        )";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, create_table_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string error = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("SQLiteDB::create{{enum.identifierCamel}}Table() - SQL execution failed: " + error + "\nSQL: " + std::string(create_table_sql));
    }
}

{% endfor %}
{% for enum in enums %}

int64_t SQLiteDB::get{{enum.identifierCamel}}IdByName(const std::string& name) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::get{{enum.identifierCamel}}IdByName() - Database not connected.");
    }
    const char* sql = "SELECT id FROM {{enum.identifier}} WHERE name = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    
    int64_t id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return id;
}

int64_t SQLiteDB::get{{enum.identifierCamel}}IdByValue(int value) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::get{{enum.identifierCamel}}IdByValue() - Database not connected.");
    }
    const char* sql = "SELECT id FROM {{enum.identifier}} WHERE value = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_int(stmt, 1, value);
    
    int64_t id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return id;
}

std::optional<std::string> SQLiteDB::get{{enum.identifierCamel}}NameById(int64_t id) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::get{{enum.identifierCamel}}NameById() - Database not connected.");
    }
    const char* sql = "SELECT name FROM {{enum.identifier}} WHERE id = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_int64(stmt, 1, id);
    
    std::optional<std::string> name;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (name_text) {
            name = std::string(name_text);
        }
    }
    sqlite3_finalize(stmt);
    return name;
}

std::optional<int> SQLiteDB::get{{enum.identifierCamel}}ValueById(int64_t id) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::get{{enum.identifierCamel}}ValueById() - Database not connected.");
    }
    const char* sql = "SELECT value FROM {{enum.identifier}} WHERE id = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_int64(stmt, 1, id);
    
    std::optional<int> value;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        value = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return value;
}

std::optional<std::string> SQLiteDB::get{{enum.identifierCamel}}NameByValue(int value) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::get{{enum.identifierCamel}}NameByValue() - Database not connected.");
    }
    const char* sql = "SELECT name FROM {{enum.identifier}} WHERE value = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_int(stmt, 1, value);
    
    std::optional<std::string> name;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (name_text) {
            name = std::string(name_text);
        }
    }
    sqlite3_finalize(stmt);
    return name;
}

std::optional<int> SQLiteDB::get{{enum.identifierCamel}}ValueByName(const std::string& name) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::get{{enum.identifierCamel}}ValueByName() - Database not connected.");
    }
    const char* sql = "SELECT value FROM {{enum.identifier}} WHERE name = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    
    std::optional<int> value;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        value = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return value;
}

std::vector<std::pair<std::string, int>> SQLiteDB::getAll{{enum.identifierCamel}}Values() {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::getAll{{enum.identifierCamel}}Values() - Database not connected.");
    }
    std::vector<std::pair<std::string, int>> results;
    const char* sql = "SELECT name, value FROM {{enum.identifier}} ORDER BY value;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int value = sqlite3_column_int(stmt, 1);
        if (name_text) {
            results.push_back({std::string(name_text), value});
        }
    }
    sqlite3_finalize(stmt);
    return results;
}

{% endfor %}
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
{% set field_count = 0 %}
{% for field in struct.member_variables %}
    {% if not field.type.is_array %}
        {% set field_count = field_count + 1 %}
    {% endif %}
{% endfor %}
{% set current_field = 0 %}
{% for field in struct.member_variables %}
    {% if not field.type.is_array %}
        {% set current_field = current_field + 1 %}
    {% endif %}

    {% if field.type.is_array %}
    {% else if field.type.is_struct %}
        {{field.type.identifier}}_id INTEGER
    {% else if field.type.is_enum %}
        {{field.type.identifier}}_id INTEGER
    {% else %}
        {{ field.identifier }} {{ SQLite_convert_to_local_type(field.type) }} 
    {% endif %}

    {% if not field.type.is_array %}
        {% if field.type.required %} NOT NULL {% endif %}
        {% if field.unique %} UNIQUE {% endif %}
        {% if field.primary_key %} PRIMARY KEY {% endif %}
        {% if field.auto_increment %} AUTOINCREMENT {% endif %}
    {% endif %}

    {% if field.type.is_array %}
    {% else if field.type.is_struct %}
        REFERENCES {{field.type.identifier}}(id)
        DEFAULT 0
    {% else if field.type.is_enum %}
        REFERENCES {{field.type.identifier}}(id)
        DEFAULT 0
    {% else %}
        {% if field.reference.struct_name!="" %} REFERENCES {{field.reference.struct_name}}({{field.reference.variable_name}}) {% endif %}
        {% if field.default_value != "" %} DEFAULT {{SQLite_format_default(field.type, field.default_value)}}{% endif %}
    {% endif %}
    {% if not field.type.is_array %}
        {% if current_field < field_count or additional_field_count > 0 %},{% endif %}    
    {% endif %}
{% endfor %}
{% set current_field = 0 %}
{% for inner_struct in structs %}
    {% for mv in inner_struct.member_variables %}
        {% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set current_field = current_field + 1 %}
            {{inner_struct.identifier}}_id INTEGER REFERENCES {{inner_struct.identifier}}(id){% if current_field < additional_field_count %},{% endif %}
        {% endif %}
    {% endfor %}
{% endfor %}
);
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
CREATE TABLE IF NOT EXISTS {{struct.identifier}}_{{mv.identifier}} (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    {{struct.identifier}}_id INTEGER NOT NULL REFERENCES {{struct.identifier}}(id),
    sequence INTEGER NOT NULL,
    value {{ SQLite_convert_to_local_type(mv.type.elem_type) }} NOT NULL{% if mv.unique %},
    UNIQUE({{struct.identifier}}_id, value){% endif %},
    UNIQUE({{struct.identifier}}_id, sequence)
);
CREATE INDEX IF NOT EXISTS idx_{{struct.identifier}}_{{mv.identifier}}_parent_id ON {{struct.identifier}}_{{mv.identifier}}({{struct.identifier}}_id);
{% endif %}
{% endfor %}
        )";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, create_table_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string error = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("SQLiteDB::create{{struct.identifierCamel}}Table() - SQL execution failed: " + error + "\nSQL: " + std::string(create_table_sql));
    }
}

void SQLiteDB::create{{struct.identifierCamel}}Triggers() {
    // Create ON DELETE triggers for cascade deletions
{% for inner_struct in structs %}
{% for mv in inner_struct.member_variables %}
{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}
        const char* {{struct.identifier}}_{{inner_struct.identifier}}_trigger_sql = R"(
CREATE TRIGGER IF NOT EXISTS fk_{{struct.identifier}}_{{inner_struct.identifier}}_delete
BEFORE DELETE ON {{inner_struct.identifier}}
FOR EACH ROW
BEGIN
    DELETE FROM {{struct.identifier}} WHERE {{inner_struct.identifier}}_id = OLD.id;
END;
        )";
		char* {{struct.identifier}}_{{inner_struct.identifier}}errMsg = nullptr;
        if (sqlite3_exec(db, {{struct.identifier}}_{{inner_struct.identifier}}_trigger_sql, nullptr, nullptr, &{{struct.identifier}}_{{inner_struct.identifier}}errMsg) != SQLITE_OK) {
            std::string error = {{struct.identifier}}_{{inner_struct.identifier}}errMsg ? {{struct.identifier}}_{{inner_struct.identifier}}errMsg : "Unknown error";
            sqlite3_free({{struct.identifier}}_{{inner_struct.identifier}}errMsg);
            throw std::runtime_error("SQLiteDB::create{{struct.identifierCamel}}Table() - Failed to create trigger for {{inner_struct.identifier}}: " + error);
        }
{% endif %}
{% endfor %}
{% endfor %}

   // Create triggers for primitive arrays
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
        // Create ON DELETE trigger for cascade deletion
        const char* {{struct.identifier}}_{{mv.identifier}}_trigger_sql = R"(
CREATE TRIGGER IF NOT EXISTS fk_{{struct.identifier}}_{{mv.identifier}}_delete
BEFORE DELETE ON {{struct.identifier}}
FOR EACH ROW
BEGIN
    DELETE FROM {{struct.identifier}}_{{mv.identifier}} WHERE {{struct.identifier}}_id = OLD.id;
END;
        )";
		char* {{struct.identifier}}_{{mv.identifier}}errMsg = nullptr;
        if (sqlite3_exec(db, {{struct.identifier}}_{{mv.identifier}}_trigger_sql, nullptr, nullptr, &{{struct.identifier}}_{{mv.identifier}}errMsg) != SQLITE_OK) {
            std::string error = {{struct.identifier}}_{{mv.identifier}}errMsg ? {{struct.identifier}}_{{mv.identifier}}errMsg : "Unknown error";
            sqlite3_free({{struct.identifier}}_{{mv.identifier}}errMsg);
            throw std::runtime_error("SQLiteDB::create{{struct.identifierCamel}}Triggers() - Failed to create trigger for {{struct.identifier}}_{{mv.identifier}}: " + error);
        }
{% endif %}
{% endfor %}

    // Create triggers for other structs that reference this struct
{% for other_struct in structs %}
{% for field in other_struct.member_variables %}
{% if field.type.is_struct and field.type.identifier == struct.identifier %}
        const char* {{struct.identifier}}_{{other_struct.identifier}}_{{field.identifier}}_check_trigger_sql = R"(
CREATE TRIGGER IF NOT EXISTS fk_{{other_struct.identifier}}_{{field.identifier}}_delete_check
BEFORE DELETE ON {{struct.identifier}}
FOR EACH ROW
BEGIN
    DELETE FROM {{other_struct.identifier}} WHERE {{field.identifier}}_id = OLD.id;
END;
        )";
		char* {{struct.identifier}}_{{other_struct.identifier}}_{{field.identifier}}_errMsg = nullptr;
        if (sqlite3_exec(db, {{struct.identifier}}_{{other_struct.identifier}}_{{field.identifier}}_check_trigger_sql, nullptr, nullptr, &{{struct.identifier}}_{{other_struct.identifier}}_{{field.identifier}}_errMsg) != SQLITE_OK) {
            std::string error = {{struct.identifier}}_{{other_struct.identifier}}_{{field.identifier}}_errMsg ? {{struct.identifier}}_{{other_struct.identifier}}_{{field.identifier}}_errMsg : "Unknown error";
            sqlite3_free({{struct.identifier}}_{{other_struct.identifier}}_{{field.identifier}}_errMsg);
            throw std::runtime_error("SQLiteDB::create{{struct.identifierCamel}}Triggers() - Failed to create trigger for {{other_struct.identifier}}.{{field.identifier}}: " + error);
        }
{% endif %}
{% endfor %}
{% endfor %}
}

void SQLiteDB::delete{{struct.identifierCamel}}Triggers() {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::delete{{struct.identifierCamel}}Triggers() - Database not connected. Call connect() first.");
    }
    
    char* errMsg = nullptr;
    
    // Delete ON DELETE triggers for inner struct references
{% for inner_struct in structs %}
{% for mv in inner_struct.member_variables %}
{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}
    {
        const char* drop_trigger_sql = "DROP TRIGGER IF EXISTS fk_{{struct.identifier}}_{{inner_struct.identifier}}_delete;";
        if (sqlite3_exec(db, drop_trigger_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::string error = errMsg ? errMsg : "Unknown error";
            sqlite3_free(errMsg);
            throw std::runtime_error("SQLiteDB::delete{{struct.identifierCamel}}Triggers() - Failed to drop trigger fk_{{struct.identifier}}_{{inner_struct.identifier}}_delete: " + error);
        }
    }
{% endif %}
{% endfor %}
{% endfor %}
    
    // Delete triggers for primitive arrays
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
    {
        const char* drop_trigger_sql = "DROP TRIGGER IF EXISTS fk_{{struct.identifier}}_{{mv.identifier}}_delete;";
        if (sqlite3_exec(db, drop_trigger_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::string error = errMsg ? errMsg : "Unknown error";
            sqlite3_free(errMsg);
            throw std::runtime_error("SQLiteDB::delete{{struct.identifierCamel}}Triggers() - Failed to drop trigger fk_{{struct.identifier}}_{{mv.identifier}}_delete: " + error);
        }
    }
{% endif %}
{% endfor %}
    
    // Delete triggers for other structs that reference this struct
{% for other_struct in structs %}
{% for field in other_struct.member_variables %}
{% if field.type.is_struct and field.type.identifier == struct.identifier %}
    {
        const char* drop_trigger_sql = "DROP TRIGGER IF EXISTS fk_{{other_struct.identifier}}_{{field.identifier}}_delete_check;";
        if (sqlite3_exec(db, drop_trigger_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::string error = errMsg ? errMsg : "Unknown error";
            sqlite3_free(errMsg);
            throw std::runtime_error("SQLiteDB::delete{{struct.identifierCamel}}Triggers() - Failed to drop trigger fk_{{other_struct.identifier}}_{{field.identifier}}_delete_check: " + error);
        }
    }
{% endif %}
{% endfor %}
{% endfor %}
}

std::vector<std::shared_ptr<{{struct.identifier}}Schema>> SQLiteDB::selectAll{{struct.identifierCamel}}() {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::selectAll{{struct.identifierCamel}}() - Database not connected. Call connect() first.");
    }
    
    std::vector<std::shared_ptr<{{struct.identifier}}Schema>> results;
    const char* sql = R"(SELECT 
    {% set field_count = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set additional_field_count = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set additional_field_count = additional_field_count + 1 %}{% endif %}{% endfor %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set current_field = current_field + 1 %}{% if field.type.is_struct or field.type.is_enum %}{{field.type.identifier}}_id{% else %}{{field.identifier}}{% endif %}{% if current_field < field_count or additional_field_count > 0 %}, {% endif %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set current_field = current_field + 1 %}{{inner_struct.identifier}}_id{% if current_field < additional_field_count %}, {% endif %}{% endif %}{% endfor %}{% endfor %}
    
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
   {% set field_count = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set current_field = current_field + 1 %}{% if field.type.is_struct or field.type.is_enum %}{{field.type.identifier}}_id{% else %}{{field.identifier}}{% endif %}{% if current_field < field_count %}, {% endif %}{% endif %}{% endfor %}
    
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
{% if not field.type.is_array %}
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
        {% set field_count = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set current_field = current_field + 1 %}{% if field.type.is_struct or field.type.is_enum %}{{field.type.identifier}}_id{% else %}{{field.identifier}}{% endif %}{% if current_field < field_count %}, {% endif %}{% endif %}{% endfor %}

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
        {% set field_count = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set current_field = current_field + 1 %}{% if field.type.is_struct or field.type.is_enum %}{{field.type.identifier}}_id{% else %}{{field.identifier}}{% endif %}{% if current_field < field_count %}, {% endif %}{% endif %}{% endfor %}
        
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
    return SQLiteQueryBuilder(shared_from_this()).From("{{struct.identifier}}");
}

SQLiteQueryBuilder SQLiteDB::Query{{struct.identifierCamel}}(){
    return SQLiteQueryBuilder(shared_from_this());
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
    const char* sql_filter_id = "INSERT OR REPLACE INTO {{struct.identifier}} ({% set field_count = 0 %}{% for field in struct.member_variables %}{% if field.identifier != "id" and not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set additional_field_count = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set additional_field_count = additional_field_count + 1 %}{% endif %}{% endfor %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}{% if field.identifier != "id" and not field.type.is_array %}{% set current_field = current_field + 1 %}{% if field.type.is_struct or field.type.is_enum %}{{field.type.identifier}}_id{% else %}{{field.identifier}}{% endif %}{% if current_field < field_count or additional_field_count > 0 %}, {% endif %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set current_field = current_field + 1 %}{{inner_struct.identifier}}_id{% if current_field < additional_field_count %}, {% endif %}{% endif %}{% endfor %}{% endfor %}) VALUES ({% set current_param = 0 %}{% for field in struct.member_variables %}{% if field.identifier != "id" and not field.type.is_array %}{% set current_param = current_param + 1 %}?{% if current_param < field_count or additional_field_count > 0 %}, {% endif %}{% endif %}{% endfor %}{% set current_param = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set current_param = current_param + 1 %}?{% if current_param < additional_field_count %}, {% endif %}{% endif %}{% endfor %}{% endfor %})";
    const char* sql = "INSERT OR REPLACE INTO {{struct.identifier}} ({% set field_count = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set field_count = field_count + 1 %}{% endif %}{% endfor %}{% set additional_field_count = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set additional_field_count = additional_field_count + 1 %}{% endif %}{% endfor %}{% endfor %}{% set current_field = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set current_field = current_field + 1 %}{% if field.type.is_struct or field.type.is_enum %}{{field.type.identifier}}_id{% else %}{{field.identifier}}{% endif %}{% if current_field < field_count or additional_field_count > 0 %}, {% endif %}{% endif %}{% endfor %}{% set current_field = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set current_field = current_field + 1 %}{{inner_struct.identifier}}_id{% if current_field < additional_field_count %}, {% endif %}{% endif %}{% endfor %}{% endfor %}) VALUES ({% set current_param = 0 %}{% for field in struct.member_variables %}{% if not field.type.is_array %}{% set current_param = current_param + 1 %}?{% if current_param < field_count or additional_field_count > 0 %}, {% endif %}{% endif %}{% endfor %}{% set current_param = 0 %}{% for inner_struct in structs %}{% for mv in inner_struct.member_variables %}{% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == struct.identifier %}{% set current_param = current_param + 1 %}?{% if current_param < additional_field_count %}, {% endif %}{% endif %}{% endfor %}{% endfor %})";
    
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
    
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (result != SQLITE_DONE) {
        int err = sqlite3_errcode(db);
        int ext_err = sqlite3_extended_errcode(db);
        std::string error_msg = sqlite3_errmsg(db);
        
        // Check if it's a foreign key constraint error
        if (err == SQLITE_CONSTRAINT || ext_err == SQLITE_CONSTRAINT_FOREIGNKEY) {
            std::string fk_details = formatForeignKeyError("DELETE", "{{struct.identifier}}", id);
            throw std::runtime_error("SQLiteDB::delete{{struct.identifierCamel}}ById(" + std::to_string(id) + ") - Foreign key constraint violation: " + error_msg + "\n" + fk_details);
        }
        
        throw std::runtime_error("SQLiteDB::delete{{struct.identifierCamel}}ById(" + std::to_string(id) + ") - Failed to delete: " + error_msg + " (error code: " + std::to_string(err) + ", extended: " + std::to_string(ext_err) + ")");
    }
    
    return true;
}

bool SQLiteDB::delete{{struct.identifierCamel}}ByIdCascade(int64_t id) {
    if (!isConnected()) {
        throw std::runtime_error("SQLiteDB::delete{{struct.identifierCamel}}ByIdCascade(" + std::to_string(id) + ") - Database not connected. Call connect() first.");
    }
    
    // First, recursively delete nested struct array items
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.elem_type.is_struct %}
    auto nested_{{mv.identifier}}_items = select{{mv.type.elem_type.identifier}}By{{struct.identifier}}_id(id);
    for (const auto& item : nested_{{mv.identifier}}_items) {
        delete{{mv.type.elem_type.identifierCamel}}ByIdCascade(item->getId());
    }
{% endif %}
{% endfor %}

    // Delete primitive array entries (stored in child tables)
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
    delete{{struct.identifierCamel}}{{mv.identifierCamel}}(id);
{% endif %}
{% endfor %}

    // Delete referenced struct objects if they exist
{% for mv in struct.member_variables %}
{% if mv.type.is_struct and not mv.type.is_array %}
        auto {{struct.identifierCamel}}_{{mv.identifierCamel}}_obj = select{{struct.identifierCamel}}ById(id);
        if ({{struct.identifierCamel}}_{{mv.identifierCamel}}_obj) {
{% if mv.type.required %}
            if ({{struct.identifierCamel}}_{{mv.identifierCamel}}_obj->get{{mv.identifierCamel}}()) {
                delete{{mv.type.identifierCamel}}ByIdCascade({{struct.identifierCamel}}_{{mv.identifierCamel}}_obj->get{{mv.identifierCamel}}()->getId());
            }
{% else %}
            if ({{struct.identifierCamel}}_{{mv.identifierCamel}}_obj->get{{mv.identifierCamel}}().has_value() && {{struct.identifierCamel}}_{{mv.identifierCamel}}_obj->get{{mv.identifierCamel}}().value()) {
                delete{{mv.type.identifierCamel}}ByIdCascade({{struct.identifierCamel}}_{{mv.identifierCamel}}_obj->get{{mv.identifierCamel}}().value()->getId());
            }
{% endif %}
        }
{% endif %}
{% endfor %}

    // Now delete the main object (triggers will handle remaining FK cascades)
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
    return GenericSQLiteQueryBuilder(shared_from_this());
}

// SQLite callback hook implementations
void SQLiteDB::updateHook(int operation, const char* dbName, const char* tableName, sqlite3_int64 rowid){
    const char* opName;
    switch(operation) {
        case SQLITE_INSERT: opName = "INSERT"; break;
        case SQLITE_UPDATE: opName = "UPDATE"; break;
        case SQLITE_DELETE: opName = "DELETE"; break;
        default: opName = "UNKNOWN"; break;
    }
    
    // Default implementation - can be overridden in derived classes
    printf("%s on %s.%s, rowid: %lld\n", opName, dbName, tableName, rowid);
}

int SQLiteDB::commitHook() {
    // Default implementation - return 0 to allow commit, non-zero to rollback
    // Override in derived class to add custom commit logic
    return 0;
}

void SQLiteDB::rollbackHook() {
    // Default implementation - override in derived class to add custom rollback logic
}

void SQLiteDB::traceHook(unsigned int traceType, void* pCtx, void* p, void* x) {
    // Default implementation - override in derived class for custom tracing
    if (traceType == SQLITE_TRACE_STMT) {
        sqlite3_stmt* stmt = static_cast<sqlite3_stmt*>(p);
        char* sql = sqlite3_expanded_sql(stmt);
        if (sql) {
            printf("SQL: %s\n", sql);
            sqlite3_free(sql);
        }
    } else if (traceType == SQLITE_TRACE_PROFILE) {
        sqlite3_stmt* stmt = static_cast<sqlite3_stmt*>(p);
        sqlite3_int64* nanoseconds = static_cast<sqlite3_int64*>(x);
        printf("Profile: %lld ns\n", *nanoseconds);
    }
}

int SQLiteDB::progressHook() {
    // Default implementation - return 0 to continue, non-zero to interrupt
    // Override in derived class to add custom progress monitoring
    return 0;
}

int SQLiteDB::authorizerHook(int actionCode, const char* detail1, const char* detail2, 
                            const char* dbName, const char* triggerOrView) {
    // Default implementation - return SQLITE_OK to allow, SQLITE_DENY to deny, SQLITE_IGNORE to ignore
    // Override in derived class to add custom authorization logic
    
    // Example of what you might check:
    // if (actionCode == SQLITE_DELETE && detail1 && strcmp(detail1, "sensitive_table") == 0) {
    //     return SQLITE_DENY;
    // }
    
    return SQLITE_OK;
}

// Foreign key debugging helper implementations
std::vector<SQLiteDB::ForeignKeyViolation> SQLiteDB::checkForeignKeyViolations() {
    std::vector<ForeignKeyViolation> violations;
    
    if (!isConnected()) {
        return violations;
    }
    
    const char* sql = "PRAGMA foreign_key_check;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return violations;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ForeignKeyViolation violation;
        
        const char* table_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        violation.table = table_text ? table_text : "";
        
        violation.rowid = sqlite3_column_int64(stmt, 1);
        
        const char* parent_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        violation.parent = parent_text ? parent_text : "";
        
        violation.fkid = sqlite3_column_int(stmt, 3);
        
        violations.push_back(violation);
    }
    
    sqlite3_finalize(stmt);
    return violations;
}

std::vector<SQLiteDB::ForeignKeyInfo> SQLiteDB::getForeignKeyList(const std::string& table_name) {
    std::vector<ForeignKeyInfo> fk_list;
    
    if (!isConnected()) {
        return fk_list;
    }
    
    std::string sql = "PRAGMA foreign_key_list(" + table_name + ");";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return fk_list;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ForeignKeyInfo info;
        
        info.id = sqlite3_column_int(stmt, 0);
        info.seq = sqlite3_column_int(stmt, 1);
        
        const char* table_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.table = table_text ? table_text : "";
        
        const char* from_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.from = from_text ? from_text : "";
        
        const char* to_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        info.to = to_text ? to_text : "";
        
        const char* on_update_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.on_update = on_update_text ? on_update_text : "";
        
        const char* on_delete_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        info.on_delete = on_delete_text ? on_delete_text : "";
        
        fk_list.push_back(info);
    }
    
    sqlite3_finalize(stmt);
    return fk_list;
}

std::string SQLiteDB::formatForeignKeyError(const std::string& operation, const std::string& table, int64_t id) {
    std::string error_details = "Foreign key constraint details:\\n";
    
    auto violations = checkForeignKeyViolations();
    
    if (violations.empty()) {
        error_details += "  No violations found in PRAGMA foreign_key_check (constraint may be preventing " + operation + ")\\n";
        
        // Get FK list for the table being operated on
        auto fk_list = getForeignKeyList(table);
        if (!fk_list.empty()) {
            error_details += "  Foreign keys defined on table '" + table + "':\\n";
            for (const auto& fk : fk_list) {
                error_details += "    - " + fk.from + " -> " + fk.table + "(" + fk.to + ")";
                error_details += " [on_delete: " + fk.on_delete + ", on_update: " + fk.on_update + "]\\n";
            }
        }
    } else {
        error_details += "  Violations detected:\\n";
        for (const auto& violation : violations) {
            error_details += "    - Child table '" + violation.table + "' (rowid: " + std::to_string(violation.rowid) + ")";
            error_details += " references parent table '" + violation.parent + "' (fkid: " + std::to_string(violation.fkid) + ")\\n";
            
            // Get detailed FK info
            auto fk_list = getForeignKeyList(violation.table);
            for (const auto& fk : fk_list) {
                if (fk.id == violation.fkid) {
                    error_details += "      Column '" + fk.from + "' references " + fk.table + "(" + fk.to + ")\\n";
                    break;
                }
            }
        }
    }
    
    error_details += "  Attempted operation: " + operation + " on table '" + table + "' (id: " + std::to_string(id) + ")";
    
    return error_details;
}

// Migration functions
{% for migration in migrations %}
std::string SQLiteDB::migrate_{{migration.structName}}_table_{{migration.fromVersion.major}}_{{migration.fromVersion.minor}}_{{migration.fromVersion.patch}}_to_{{migration.toVersion.major}}_{{migration.toVersion.minor}}_{{migration.toVersion.patch}}() {
    return R"SQL(
-- Migration: {{migration.structName}} from {{migration.fromVersion.major}}.{{migration.fromVersion.minor}}.{{migration.fromVersion.patch}} to {{migration.toVersion.major}}.{{migration.toVersion.minor}}.{{migration.toVersion.patch}}
-- Generated by SchemaLang Transpiler

BEGIN TRANSACTION;

{% for op in migration.operations %}{% if op.type == "AddField" %}ALTER TABLE {{migration.structName}} ADD COLUMN {{op.fieldName}} TEXT NOT NULL DEFAULT '';
{% else %}{% if op.type == "RemoveField" %}-- WARNING: Dropping column {{op.fieldName}} will cause data loss
ALTER TABLE {{migration.structName}} DROP COLUMN {{op.fieldName}};
{% else %}{% if op.type == "RenameField" %}ALTER TABLE {{migration.structName}} RENAME COLUMN {{op.fieldName}} TO {{op.fieldName}};
{% else %}{% if op.type == "ChangeType" %}-- WARNING: Changing column type requires table recreation in SQLite
-- Manual migration required for type change on column: {{op.fieldName}}
{% endif %}{% endif %}{% endif %}{% endif %}{% endfor %}
-- Update schema version
INSERT OR REPLACE INTO _schema_versions (table_name, version_major, version_minor, version_patch, applied_at) VALUES ('{{migration.structName}}', {{migration.toVersion.major}}, {{migration.toVersion.minor}}, {{migration.toVersion.patch}}, datetime('now'));

COMMIT;
)SQL";
}

{% endfor %}