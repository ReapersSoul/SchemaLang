#pragma once
#include <vector>
#include <optional>
#include <string>
#include <set>
#include <map>
#include <stdexcept>
#include <mysqlx/xdevapi.h>
{% for include in includes %}
#include {{include}}
{% endfor %}


class MySQLDB
{
public:
    virtual ~MySQLDB() {}
    MySQLDB(const std::string& uri, const std::string& db_name = "schema_db");
    void connect();
    void disconnect();
    bool isConnected() const;
    mysqlx::Session& getSession();
    void setDatabaseName(const std::string& db_name) { database_name = db_name; }
    std::string getDatabaseName() const { return database_name; }

{% for struct in structs %}
    virtual std::shared_ptr<{{struct.identifier}}Schema> select{{struct.identifierCamel}}ById(int64_t id){
        if (!isConnected()) {
            throw std::runtime_error("Database not connected");
        }
        
        // Check if object is already cached
        auto cacheIt = {{struct.identifier}}Cache.find(id);
        if (cacheIt != {{struct.identifier}}Cache.end()) {
            // Object found in cache, update it and return
            auto cachedObj = cacheIt->second;
            // Set the MySQL session for the cached object if not already set
            if ({{struct.identifier}}Schema::GetMySQLSession() == nullptr) {
                {{struct.identifier}}Schema::SetMySQLSession(std::shared_ptr<mysqlx::Session>(session.get(), [](std::shared_ptr<mysqlx::Session>){}));
            }
            cachedObj->MySQLUpdate();
            return cachedObj;
        }
        
        // Object not in cache, fetch from database
        mysqlx::Schema db = getSession().getSchema(database_name);
        mysqlx::Table table = db.getTable("{{struct.identifier}}");
        mysqlx::RowResult res = table.select("*").where("id = :id").bind("id", id).execute();
        mysqlx::Row row = res.fetchOne();
        
        if (row) {
            auto obj = std::make_shared<{{struct.identifier}}Schema>();
            // Set the MySQL session for the new object
            if ({{struct.identifier}}Schema::GetMySQLSession() == nullptr) {
                {{struct.identifier}}Schema::SetMySQLSession(std::shared_ptr<mysqlx::Session>(session.get(), [](std::shared_ptr<mysqlx::Session>){}));
            }
            
            // Populate object from database row
{% set column_index = 0 %}
{% for mv in struct.member_variables %}
{% if not (mv.type.is_array or mv.type.is_struct or mv.type.is_enum) %}
            // Set {{mv.identifier}}
{% if mv.type.is_integer %}
            obj->set{{mv.identifierCamel}}(row[{{column_index}}].get<int>());
{% endif %}
{% if mv.type.is_real %}
            obj->set{{mv.identifierCamel}}(row[{{column_index}}].get<double>());
{% endif %}
{% if mv.type.is_string %}
            obj->set{{mv.identifierCamel}}(row[{{column_index}}].get<std::string>());
{% endif %}
{% if mv.type.is_bool %}
            obj->set{{mv.identifierCamel}}(row[{{column_index}}].get<bool>());
{% endif %}
{% if not (mv.type.is_integer or mv.type.is_real or mv.type.is_string or mv.type.is_bool) %}
            obj->set{{mv.identifierCamel}}(row[{{column_index}}].get<{{mv.type.estimated}}>());
{% endif %}
{% set column_index = column_index + 1 %}
{% endif %}
{% endfor %}
            
            // Cache the object
            {{struct.identifier}}Cache[id] = obj;
            
            return obj;
        }
        
        // Object not found in database
        return nullptr;
    }

//updateInsert
int64_t insertOrUpdate{{struct.identifierCamel}}(const std::shared_ptr<{{struct.identifier}}Schema>& obj) {
        if (!isConnected()) {
            throw std::runtime_error("Database not connected");
        }
        
        mysqlx::Schema db = getSession().getSchema(database_name);
        mysqlx::Table table = db.getTable("{{struct.identifier}}");
        
        // Check if the object already exists
        if (obj->getId() > 0) {
            // Update existing object
            table.update()
                .set("{{struct.identifier}}", obj->toMySQLRow())
                .where("id = :id")
                .bind("id", obj->getId())
                .execute();
            return obj->getId();
        } else {
            // Insert new object
            mysqlx::Result res = table.insert("{{struct.identifier}}").values(obj->toMySQLRow()).execute();
            int64_t newId = res.getAutoIncrementValue();
            obj->setId(newId);
            {{struct.identifier}}Cache[newId] = obj;
            return newId;
        }
    }

    virtual bool delete{{struct.identifierCamel}}ById(int64_t id) {
        if (!isConnected()) {
            throw std::runtime_error("Database not connected");
        }
        
        mysqlx::Schema db = getSession().getSchema(database_name);
        mysqlx::Table table = db.getTable("{{struct.identifier}}");
        
        // Delete from database
        mysqlx::Result res = table.remove().where("id = :id").bind("id", id).execute();
        
        // Remove from cache if it exists
        auto cacheIt = {{struct.identifier}}Cache.find(id);
        if (cacheIt != {{struct.identifier}}Cache.end()) {
            {{struct.identifier}}Cache.erase(cacheIt);
        }
        
        // Return true if a row was actually deleted
        return res.getAffectedItemsCount() > 0;
    }

{% endfor %}

private:
    std::string uri;
    std::string database_name;
    std::unique_ptr<mysqlx::Session> session;
    {% for struct in structs %}
    std::map<int64_t, std::shared_ptr<{{struct.identifier}}Schema>> {{struct.identifier}}Cache;
    {% endfor %}
};