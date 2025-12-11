#pragma once
#include <vector>
#include <optional>
#include <string>
#include <set>
#include <map>
#include <stdexcept>
#include <sqlite3.h>
#include <filesystem>
#include {{format_include("SQLiteQueryBuilder.hpp")}}
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
    virtual void connect();
    virtual void disconnect();
    virtual bool isConnected() const;
    virtual sqlite3* getDB();
{% for enum in enums %}
	virtual void create{{enum.identifierCamel}}Table();
	virtual int64_t get{{enum.identifierCamel}}IdByName(const std::string& name);
	virtual int64_t get{{enum.identifierCamel}}IdByValue(int value);
	virtual std::optional<std::string> get{{enum.identifierCamel}}NameById(int64_t id);
	virtual std::optional<int> get{{enum.identifierCamel}}ValueById(int64_t id);
	virtual std::optional<std::string> get{{enum.identifierCamel}}NameByValue(int value);
	virtual std::optional<int> get{{enum.identifierCamel}}ValueByName(const std::string& name);
	virtual std::vector<std::pair<std::string, int>> getAll{{enum.identifierCamel}}Values();
{% endfor %}

{% for struct in structs %}
    virtual void create{{struct.identifierCamel}}Table();
    virtual void create{{struct.identifierCamel}}Triggers();
	virtual void delete{{struct.identifierCamel}}Triggers();

    virtual std::vector<std::shared_ptr<{{struct.identifier}}Schema>> selectAll{{struct.identifierCamel}}();

    virtual std::shared_ptr<{{struct.identifier}}Schema> select{{struct.identifierCamel}}ById(int64_t id);

{% for mv in struct.member_variables %}
{% if not (mv.type.is_array or mv.type.is_struct or mv.type.is_enum) %}
{% if mv.identifier != "id" %}
    virtual std::vector<std::shared_ptr<{{struct.identifier}}Schema>> select{{struct.identifierCamel}}By{{mv.identifierCamel}}({{mv.type.estimated}} value);
{% endif %}
{% endif %}
{% endfor %}

{# Generate selectBy methods for parent relationships #}
{% for other_struct in structs %}
{% for other_mv in other_struct.member_variables %}
{% if other_mv.type.is_array and other_mv.type.elem_type.is_struct and other_mv.type.elem_type.identifier == struct.identifier %}
    virtual std::vector<std::shared_ptr<{{struct.identifier}}Schema>> select{{struct.identifierCamel}}By{{other_struct.identifier}}_id(int64_t value);
{% endif %}
{% endfor %}
{% endfor %}

    // Fluent query builder - now returns builder without table preset
    SQLiteQueryBuilder Select{{struct.identifierCamel}}();

    virtual int64_t insertOrUpdate{{struct.identifierCamel}}(std::shared_ptr<{{struct.identifier}}Schema> obj, bool force_id=false);

    virtual bool delete{{struct.identifierCamel}}ById(int64_t id);
    
    virtual bool delete{{struct.identifierCamel}}ByIdCascade(int64_t id);

    virtual bool has{{struct.identifierCamel}}ById(int64_t id);

{% for mv in struct.member_variables %}
{% if not (mv.type.is_array or mv.type.is_struct or mv.type.is_enum) %}
{% if mv.identifier != "id" %}
    virtual bool has{{struct.identifierCamel}}By{{mv.identifierCamel}}({{mv.type.estimated}} value);
{% endif %}
{% endif %}
{% endfor %}

    // Primitive array helper methods
{% for mv in struct.member_variables %}
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
    virtual std::vector<{{mv.type.elem_type.estimated}}> select{{struct.identifierCamel}}{{mv.identifierCamel}}(int64_t parent_id);
    virtual bool delete{{struct.identifierCamel}}{{mv.identifierCamel}}(int64_t parent_id);
    virtual bool update{{struct.identifierCamel}}{{mv.identifierCamel}}(int64_t parent_id, const std::vector<{{mv.type.elem_type.estimated}}>& values);
{% endif %}
{% endfor %}

{#    // // Bulk insert/update
    // int64_t insertOrUpdateBulk{{struct}}(std::vector<std::shared_ptr<{{struct}}Schema>> objects);
    
    // // Bulk delete
    // bool deleteBulk{{struct}}ByIds(std::vector<int64_t> ids);

    //     // Count operations
    // int64_t count{{struct}}();
    // int64_t count{{struct}}By{{field}}({{type}} value);
    
    // // Exists operations
    // bool {{struct}}Exists(int64_t id);
    // bool {{struct}}ExistsBy{{field}}({{type}} value);
    
    // // Range queries for numeric/date fields
    // std::vector<std::shared_ptr<{{struct}}Schema>> select{{struct}}By{{field}}Range({{type}} min, {{type}} max);

    //     // Paginated results
    // std::vector<std::shared_ptr<{{struct}}Schema>> select{{struct}}Paginated(int64_t offset, int64_t limit);
    // std::vector<std::shared_ptr<{{struct}}Schema>> select{{struct}}OrderBy{{field}}(bool ascending = true, int64_t limit = 0);

    //     // Update specific fields without replacing entire record
    // bool update{{struct}}{{Field}}ById(int64_t id, {{type}} new_value);
    
    // // Conditional updates
    // int64_t update{{struct}}Where{{condition}}({{params}});

    //     // For array relationships (like Item -> ItemEffect)
    // bool add{{childStruct}}To{{parentStruct}}(int64_t parent_id, std::shared_ptr<{{child}}Schema> child);
    // bool remove{{childStruct}}From{{parentStruct}}(int64_t parent_id, int64_t child_id);
    // bool clear{{childStruct}}sFrom{{parentStruct}}(int64_t parent_id);

    //     // Text search (for string fields)
    // std::vector<std::shared_ptr<{{struct}}Schema>> search{{struct}}By{{field}}(std::string searchTerm, bool exactMatch = false);
    
    // // Multiple criteria search
    // std::vector<std::shared_ptr<{{struct}}Schema>> search{{struct}}(std::map<std::string, std::string> criteria);

    //     // Min/Max for numeric fields
    // {{type}} get{{struct}}Min{{field}}();
    // {{type}} get{{struct}}Max{{field}}();
    
    // // Sum/Average for numeric fields  
    // double get{{struct}}Sum{{field}}();
    // double get{{struct}}Average{{field}}();

    //     // Transaction management
    // bool beginTransaction();
    // bool commitTransaction();
    // bool rollbackTransaction();
    
    // // Batch operations within transactions
    // template<typename Func>
    // bool executeInTransaction(Func operation);
#}
{% endfor %}

    // Generic query builder factory methods
{% for struct in structs %}
    SQLiteQueryBuilder Query{{struct.identifierCamel}}();
{% endfor %}

    // Generic query builder for custom queries
    GenericSQLiteQueryBuilder Query();

    // SQLite callback hooks
    virtual void updateHook(int operation, const char* dbName, const char* tableName, sqlite3_int64 rowid);
    virtual int commitHook();
    virtual void rollbackHook();
    virtual void traceHook(unsigned int traceType, void* pCtx, void* p, void* x);
    virtual int progressHook();
    virtual int authorizerHook(int actionCode, const char* detail1, const char* detail2, 
                              const char* dbName, const char* triggerOrView);

    // Foreign key debugging helpers
    struct ForeignKeyViolation {
        std::string table;
        int64_t rowid;
        std::string parent;
        int fkid;
    };
    
    struct ForeignKeyInfo {
        int id;
        int seq;
        std::string table;
        std::string from;
        std::string to;
        std::string on_update;
        std::string on_delete;
    };
    
    virtual std::vector<ForeignKeyViolation> checkForeignKeyViolations();
    virtual std::vector<ForeignKeyInfo> getForeignKeyList(const std::string& table_name);
    virtual std::string formatForeignKeyError(const std::string& operation, const std::string& table, int64_t id);

private:
    std::filesystem::path db_path;
    sqlite3* db;

    static void updateCallback(void* userData, int operation, const char* dbName, 
                   const char* tableName, sqlite3_int64 rowid) {
        SQLiteDB* self = static_cast<SQLiteDB*>(userData);
        self->updateHook(operation, dbName, tableName, rowid);
    }
    
    static int commitCallback(void* userData) {
        SQLiteDB* self = static_cast<SQLiteDB*>(userData);
        return self->commitHook();
    }
    
    static void rollbackCallback(void* userData) {
        SQLiteDB* self = static_cast<SQLiteDB*>(userData);
        self->rollbackHook();
    }
    
    static int traceCallback(unsigned int traceType, void* userData, void* p, void* x) {
        SQLiteDB* self = static_cast<SQLiteDB*>(userData);
        self->traceHook(traceType, userData, p, x);
        return 0;
    }
    
    static int progressCallback(void* userData) {
        SQLiteDB* self = static_cast<SQLiteDB*>(userData);
        return self->progressHook();
    }
    
    static int authorizerCallback(void* userData, int actionCode, const char* detail1, 
                                 const char* detail2, const char* dbName, const char* triggerOrView) {
        SQLiteDB* self = static_cast<SQLiteDB*>(userData);
        return self->authorizerHook(actionCode, detail1, detail2, dbName, triggerOrView);
    }
};