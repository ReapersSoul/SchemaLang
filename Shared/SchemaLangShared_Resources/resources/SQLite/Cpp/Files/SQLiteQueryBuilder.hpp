#pragma once
#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>
#include <sstream>
#include <variant>
#include <stdexcept>
{% for struct in structs %}
class {{struct.identifier}}Schema;
{% endfor %}

class SQLiteDB;

// Forward declaration for generic query builder
class GenericSQLiteQueryBuilder;

// Flexible result class for custom column selections
class SqliteResult {
private:
    std::vector<std::variant<std::string, int64_t, double, bool, std::nullptr_t>> values;
    std::vector<std::string> column_names;

public:
    SqliteResult() = default;
    
    void addColumn(const std::string& name) {
        column_names.push_back(name);
    }
    
    void addValue(const std::string& value) {
        values.push_back(value);
    }
    
    void addValue(int64_t value) {
        values.push_back(value);
    }
    
    void addValue(double value) {
        values.push_back(value);
    }
    
    void addValue(bool value) {
        values.push_back(value);
    }
    
    void addNull() {
        values.push_back(nullptr);
    }
    
    template<typename T>
    T get(int column_index) const {
        if (column_index < 0 || column_index >= static_cast<int>(values.size())) {
            throw std::out_of_range("Column index out of range");
        }
        
        const auto& value = values[column_index];
        
        if (std::holds_alternative<std::nullptr_t>(value)) {
            throw std::runtime_error("Cannot convert NULL value to requested type");
        }
        
        if constexpr (std::is_same_v<T, std::string>) {
            if (std::holds_alternative<std::string>(value)) {
                return std::get<std::string>(value);
            }
            // Convert other types to string
            if (std::holds_alternative<int64_t>(value)) {
                return std::to_string(std::get<int64_t>(value));
            }
            if (std::holds_alternative<double>(value)) {
                return std::to_string(std::get<double>(value));
            }
            if (std::holds_alternative<bool>(value)) {
                return std::get<bool>(value) ? "true" : "false";
            }
        } else if constexpr (std::is_same_v<T, int64_t>) {
            if (std::holds_alternative<int64_t>(value)) {
                return std::get<int64_t>(value);
            }
            if (std::holds_alternative<double>(value)) {
                return static_cast<int64_t>(std::get<double>(value));
            }
            if (std::holds_alternative<bool>(value)) {
                return std::get<bool>(value) ? 1 : 0;
            }
        } else if constexpr (std::is_same_v<T, int>) {
            if (std::holds_alternative<int64_t>(value)) {
                return static_cast<int>(std::get<int64_t>(value));
            }
            if (std::holds_alternative<double>(value)) {
                return static_cast<int>(std::get<double>(value));
            }
            if (std::holds_alternative<bool>(value)) {
                return std::get<bool>(value) ? 1 : 0;
            }
        } else if constexpr (std::is_same_v<T, double>) {
            if (std::holds_alternative<double>(value)) {
                return std::get<double>(value);
            }
            if (std::holds_alternative<int64_t>(value)) {
                return static_cast<double>(std::get<int64_t>(value));
            }
        } else if constexpr (std::is_same_v<T, bool>) {
            if (std::holds_alternative<bool>(value)) {
                return std::get<bool>(value);
            }
            if (std::holds_alternative<int64_t>(value)) {
                return std::get<int64_t>(value) != 0;
            }
        }
        
        throw std::runtime_error("Cannot convert column value to requested type");
    }
    
    size_t columnCount() const {
        return values.size();
    }
    
    const std::string& getColumnName(int index) const {
        if (index < 0 || index >= static_cast<int>(column_names.size())) {
            throw std::out_of_range("Column index out of range");
        }
        return column_names[index];
    }
    
    bool isNull(int column_index) const {
        if (column_index < 0 || column_index >= static_cast<int>(values.size())) {
            throw std::out_of_range("Column index out of range");
        }
        return std::holds_alternative<std::nullptr_t>(values[column_index]);
    }
};

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
    std::vector<std::string> select_columns;
    bool custom_select = false;

public:
    SQLiteQueryBuilder(SQLiteDB* database) 
        : db(database) {}

    SQLiteQueryBuilder& Select(const std::string& columns) {
        custom_select = true;
        // Split columns by comma and trim whitespace
        std::istringstream ss(columns);
        std::string column;
        select_columns.clear();
        while (std::getline(ss, column, ',')) {
            // Trim whitespace
            size_t start = column.find_first_not_of(" \t");
            size_t end = column.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos) {
                select_columns.push_back(column.substr(start, end - start + 1));
            }
        }
        return *this;
    }

    SQLiteQueryBuilder& Select(const std::vector<std::string>& columns) {
        custom_select = true;
        select_columns = columns;
        return *this;
    }

    SQLiteQueryBuilder& From(const std::string& table) {
        table_name = table;
        return *this;
    }

    SQLiteQueryBuilder& Where(const std::string& condition) {
        where_clauses.push_back(condition);
        return *this;
    }

    SQLiteQueryBuilder& WhereEquals(const std::string& field, const std::string& value) {
        where_clauses.push_back(field + " = ?");
        string_bindings.push_back({bind_index++, value});
        return *this;
    }

    SQLiteQueryBuilder& WhereEquals(const std::string& field, int64_t value) {
        where_clauses.push_back(field + " = ?");
        int_bindings.push_back({bind_index++, value});
        return *this;
    }

    SQLiteQueryBuilder& WhereEquals(const std::string& field, double value) {
        where_clauses.push_back(field + " = ?");
        double_bindings.push_back({bind_index++, value});
        return *this;
    }

    SQLiteQueryBuilder& WhereGreaterThan(const std::string& field, int64_t value) {
        where_clauses.push_back(field + " > ?");
        int_bindings.push_back({bind_index++, value});
        return *this;
    }

    SQLiteQueryBuilder& WhereLessThan(const std::string& field, int64_t value) {
        where_clauses.push_back(field + " < ?");
        int_bindings.push_back({bind_index++, value});
        return *this;
    }

    SQLiteQueryBuilder& WhereLike(const std::string& field, const std::string& pattern) {
        where_clauses.push_back(field + " LIKE ?");
        string_bindings.push_back({bind_index++, pattern});
        return *this;
    }

    SQLiteQueryBuilder& WhereIn(const std::string& field, const std::vector<int64_t>& values) {
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

    SQLiteQueryBuilder& OrderBy(const std::string& field, bool ascending = true) {
        order_clauses.push_back(field + (ascending ? " ASC" : " DESC"));
        return *this;
    }

    SQLiteQueryBuilder& Limit(int limit) {
        limit_value = limit;
        return *this;
    }

    SQLiteQueryBuilder& Offset(int offset) {
        offset_value = offset;
        return *this;
    }

    template<typename T>
    std::vector<std::shared_ptr<T>> Exec();
    template<typename T>
    std::shared_ptr<T> First();
    bool Exists();
    int Count();
    
    // Custom select methods that return SqliteResult
    std::vector<SqliteResult> ExecCustom();
    SqliteResult FirstCustom();
    
    // Public method to inspect generated SQL query
    std::string BuildQuery() {
        return BuildQuery("*");
    }

private:
    std::string BuildQuery(const std::string& select_clause) {
        if (table_name.empty()) {
            throw std::runtime_error("SQLiteQueryBuilder: No table specified. Call From() method first.");
        }
        
        std::stringstream sql;
        std::string actual_select = select_clause;
        
        // If custom select is enabled and no explicit select_clause provided, use custom columns
        if (custom_select && select_clause == "*" && !select_columns.empty()) {
            actual_select = "";
            for (size_t i = 0; i < select_columns.size(); ++i) {
                if (i > 0) actual_select += ", ";
                actual_select += select_columns[i];
            }
        }
        
        sql << "SELECT " << actual_select << " FROM " << table_name;
        
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
std::vector<std::shared_ptr<{{struct.identifier}}Schema>> SQLiteQueryBuilder::Exec<{{struct.identifier}}Schema>();

template<>
std::shared_ptr<{{struct.identifier}}Schema> SQLiteQueryBuilder::First<{{struct.identifier}}Schema>();
{% endfor %}

// Generic query builder for custom queries without schema types
class GenericSQLiteQueryBuilder {
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
    std::vector<std::string> select_columns;

public:
    GenericSQLiteQueryBuilder(SQLiteDB* database) 
        : db(database) {}

    GenericSQLiteQueryBuilder& Select(const std::string& columns) {
        std::istringstream ss(columns);
        std::string column;
        select_columns.clear();
        while (std::getline(ss, column, ',')) {
            size_t start = column.find_first_not_of(" \t");
            size_t end = column.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos) {
                select_columns.push_back(column.substr(start, end - start + 1));
            }
        }
        return *this;
    }

    GenericSQLiteQueryBuilder& Select(const std::vector<std::string>& columns) {
        select_columns = columns;
        return *this;
    }

    GenericSQLiteQueryBuilder& From(const std::string& table) {
        table_name = table;
        return *this;
    }

    GenericSQLiteQueryBuilder& Where(const std::string& condition) {
        where_clauses.push_back(condition);
        return *this;
    }

    GenericSQLiteQueryBuilder& Where(const std::string& field, const std::string& value) {
        where_clauses.push_back(field + " = ?");
        string_bindings.push_back({bind_index++, value});
        return *this;
    }

    GenericSQLiteQueryBuilder& Where(const std::string& field, int64_t value) {
        where_clauses.push_back(field + " = ?");
        int_bindings.push_back({bind_index++, value});
        return *this;
    }

    GenericSQLiteQueryBuilder& Where(const std::string& field, double value) {
        where_clauses.push_back(field + " = ?");
        double_bindings.push_back({bind_index++, value});
        return *this;
    }

    GenericSQLiteQueryBuilder& WhereEquals(const std::string& field, const std::string& value) {
        return Where(field, value);
    }

    GenericSQLiteQueryBuilder& WhereEquals(const std::string& field, int64_t value) {
        return Where(field, value);
    }

    GenericSQLiteQueryBuilder& WhereEquals(const std::string& field, double value) {
        return Where(field, value);
    }

    GenericSQLiteQueryBuilder& WhereGreaterThan(const std::string& field, int64_t value) {
        where_clauses.push_back(field + " > ?");
        int_bindings.push_back({bind_index++, value});
        return *this;
    }

    GenericSQLiteQueryBuilder& WhereLessThan(const std::string& field, int64_t value) {
        where_clauses.push_back(field + " < ?");
        int_bindings.push_back({bind_index++, value});
        return *this;
    }

    GenericSQLiteQueryBuilder& WhereLike(const std::string& field, const std::string& pattern) {
        where_clauses.push_back(field + " LIKE ?");
        string_bindings.push_back({bind_index++, pattern});
        return *this;
    }

    GenericSQLiteQueryBuilder& WhereIn(const std::string& field, const std::vector<int64_t>& values) {
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

    GenericSQLiteQueryBuilder& OrderBy(const std::string& field, bool ascending = true) {
        order_clauses.push_back(field + (ascending ? " ASC" : " DESC"));
        return *this;
    }

    GenericSQLiteQueryBuilder& Limit(int limit) {
        limit_value = limit;
        return *this;
    }

    GenericSQLiteQueryBuilder& Offset(int offset) {
        offset_value = offset;
        return *this;
    }

    std::vector<SqliteResult> Exec();
    SqliteResult First();
    bool Exists();
    int Count();
    
    // Public method to inspect generated SQL query
    std::string BuildQuery() {
        return BuildQuery("");
    }

private:
    std::string BuildQuery(const std::string& select_clause) {
        if (table_name.empty()) {
            throw std::runtime_error("GenericSQLiteQueryBuilder: No table specified. Call From() method first.");
        }
        
        std::stringstream sql;
        std::string actual_select = select_clause;
        
        if (actual_select.empty()) {
            if (!select_columns.empty()) {
                for (size_t i = 0; i < select_columns.size(); ++i) {
                    if (i > 0) actual_select += ", ";
                    actual_select += select_columns[i];
                }
            } else {
                actual_select = "*";
            }
        }
        
        sql << "SELECT " << actual_select << " FROM " << table_name;
        
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