#pragma once
#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>
#include <sstream>
{% for struct in structs %}
class {{struct.identifier}}Schema;
{% endfor %}

class SQLiteDB;

template<typename T>
class SQLiteQueryBuilder {
private:
    SQLiteDB* db;
    std::string table_name;
    std::vector<std::string> where_clauses;
    std::vector<std::string> order_clauses;
    std::vector<std::pair<int, std::string>> string_bindings;
    std::vector<std::pair<int, int64_t>> int_bindings;
    std::vector<std::pair<int, double>> double_bindings;
    int limit_value = -1;
    int offset_value = -1;
    int bind_index = 1;

public:
    SQLiteQueryBuilder(SQLiteDB* database) 
        : db(database) {}

    SQLiteQueryBuilder<T>& From(const std::string& table) {
        table_name = table;
        return *this;
    }

    SQLiteQueryBuilder<T>& Where(const std::string& condition) {
        where_clauses.push_back(condition);
        return *this;
    }

    SQLiteQueryBuilder<T>& WhereEquals(const std::string& field, const std::string& value) {
        where_clauses.push_back(field + " = ?");
        string_bindings.push_back({bind_index++, value});
        return *this;
    }

    SQLiteQueryBuilder<T>& WhereEquals(const std::string& field, int64_t value) {
        where_clauses.push_back(field + " = ?");
        int_bindings.push_back({bind_index++, value});
        return *this;
    }

    SQLiteQueryBuilder<T>& WhereEquals(const std::string& field, double value) {
        where_clauses.push_back(field + " = ?");
        double_bindings.push_back({bind_index++, value});
        return *this;
    }

    SQLiteQueryBuilder<T>& WhereGreaterThan(const std::string& field, int64_t value) {
        where_clauses.push_back(field + " > ?");
        int_bindings.push_back({bind_index++, value});
        return *this;
    }

    SQLiteQueryBuilder<T>& WhereLessThan(const std::string& field, int64_t value) {
        where_clauses.push_back(field + " < ?");
        int_bindings.push_back({bind_index++, value});
        return *this;
    }

    SQLiteQueryBuilder<T>& WhereLike(const std::string& field, const std::string& pattern) {
        where_clauses.push_back(field + " LIKE ?");
        string_bindings.push_back({bind_index++, pattern});
        return *this;
    }

    SQLiteQueryBuilder<T>& WhereIn(const std::string& field, const std::vector<int64_t>& values) {
        if (!values.empty()) {
            std::string placeholders = "?";
            for (size_t i = 1; i < values.size(); ++i) {
                placeholders += ", ?";
            }
            where_clauses.push_back(field + " IN (" + placeholders + ")");
            for (int64_t value : values) {
                int_bindings.push_back({bind_index++, value});
            }
        }
        return *this;
    }

    SQLiteQueryBuilder<T>& OrderBy(const std::string& field, bool ascending = true) {
        order_clauses.push_back(field + (ascending ? " ASC" : " DESC"));
        return *this;
    }

    SQLiteQueryBuilder<T>& Limit(int limit) {
        limit_value = limit;
        return *this;
    }

    SQLiteQueryBuilder<T>& Offset(int offset) {
        offset_value = offset;
        return *this;
    }

    std::vector<std::shared_ptr<T>> Exec();
    std::shared_ptr<T> First();
    bool Exists();
    int Count();

private:
    std::string BuildQuery(const std::string& select_clause = "*") {
        if (table_name.empty()) {
            throw std::runtime_error("SQLiteQueryBuilder: No table specified. Call From() method first.");
        }
        
        std::stringstream sql;
        sql << "SELECT " << select_clause << " FROM " << table_name;
        
        if (!where_clauses.empty()) {
            sql << " WHERE ";
            for (size_t i = 0; i < where_clauses.size(); ++i) {
                if (i > 0) sql << " AND ";
                sql << where_clauses[i];
            }
        }
        
        if (!order_clauses.empty()) {
            sql << " ORDER BY ";
            for (size_t i = 0; i < order_clauses.size(); ++i) {
                if (i > 0) sql << ", ";
                sql << order_clauses[i];
            }
        }
        
        if (limit_value > 0) {
            sql << " LIMIT " << limit_value;
        }
        
        if (offset_value > 0) {
            sql << " OFFSET " << offset_value;
        }
        
        return sql.str();
    }

    void BindParameters(sqlite3_stmt* stmt) {
        for (const auto& binding : string_bindings) {
            sqlite3_bind_text(stmt, binding.first, binding.second.c_str(), -1, SQLITE_STATIC);
        }
        for (const auto& binding : int_bindings) {
            sqlite3_bind_int64(stmt, binding.first, binding.second);
        }
        for (const auto& binding : double_bindings) {
            sqlite3_bind_double(stmt, binding.first, binding.second);
        }
    }
};

// Specializations for each struct type
{% for struct in structs %}
template<>
std::vector<std::shared_ptr<{{struct.identifier}}Schema>> SQLiteQueryBuilder<{{struct.identifier}}Schema>::Exec();

template<>
std::shared_ptr<{{struct.identifier}}Schema> SQLiteQueryBuilder<{{struct.identifier}}Schema>::First();

template<>
bool SQLiteQueryBuilder<{{struct.identifier}}Schema>::Exists();

template<>
int SQLiteQueryBuilder<{{struct.identifier}}Schema>::Count();
{% endfor %}