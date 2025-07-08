#include "NodeSchema.hpp"

int64_t NodeSchema::getid()
{
    return id;
}

int64_t NodeSchema::setid(int64_t id)
{
    this->id = id;
    return this->id;
}

std::string NodeSchema::getname()
{
    return name;
}

std::string NodeSchema::setname(std::string name)
{
    this->name = name;
    return this->name;
}

// JSON methods
nlohmann::json NodeSchema::toJSON()
{
    nlohmann::json j;
    j["id"] = id;
    j["name"] = name;
    return j;
}

void NodeSchema::fromJSON(nlohmann::json j)
{
    id = j["id"].get<int64_t>();
    name = j["name"].get<std::string>();
}

nlohmann::json NodeSchema::getJsonSchema()
{
    return nlohmann::json::parse("{\n\t\"properties\": {\n\t\t\"id\": {\n\t\t\t\"description\": \"The unique identifier of the ability effect\",\n\t\t\t\"type\": \"number\"\n\t\t},\n\t\t\"name\": {\n\t\t\t\"description\": \"The name of the ability effect\",\n\t\t\t\"type\": \"string\"\n\t\t}\n\t},\n\t\"required\": [\n\t\t\"id\",\n\t\t\"name\"\n\t],\n\t\"title\": \"Node\",\n\t\"type\": \"object\"\n}");
}

// Lua methods
void NodeSchema::lua_push(lua_State *L)
{
    lua_newtable(L);

    // Create userdata for this NodeSchema instance
    NodeSchema **userdata = (NodeSchema **)lua_newuserdata(L, sizeof(NodeSchema *));
    *userdata = this;

    // Set metatable for the userdata
    luaL_getmetatable(L, "NodeSchema");
    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "_self");

    // Set up metamethods to sync with C++ object
    lua_newtable(L); // metatable for the main table

    // __index metamethod
    lua_pushcfunction(L, [](lua_State *L) -> int
                      {
        const char* key = lua_tostring(L, 2);
        lua_getfield(L, 1, "_self");
        NodeSchema* self = *(NodeSchema**)lua_touserdata(L, -1);
        lua_pop(L, 1);
        
        if (strcmp(key, "id") == 0) {
            lua_pushinteger(L, self->getid());
        } else if (strcmp(key, "name") == 0) {
            lua_pushstring(L, self->getname().c_str());
        } else {
            lua_pushnil(L);
        }
        return 1; });
    lua_setfield(L, -2, "__index");

    // __newindex metamethod
    lua_pushcfunction(L, [](lua_State *L) -> int
                      {
        const char* key = lua_tostring(L, 2);
        lua_getfield(L, 1, "_self");
        NodeSchema* self = *(NodeSchema**)lua_touserdata(L, -1);
        lua_pop(L, 1);
        
        if (strcmp(key, "id") == 0) {
            self->setid(lua_tointeger(L, 3));
        } else if (strcmp(key, "name") == 0) {
            self->setname(lua_tostring(L, 3));
        }
        return 0; });
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);
}

void NodeSchema::lua_to(lua_State *L, int index)
{
    if (!lua_istable(L, index))
    {
        luaL_error(L, "Expected a table for NodeSchema");
        return;
    }

    lua_getfield(L, index, "id");
    if (lua_isinteger(L, -1))
    {
        id = lua_tointeger(L, -1);
    }
    else
    {
        luaL_error(L, "Invalid type for id");
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "name");
    if (lua_isstring(L, -1))
    {
        name = lua_tostring(L, -1);
    }
    else
    {
        luaL_error(L, "Invalid type for name");
    }
    lua_pop(L, 1);
}

// SQLite methods
std::vector<NodeSchema *> NodeSchema::SQLiteSelectByid(sqlite3 *db, int64_t id)
{
    std::vector<NodeSchema *> results;
    sqlite3_stmt *stmt;
    const char *sql = "SELECT * FROM NodeSchema WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "SQLite error: " << sqlite3_errmsg(db) << std::endl;
        return results;
    }

    sqlite3_bind_int64(stmt, 1, id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        NodeSchema *node = new NodeSchema();
        node->id = sqlite3_column_int64(stmt, 0);
        node->name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        results.push_back(node);
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<NodeSchema *> NodeSchema::SQLiteSelectByname(sqlite3 *db, std::string name)
{
    std::vector<NodeSchema *> results;
    sqlite3_stmt *stmt;
    const char *sql = "SELECT * FROM NodeSchema WHERE name = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "SQLite error: " << sqlite3_errmsg(db) << std::endl;
        return results;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        NodeSchema *node = new NodeSchema();
        node->id = sqlite3_column_int64(stmt, 0);
        node->name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        results.push_back(node);
    }

    sqlite3_finalize(stmt);
    return results;
}

bool NodeSchema::SQLiteInsert(sqlite3 *db, int64_t id, std::string name)
{
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO NodeSchema (id, name) VALUES (?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "SQLite error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "SQLite error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool NodeSchema::SQLiteInsert(sqlite3 *db)
{
    return SQLiteInsert(db, id, name);
}

bool NodeSchema::SQLiteUpdate(sqlite3 *db, int64_t id, std::string name)
{
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE NodeSchema SET name = ? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "SQLite error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "SQLite error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool NodeSchema::SQLiteUpdate(sqlite3 *db)
{
    return SQLiteUpdate(db, id, name);
}

bool NodeSchema::SQLiteDelete(sqlite3 *db, int64_t id)
{
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM NodeSchema WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "SQLite error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "SQLite error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool NodeSchema::SQLiteDelete(sqlite3 *db)
{
    return SQLiteDelete(db, id);
}

std::string NodeSchema::getSQLiteCreateTableStatement()
{
    return "CREATE TABLE IF NOT EXISTS NodeSchema (id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE NOT NULL, name TEXT NOT NULL);";
}

bool NodeSchema::SQLiteCreateTable(sqlite3 *db)
{
    char *errMsg = nullptr;
    const char *sql = getSQLiteCreateTableStatement().c_str();

    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::cerr << "SQLite error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    // node schema sqlite on update
    int rx = sqlite3_create_function(nullptr, "node_schema_on_update", 2, SQLITE_UTF8, nullptr, [](sqlite3_context *context, int argc, sqlite3_value **argv)
                                     {
                                         // find the NodeSchema object with the given id
                                         if (argc != 2 || sqlite3_value_type(argv[0]) != SQLITE_INTEGER || sqlite3_value_type(argv[1]) != SQLITE_TEXT)
                                         {
                                             sqlite3_result_error(context, "Invalid arguments for node_schema_on_update", -1);
                                             return;
                                         }
                                         int64_t id = sqlite3_value_int64(argv[0]);
                                         std::string name = reinterpret_cast<const char *>(sqlite3_value_text(argv[1]));
                                         NodeSchema *node = nullptr;
                                         for (NodeSchema *n : all_nodes)
                                         {
                                             if (n->getid() == id)
                                             {
                                                 node = n;
                                                 break;
                                             }
                                         }
                                         if (!node)
                                         {
                                             sqlite3_result_error(context, "NodeSchema with given id not found", -1);
                                             return;
                                         }
                                         // Update the node schema
                                         node->setname(name);
                                         sqlite3_result_int(context, 1);
                                     },
                                     nullptr, nullptr);
    if (rx != SQLITE_OK)
    {
        std::cerr << "Error creating node_schema_on_update function: " << sqlite3_errmsg(nullptr) << std::endl;
    }

    return true;
}

// MySQL methods
std::vector<NodeSchema *> NodeSchema::MySQLSelectByid(mysqlx::Session &session, int64_t id)
{
    std::vector<NodeSchema *> results;
    mysqlx::SqlStatement stmt = session.sql("SELECT * FROM NodeSchema WHERE id = ?");
    stmt.bind(id);

    mysqlx::RowResult res = stmt.execute();
    for (mysqlx::Row row : res)
    {
        NodeSchema *node = new NodeSchema();
        node->id = row[0].get<int64_t>();
        node->name = row[1].get<std::string>();
        results.push_back(node);
    }

    return results;
}

std::vector<NodeSchema *> NodeSchema::MySQLSelectByname(mysqlx::Session &session, std::string name)
{
    std::vector<NodeSchema *> results;
    mysqlx::SqlStatement stmt = session.sql("SELECT * FROM NodeSchema WHERE name = ?");
    stmt.bind(name);

    mysqlx::RowResult res = stmt.execute();
    for (mysqlx::Row row : res)
    {
        NodeSchema *node = new NodeSchema();
        node->id = row[0].get<int64_t>();
        node->name = row[1].get<std::string>();
        results.push_back(node);
    }

    return results;
}

bool NodeSchema::MySQLInsert(mysqlx::Session &session, int64_t id, std::string name)
{
    mysqlx::SqlStatement stmt = session.sql("INSERT INTO NodeSchema (id, name) VALUES (?, ?)");
    stmt.bind(id, name);

    try
    {
        stmt.execute();
    }
    catch (const mysqlx::Error &err)
    {
        std::cerr << "MySQL error: " << err.what() << std::endl;
        return false;
    }

    return true;
}

bool NodeSchema::MySQLInsert(mysqlx::Session &session)
{
    return MySQLInsert(session, id, name);
}

bool NodeSchema::MySQLUpdate(mysqlx::Session &session, int64_t id, std::string name)
{
    mysqlx::SqlStatement stmt = session.sql("UPDATE NodeSchema SET name = ? WHERE id = ?");
    stmt.bind(name, id);

    try
    {
        stmt.execute();
    }
    catch (const mysqlx::Error &err)
    {
        std::cerr << "MySQL error: " << err.what() << std::endl;
        return false;
    }

    return true;
}

bool NodeSchema::MySQLUpdate(mysqlx::Session &session)
{
    return MySQLUpdate(session, id, name);
}

bool NodeSchema::MySQLDelete(mysqlx::Session &session, int64_t id)
{
    mysqlx::SqlStatement stmt = session.sql("DELETE FROM NodeSchema WHERE id = ?");
    stmt.bind(id);

    try
    {
        stmt.execute();
    }
    catch (const mysqlx::Error &err)
    {
        std::cerr << "MySQL error: " << err.what() << std::endl;
        return false;
    }

    return true;
}

bool NodeSchema::MySQLDelete(mysqlx::Session &session)
{
    return MySQLDelete(session, id);
}

std::string NodeSchema::getMySQLCreateTableStatement()
{
    return "CREATE TABLE IF NOT EXISTS NodeSchema (id BIGINT PRIMARY KEY AUTO_INCREMENT UNIQUE NOT NULL, name VARCHAR(255) NOT NULL);";
}

bool NodeSchema::MySQLCreateTable(mysqlx::Session &session)
{
    try
    {
        session.sql(getMySQLCreateTableStatement()).execute();
    }
    catch (const mysqlx::Error &err)
    {
        std::cerr << "MySQL error: " << err.what() << std::endl;
        return false;
    }

    return true;
}