#pragma once
#include <nlohmann/json.hpp>
#include <magic_enum/magic_enum.hpp>
#include <string>
#include <vector>
#include <type_traits>
#include <functional>
#include <utility>
#include <array>  // Add this include

namespace schema {

/**
 * @brief Options for a schema field
 */
struct FieldOptions {
    std::string description;
    bool optional = false;
    
    // Numeric constraints
    bool has_min = false;
    bool has_max = false;
    int min_value = 0;
    int max_value = 0;
    
    // String constraints
    bool has_min_length = false;
    bool has_max_length = false;
    int min_length = 0;
    int max_length = 0;
    
    // Array constraints
    bool has_min_items = false;
    bool has_max_items = false;
    int min_items = 0;
    int max_items = 0;

    // Enum constraints
    std::vector<std::string> enum_values;

    // Dynamic callback for runtime field options generation
    std::function<void(FieldOptions&)> options_callback = nullptr;
};

// Add callback function for dynamic field options
inline FieldOptions callback(std::function<void(FieldOptions&)> cb) {
    FieldOptions opts;
    opts.options_callback = cb;
    return opts;
}

// Option helpers
inline FieldOptions desc(const std::string& description) {
    FieldOptions opts;
    opts.description = description;
    return opts;
}

inline FieldOptions optional() {
    FieldOptions opts;
    opts.optional = true;
    return opts;
}

inline FieldOptions range(int min, int max) {
    FieldOptions opts;
    opts.has_min = true;
    opts.has_max = true;
    opts.min_value = min;
    opts.max_value = max;
    return opts;
}

// Add float overloads for range
inline FieldOptions range(float min, float max) {
    FieldOptions opts;
    opts.has_min = true;
    opts.has_max = true;
    opts.min_value = static_cast<int>(min);
    opts.max_value = static_cast<int>(max);
    return opts;
}

inline FieldOptions min(int value) {
    FieldOptions opts;
    opts.has_min = true;
    opts.min_value = value;
    return opts;
}

inline FieldOptions max(int value) {
    FieldOptions opts;
    opts.has_max = true;
    opts.max_value = value;
    return opts;
}

inline FieldOptions min_length(int value) {
    FieldOptions opts;
    opts.has_min_length = true;
    opts.min_length = value;
    return opts;
}

inline FieldOptions max_length(int value) {
    FieldOptions opts;
    opts.has_max_length = true;
    opts.max_length = value;
    return opts;
}

inline FieldOptions items(int min, int max) {
    FieldOptions opts;
    opts.has_min_items = true;
    opts.has_max_items = true;
    opts.min_items = min;
    opts.max_items = max;
    return opts;
}

// Add single-argument items function
inline FieldOptions items(int count) {
    FieldOptions opts;
    opts.has_min_items = true;
    opts.has_max_items = true;
    opts.min_items = count;
    opts.max_items = count;
    return opts;
}

// Add enum_values function
inline FieldOptions enum_values(const std::vector<std::string>& values) {
    FieldOptions opts;
    opts.enum_values = values;
    return opts;
}

// Add Enum function using magic_enum
template<typename EnumType>
inline FieldOptions Enum() {
    static_assert(std::is_enum_v<EnumType>, "Template parameter must be an enum type");
    
    FieldOptions opts;
    auto enum_names = magic_enum::enum_names<EnumType>();
    
    opts.enum_values.reserve(enum_names.size());
    for (const auto& name : enum_names) {
        opts.enum_values.emplace_back(std::string(name));
    }
    
    return opts;
}

inline FieldOptions operator|(const FieldOptions& a, const FieldOptions& b) {
    FieldOptions result = a;
    // Transfer the callback if it exists in b
    if (b.options_callback) {
        result.options_callback = b.options_callback;
    }
    if (!b.description.empty()) result.description = b.description;
    if (b.optional) result.optional = true;
    if (b.has_min) {
        result.has_min = true;
        result.min_value = b.min_value;
    }
    if (b.has_max) {
        result.has_max = true;
        result.max_value = b.max_value;
    }
    if (b.has_min_length) {
        result.has_min_length = true;
        result.min_length = b.min_length;
    }
    if (b.has_max_length) {
        result.has_max_length = true;
        result.max_length = b.max_length;
    }
    if (b.has_min_items) {
        result.has_min_items = true;
        result.min_items = b.min_items;
    }
    if (b.has_max_items) {
        result.has_max_items = true;
        result.max_items = b.max_items;
    }
    if (!b.enum_values.empty()) result.enum_values = b.enum_values;
    return result;
}

// Apply dynamic options from callback if available
inline FieldOptions apply_callback(const FieldOptions& options) {
    if (options.options_callback) {
        // Create a copy of the options to modify
        FieldOptions result = options;
        
        // Execute the callback to modify the options directly
        options.options_callback(result);
        
        // Return the modified options
        return result;
    }
    // If no callback is set, return the original options
    return options;
}

// Helper struct to hold field information
struct FieldInfo {
    std::string name;
    nlohmann::json schema;
    bool required;
    
    FieldInfo(const std::string& n, const nlohmann::json& s, bool r)
        : name(n), schema(s), required(r) {}
};

// Schema type detection and generation helpers

template<typename T>
nlohmann::json integer_schema(const FieldOptions& options = {}) {
    FieldOptions applied_options = apply_callback(options);
    nlohmann::json schema{{"type", "integer"}};
    if (applied_options.has_min) schema["minimum"] = applied_options.min_value;
    if (applied_options.has_max) schema["maximum"] = applied_options.max_value;
    if (!applied_options.description.empty()) schema["description"] = applied_options.description;
    return schema;
}


template<typename T>
nlohmann::json number_schema(const FieldOptions& options = {}) {
    FieldOptions applied_options = apply_callback(options);
    nlohmann::json schema{{"type", "number"}};
    if (applied_options.has_min) schema["minimum"] = applied_options.min_value;
    if (applied_options.has_max) schema["maximum"] = applied_options.max_value;
    if (!applied_options.description.empty()) schema["description"] = applied_options.description;
    return schema;
}


template<typename T>
nlohmann::json string_schema(const FieldOptions& options = {}) {
    FieldOptions applied_options = apply_callback(options);
    nlohmann::json schema{{"type", "string"}};
    if (applied_options.has_min_length) schema["minLength"] = applied_options.min_length;
    if (applied_options.has_max_length) schema["maxLength"] = applied_options.max_length;
    if (!applied_options.description.empty()) schema["description"] = applied_options.description;
    if (!applied_options.enum_values.empty()) schema["enum"] = applied_options.enum_values;
    return schema;
}


template<typename T>
nlohmann::json boolean_schema(const FieldOptions& options = {}) {
    FieldOptions applied_options = apply_callback(options);
    nlohmann::json schema{{"type", "boolean"}};
    if (!applied_options.description.empty()) schema["description"] = applied_options.description;
    return schema;
}

template<typename T, typename = void>
struct has_schema_method : std::false_type {};

template<typename T>
struct has_schema_method<T, 
    std::void_t<decltype(std::declval<T>().schema())>> 
    : std::true_type {};

template<typename T, typename = void>
struct has_static_schema_method : std::false_type {};

template<typename T>
struct has_static_schema_method<T, 
    std::void_t<decltype(T::schema())>> 
    : std::true_type {};

template<typename T, typename = void>
struct has_value_type : std::false_type {};

template<typename T>
struct has_value_type<T, 
    std::void_t<typename T::value_type>> 
    : std::true_type {};

// Add detection for std::array
template<typename T>
struct is_std_array : std::false_type {};

template<typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

// Get schema for any field type

template<typename T>
nlohmann::json get_field_schema(const FieldOptions& options = {}) {
    FieldOptions applied_options = apply_callback(options);
    // Handle enums automatically
    if constexpr (std::is_enum_v<T>) {
        nlohmann::json schema{{"type", "string"}};
        
        // Get enum values using magic_enum if not already provided in options
        if (applied_options.enum_values.empty()) {
            auto enum_names = magic_enum::enum_names<T>();
            std::vector<std::string> enum_vals;
            enum_vals.reserve(enum_names.size());
            for (const auto& name : enum_names) {
                enum_vals.emplace_back(std::string(name));
            }
            schema["enum"] = enum_vals;
        } else {
            schema["enum"] = applied_options.enum_values;
        }
        
        if (!applied_options.description.empty()) schema["description"] = applied_options.description;
        return schema;
    }
    // Handle integral types (but not bool)
    else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
        return integer_schema<T>(applied_options);
    } 
    // Handle floating point types
    else if constexpr (std::is_floating_point_v<T>) {
        return number_schema<T>(applied_options);
    } 
    // Handle strings
    else if constexpr (std::is_same_v<T, std::string>) {
        return string_schema<T>(applied_options);
    } 
    // Handle booleans
    else if constexpr (std::is_same_v<T, bool>) {
        return boolean_schema<T>(applied_options);
    } 
    // Handle JSON
    else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, nlohmann::json>) {
        nlohmann::json schema{{"type", "object"}};
        if (!applied_options.description.empty()) schema["description"] = applied_options.description;
        return schema;
    } 
    // Handle classes with schema methods
    else if constexpr (has_schema_method<T>::value) {
        auto schema = T().schema();
        if (!applied_options.description.empty() && !schema.contains("description")) {
            schema["description"] = applied_options.description;
        }
        return schema;
    }
    // Handle classes with static schema methods
    else if constexpr (has_static_schema_method<T>::value) {
        auto schema = T::schema();
        if (!applied_options.description.empty() && !schema.contains("description")) {
            schema["description"] = applied_options.description;
        }
        return schema;
    }
    // Handle std::array specifically
    else if constexpr (is_std_array<T>::value) {
        nlohmann::json schema{{"type", "array"}};
        using ValueType = typename T::value_type;
        schema["items"] = get_field_schema<ValueType>();
        // std::array has a fixed size
        std::size_t arraySize = std::tuple_size<T>::value;
        schema["minItems"] = arraySize;
        schema["maxItems"] = arraySize;
        // Apply any other array options
        if (applied_options.has_min_items) schema["minItems"] = applied_options.min_items;
        if (applied_options.has_max_items) schema["maxItems"] = applied_options.max_items;
        if (!applied_options.description.empty()) schema["description"] = applied_options.description;
        return schema;
    }
    // Handle containers (like vector)
    else if constexpr (has_value_type<T>::value) {
        nlohmann::json schema{{"type", "array"}};
        using ValueType = typename T::value_type;
        schema["items"] = get_field_schema<ValueType>();
        if (applied_options.has_min_items) schema["minItems"] = applied_options.min_items;
        if (applied_options.has_max_items) schema["maxItems"] = applied_options.max_items;
        if (!applied_options.description.empty()) schema["description"] = applied_options.description;
        if (!applied_options.enum_values.empty()) schema["items"]["enum"] = applied_options.enum_values;
        return schema;
    }
    // Handle any other type as generic object
    else {
        nlohmann::json schema{{"type", "object"}};
        if (!applied_options.description.empty()) schema["description"] = applied_options.description;
        return schema;
    }
}

// Field declaration helper that works with primitive and reference types
#define SCHEMA_FIELD_HELPER(Type, Field, Options) \
    schema::FieldInfo(#Field, schema::get_field_schema<decltype(Type::Field)>(Options), !(Options).optional)

// Record a field with options
#define SCHEMA_FIELD(Field, ...) \
    SCHEMA_FIELD_HELPER(T, Field, __VA_ARGS__)

// Simplified schema definition macro that works in static context
#define SCHEMA_DEFINE(Type, ...) \
    static nlohmann::json schema() { \
        using T = Type; \
        nlohmann::json schema = { \
            {"type", "object"}, \
            {"properties", nlohmann::json::object()}, \
            {"required", nlohmann::json::array()} \
        }; \
        \
        std::vector<schema::FieldInfo> fields = {__VA_ARGS__}; \
        \
        for (const auto& field : fields) { \
            schema["properties"][field.name] = field.schema; \
            if (field.required) { \
                schema["required"].push_back(field.name); \
            } \
        } \
        \
        return schema; \
    }

} // namespace schema