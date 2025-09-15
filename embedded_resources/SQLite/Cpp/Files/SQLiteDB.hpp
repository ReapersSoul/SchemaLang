#pragma once
#include <vector>
#include <optional>
#include <string>
#include <set>
#include <map>
#include <stdexcept>
#include <sqlite3.h>
#include <filesystem>
{% for struct in structs %}
class {{struct.identifier}}Schema;
{% endfor %}
{# {% for enum in enums %}
enum class {{enum.identifier}}Schema;
{% endfor %} #}


class SQLiteDB
{
public:
    virtual ~SQLiteDB();
    SQLiteDB(std::filesystem::path db_path);
    void connect();
    void disconnect();
    bool isConnected() const;
    sqlite3* getDB();
{% for struct in structs %}
    virtual void create{{struct.identifierCamel}}Table();

    virtual std::vector<std::shared_ptr<{{struct.identifier}}Schema>> selectAll{{struct.identifierCamel}}();

    virtual std::shared_ptr<{{struct.identifier}}Schema> select{{struct.identifierCamel}}ById(int64_t id);

{% for mv in struct.member_variables %}
{% if not (mv.type.is_array or mv.type.is_struct or mv.type.is_enum) %}
{% if mv.identifier != "id" %}
    virtual std::vector<std::shared_ptr<{{struct.identifier}}Schema>> select{{struct.identifierCamel}}By{{mv.identifierCamel}}({{mv.type.estimated}} value);
{% endif %}
{% endif %}
{% endfor %}

    virtual int64_t insertOrUpdate{{struct.identifierCamel}}(std::shared_ptr<{{struct.identifier}}Schema> obj);

    virtual bool delete{{struct.identifierCamel}}ById(int64_t id);

{% endfor %}

private:
    std::filesystem::path db_path;
    sqlite3* db;

    static void updateCallback(void* userData, int operation, const char* dbName, 
                   const char* tableName, sqlite3_int64 rowid) {
                    SQLiteDB* self = static_cast<SQLiteDB*>(userData);
    const char* opName;
    switch(operation) {
        case SQLITE_INSERT: opName = "INSERT"; break;
        case SQLITE_UPDATE: opName = "UPDATE"; break;
        case SQLITE_DELETE: opName = "DELETE"; break;
        default: opName = "UNKNOWN"; break;
    }
    
    printf("%s on %s.%s, rowid: %lld\n", opName, dbName, tableName, rowid);
}
};