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
    try {
        if (sqlite3_open(db_path.string().c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Can't open database: " + std::string(sqlite3_errmsg(db)));
        }
        // Optionally, set up any required pragmas or configurations here
        // For example, enable foreign key support
        sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
        sqlite3_update_hook(db, SQLiteDB::updateCallback, this);

        {% for struct in structs %}
        create{{struct.identifierCamel}}Table();
        {% endfor %}

    } catch (const std::exception &ex) {
        throw std::runtime_error("STD Exception: " + std::string(ex.what()));
    } catch (...) {
        throw std::runtime_error("Unknown exception during connection");
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
        throw std::runtime_error("Database is not connected.");
    }
    return db;
};

{% for struct in structs %}

void SQLiteDB::create{{struct.identifierCamel}}Table() {
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    const char* create_table_sql = R"(
        CREATE TABLE IF NOT EXISTS {{struct.identifier}} (
            {% for field in struct.member_variables -%}
                {{- field.identifier }} {{ field.type.SQLite_estimated }}{% if field.required %} NOT NULL{% endif %}{% if field.unique %} UNIQUE{% endif %}{% if field.primary_key %} PRIMARY KEY{% endif %}{% if field.auto_increment %} AUTOINCREMENT{% endif %}{% if field.reference %} REFERENCES {{field.reference.struct_name}}({{field.reference.variable_name}}){% endif %}{% if field.default_value %} DEFAULT {{field.default_value}}{% endif %}{% if not loop.is_last %},{% endif %}
            {% endfor -%});
    )";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, create_table_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string error = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("SQL error: " + error);
    }
}

std::shared_ptr<{{struct.identifier}}Schema> SQLiteDB::select{{struct.identifierCamel}}ById(int64_t id){
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    
}

//updateInsert
int64_t SQLiteDB::insertOrUpdate{{struct.identifierCamel}}(std::shared_ptr<{{struct.identifier}}Schema> obj) {
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    
}

bool SQLiteDB::delete{{struct.identifierCamel}}ById(int64_t id) {
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    
}
{% endfor %}