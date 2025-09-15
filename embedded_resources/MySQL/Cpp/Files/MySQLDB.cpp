#pragma once
#include {{format_include("MySQLDB.hpp")}}
{% for include in includes %}
#include {{include}}
{% endfor %}


MySQLDB::~MySQLDB() {}

MySQLDB::MySQLDB(const std::string& uri, const std::string& db_name){
    this->uri = uri;
    this->database_name = db_name;
    this->session = nullptr;
};

void MySQLDB::connect(){
    if (isConnected()) {
        return; // Already connected
    }
    try {
        session = std::make_unique<mysqlx::Session>(uri);
        database_name = database_name;
    } catch (const mysqlx::Error &err) {
        throw std::runtime_error("Connection error: " + std::string(err.what()));
    } catch (std::exception &ex) {
        throw std::runtime_error("STD Exception: " + std::string(ex.what()));
    } catch (...) {
        throw std::runtime_error("Unknown exception during connection");
    }
};

void MySQLDB::disconnect(){
    if (session) {
        session->close();
        session.reset();
    }
};

bool MySQLDB::isConnected() const{
    return session != nullptr;
};

mysqlx::Session& MySQLDB::getSession(){
    if (!session) {
        throw std::runtime_error("Database session is not connected.");
    }
    return *session;
};

void MySQLDB::setDatabaseName(const std::string& db_name) { database_name = db_name; }

std::string MySQLDB::getDatabaseName() const { return database_name; }

{% for struct in structs %}
std::shared_ptr<{{struct.identifier}}Schema> MySQLDB::select{{struct.identifierCamel}}ById(int64_t id){
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    
    // Check if object is already cached
    auto cacheIt = {{struct.identifier}}Cache.find(id);
    if (cacheIt != {{struct.identifier}}Cache.end()) {
        // Object found in cache, update it and return
        auto cachedObj = cacheIt->second;
        cachedObj->MySQLSelect(this);
        return cachedObj;
    }
    
    // Object not in cache, fetch from database
    mysqlx::Schema db = getSession().getSchema(database_name);
    mysqlx::Table table = db.getTable("{{struct.identifier}}");
    mysqlx::RowResult res = table.select("*").where("id = :id").bind("id", id).execute();
    mysqlx::Row row = res.fetchOne();
    
    if (row) {
        auto obj = std::make_shared<{{struct.identifier}}Schema>();
        
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
int64_t MySQLDB::insertOrUpdate{{struct.identifierCamel}}(std::shared_ptr<{{struct.identifier}}Schema> obj) {
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    
    mysqlx::Schema db = getSession().getSchema(database_name);
    mysqlx::Table table = db.getTable("{{struct.identifier}}");
    
    // Check if the object already exists
    if (obj->getId() > 0) {
        // Update existing object
        mysqlx::abi2::r0::TableUpdate update=table.update();
{% for mv in struct.member_variables %}
{% if not mv.type.is_array and not mv.type.is_struct and not mv.type.is_enum %}
{% if mv.required %}
            update.set("{{mv.identifier}}", obj->get{{mv.identifierCamel}}());
{% else %}
            update.set("{{mv.identifier}}", obj->get{{mv.identifierCamel}}().value_or({{format_default(mv.type,"")}}));
{% endif %}
{% endif %}
{% endfor %}
        update.where("id = :id");
        update.bind("id", obj->getId());
        update.execute();
        
        // Check if object is already cached
        auto cacheIt = {{struct.identifier}}Cache.find(obj->getId());
        if (cacheIt == {{struct.identifier}}Cache.end()) {
            // Not in cache, add it
            {{struct.identifier}}Cache[obj->getId()] = obj;
        } else {
            // Object is cached, update the cached object to match the database
            auto cachedObj = cacheIt->second;
{% for mv in struct.member_variables %}
{% if not mv.type.is_array and not mv.type.is_struct and not mv.type.is_enum %}
{% if mv.required %}
            cachedObj->set{{mv.identifierCamel}}(obj->get{{mv.identifierCamel}}());
{% else %}
            cachedObj->set{{mv.identifierCamel}}(obj->get{{mv.identifierCamel}}());
{% endif %}
{% endif %}
{% endfor %}
        }
        
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

bool MySQLDB::delete{{struct.identifierCamel}}ById(int64_t id) {
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


{# {% for enum in enums %}
std::shared_ptr<{{enum.identifier}}Schema> MySQLDB::select{{enum.identifierCamel}}ById(int64_t id){
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    
    // Check if object is already cached
    auto cacheIt = {{enum.identifier}}Cache.find(id);
    if (cacheIt != {{enum.identifier}}Cache.end()) {
        // Object found in cache, update it and return
        auto cachedObj = cacheIt->second;
        cachedObj->MySQLSelect(shared_from_this());
        return cachedObj;
    }
    
    // Object not in cache, fetch from database
    mysqlx::Schema db = getSession().getSchema(database_name);
    mysqlx::Table table = db.getTable("{{enum.identifier}}");
    mysqlx::RowResult res = table.select("*").where("id = :id").bind("id", id).execute();
    mysqlx::Row row = res.fetchOne();
    
    if (row) {
        auto obj = std::make_shared<{{enum.identifier}}Schema>();
        // Set the MySQL session for the new object
        if (session == nullptr) {
            {{enum.identifier}}Schema::SetMySQLSession(std::shared_ptr<mysqlx::Session>(session.get(), [](std::shared_ptr<mysqlx::Session>){}));
        }
        
        // Populate object from database row
{% set column_index = 0 %}
{% for mv in enum.member_variables %}
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
        {{enum.identifier}}Cache[id] = obj;
        
        return obj;
    }
    
    // Object not found in database
    return nullptr;
}

//updateInsert
int64_t MySQLDB::insertOrUpdate{{enum.identifierCamel}}(const std::shared_ptr<{{enum.identifier}}Schema>& obj) {
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    
    mysqlx::Schema db = getSession().getSchema(database_name);
    mysqlx::Table table = db.getTable("{{enum.identifier}}");
    
    // Check if the object already exists
    if (obj->getId() > 0) {
        // Update existing object
        mysqlx::abi2::r0::TableUpdate update=table.update();
{% for mv in enum.member_variables %}
{% if not mv.type.is_array and not mv.type.is_struct and not mv.type.is_enum %}
{% if mv.required %}
            update.set("{{mv.identifier}}", obj->get{{mv.identifierCamel}}());
{% else %}
            update.set("{{mv.identifier}}", obj->get{{mv.identifierCamel}}().value_or({{format_default(mv.type,"")}}));
{% endif %}
{% endif %}
{% endfor %}
        update.where("id = :id");
        update.bind("id", obj->getId());
        update.execute();
        return obj->getId();
    } else {
        // Insert new object
        mysqlx::Result res = table.insert("{{enum.identifier}}").values(obj->toMySQLRow()).execute();
        int64_t newId = res.getAutoIncrementValue();
        obj->setId(newId);
        {{enum.identifier}}Cache[newId] = obj;
        return newId;
    }
}

bool MySQLDB::delete{{enum.identifierCamel}}ById(int64_t id) {
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    
    mysqlx::Schema db = getSession().getSchema(database_name);
    mysqlx::Table table = db.getTable("{{enum.identifier}}");
    
    // Delete from database
    mysqlx::Result res = table.remove().where("id = :id").bind("id", id).execute();
    
    // Remove from cache if it exists
    auto cacheIt = {{enum.identifier}}Cache.find(id);
    if (cacheIt != {{enum.identifier}}Cache.end()) {
        {{enum.identifier}}Cache.erase(cacheIt);
    }
    
    // Return true if a row was actually deleted
    return res.getAffectedItemsCount() > 0;
}
{% endfor %} #}
