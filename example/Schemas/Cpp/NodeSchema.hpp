#pragma once
#include "HasJsonSchema.hpp"
#include "HasLuaSchema.hpp"
#include "HasJavaSchema.hpp"
#include <sqlite3.h>
#include <mysqlx/xdevapi.h>
#include <iostream>
#include <string>
#include <vector>
class NodeSchema : public HasJsonSchema, public HasLuaSchema, public HasJavaSchema{
	static std::vector<NodeSchema*> all_nodes;
public:
	NodeSchema() {
		all_nodes.push_back(this);
	}

	~NodeSchema() {
		auto it = std::find(all_nodes.begin(), all_nodes.end(), this);
		if (it != all_nodes.end()) {
			all_nodes.erase(it);
		}
	}

	int64_t getid(){
		return this->id;
	}
	int64_t setid(int64_t id){
		this->id = id;
		return this->id;
	}
	std::string getname();
	std::string setname(std::string name);

    //json
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(NodeSchema, id, name);

	SCHEMA_DEFINE(NodeSchema,
        SCHEMA_FIELD(id, schema::desc("The unique identifier of the node")| schema::callback(json_field_id_options_callback)),
        SCHEMA_FIELD(name, schema::desc("The name of the node") | schema::callback(json_field_name_options_callback))
    );

    //lua
	void lua_push(lua_State* L) override;
	void lua_to(lua_State* L, int index) override;

    //sqlite
    static std::vector<NodeSchema*> SQLiteSelectByid(sqlite3 * db, int64_t id);
	static std::vector<NodeSchema*> SQLiteSelectByname(sqlite3 * db, std::string name);
    static bool SQLiteInsert(sqlite3 * db, int64_t id, std::string name);
	bool SQLiteInsert(sqlite3 * db);
	static bool SQLiteUpdate(sqlite3 * db, int64_t id, std::string name);
	bool SQLiteUpdate(sqlite3 * db);
	static bool SQLiteDelete(sqlite3 * db, int64_t id);
	bool SQLiteDelete(sqlite3 * db);
	static std::string getSQLiteCreateTableStatement();
	static bool SQLiteCreateTable(sqlite3 * db);


    //mysql
	static std::vector<NodeSchema*> MySQLSelectByid(mysqlx::Session & session, int64_t id);
	static std::vector<NodeSchema*> MySQLSelectByname(mysqlx::Session & session, std::string name);
	static bool MySQLInsert(mysqlx::Session & session, int64_t id, std::string name, std::string description);
	bool MySQLInsert(mysqlx::Session & session);
	static bool MySQLUpdate(mysqlx::Session & session, int64_t id, std::string name, std::string description);
	bool MySQLUpdate(mysqlx::Session & session);
	static bool MySQLDelete(mysqlx::Session & session, int64_t id);
	bool MySQLDelete(mysqlx::Session & session);
	static std::string getMySQLCreateTableStatement();
	static bool MySQLCreateTable(mysqlx::Session & session);

private:
	int64_t id;
	std::string name;
    NodeSchema *left;
    NodeSchema *right;

    // JSON specific field options callback 
    static void json_field_id_options_callback(FieldOptions&);
    static void json_field_name_options_callback(FieldOptions&);
};
