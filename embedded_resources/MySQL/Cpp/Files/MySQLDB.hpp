#pragma once
#include <vector>
#include <optional>
#include <string>
#include <set>
#include <map>
#include <stdexcept>
#include <mysqlx/xdevapi.h>
{% for struct in structs %}
class {{struct.identifier}}Schema;
{% endfor %}
{# {% for enum in enums %}
enum class {{enum.identifier}}Schema;
{% endfor %} #}


class MySQLDB
{
public:
    virtual ~MySQLDB();
    MySQLDB(const std::string& uri, const std::string& db_name = "schema_db");
    void connect();
    void disconnect();
    bool isConnected() const;
    mysqlx::Session& getSession();
    void setDatabaseName(const std::string& db_name);
    std::string getDatabaseName() const;

{% for struct in structs %}
    virtual std::shared_ptr<{{struct.identifier}}Schema> select{{struct.identifierCamel}}ById(int64_t id);

    virtual int64_t insertOrUpdate{{struct.identifierCamel}}(std::shared_ptr<{{struct.identifier}}Schema> obj);

    virtual bool delete{{struct.identifierCamel}}ById(int64_t id);

{% endfor %}

{# {% for enum in enums %}
    virtual std::shared_ptr<{{enum.identifier}}Schema> select{{enum.identifierCamel}}ById(int64_t id);

    virtual int64_t insertOrUpdate{{enum.identifierCamel}}({{enum.identifier}}Schema* obj);

    virtual bool delete{{enum.identifierCamel}}ById(int64_t id);

{% endfor %} #}
private:
    std::string uri;
    std::string database_name;
    std::unique_ptr<mysqlx::Session> session;
};