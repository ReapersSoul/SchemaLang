# SchemaLang Syntax Guide

## Overview
SchemaLang is a schema definition language that allows you to define structured data types with detailed metadata including constraints, relationships, and documentation. It supports generating code for multiple targets including C++, JSON schemas, MySQL, and SQLite.

## CLI Usage

### Basic Command Structure
```bash
SchemaLangTranspiler -schemaDirectory=<path> -outputDirectory=<path> [flags]
```

### Required Parameters
- `-schemaDirectory=<path>` - Path to directory containing `.schema` or `.schemaLang` files
- `-outputDirectory=<path>` - Path where generated files will be created

### Generator Flags
- `-cpp` - Generate C++ classes with getters/setters
- `-java` - Generate Java classes (currently in development)
- `-json` - Generate JSON schema files
- `-sqlite` - Generate SQLite database operations
- `-mysql` - Generate MySQL database operations

### Optional Flags
- `-help` - Display usage information
- `-R` - Recursively process subdirectories for schema files

### Advanced Options
**Warning: The following flags generate exponential numbers of files and should be used with caution**

- `-enableExponentialOperations` - Required flag to enable exponential file generation
- `-selectAllFiles` - Generate SELECT ALL operation files for all field combinations
- `-selectFiles` - Generate SELECT operation files for all field combinations  
- `-insertFiles` - Generate INSERT operation files for all field combinations
- `-updateFiles` - Generate UPDATE operation files for all field combinations
- `-deleteFiles` - Generate DELETE operation files for all field combinations

### Examples

**Basic C++ generation:**
```bash
SchemaLangTranspiler -schemaDirectory=./schemas -outputDirectory=./output -cpp
```

**Multi-target generation with drop-in system:**
```bash
SchemaLangTranspiler -schemaDirectory=./schemas -outputDirectory=./output -cpp -json -sqlite
```

**Recursive directory processing:**
```bash
SchemaLangTranspiler -schemaDirectory=./schemas -outputDirectory=./output -cpp -json -R
```

**Generate with exponential operations (use with caution):**
```bash
SchemaLangTranspiler -schemaDirectory=./schemas -outputDirectory=./output -mysql -enableExponentialOperations -selectFiles
```

### Output Structure
Generated files are organized in subdirectories based on the target:
- `<outputDirectory>/Schemas/Cpp/` - C++ header and source files
- `<outputDirectory>/Schemas/Java/` - Java class files  
- `<outputDirectory>/Schemas/Json/` - JSON schema files
- `<outputDirectory>/Schemas/Sqlite/` - SQLite operation files
- `<outputDirectory>/Schemas/Mysql/` - MySQL operation files

## Basic Structure

### 1. **Struct Definition**
```
struct StructName {
    field_definitions...
}
```

### 2. **Field Definition Syntax**
```
type: field_name: modifiers: description("text")
```

**Components:**
- **type**: The data type (primitive, array, enum, or custom struct)
- **field_name**: The name of the field (identifier)
- **modifiers**: Optional attributes that define constraints and behaviors
- **description**: Human-readable documentation (required)

## Data Types

### Primitive Types
- **Integer Types:**
  - `int8` - 8-bit signed integer
  - `int16` - 16-bit signed integer  
  - `int32` - 32-bit signed integer
  - `int64` - 64-bit signed integer
  - `uint8` - 8-bit unsigned integer
  - `uint16` - 16-bit unsigned integer
  - `uint32` - 32-bit unsigned integer
  - `uint64` - 64-bit unsigned integer

- **Floating Point Types:**
  - `float` - Single precision floating point
  - `double` - Double precision floating point

- **Other Types:**
  - `bool` - Boolean (true/false)
  - `string` - Text string
  - `char` - Single character
  - `uchar` - Unsigned character
  - `void` - Void type (mainly for function returns)

### Complex Types
- `array<Type>` - Array of specified type
- `enum` - Enumeration (defined separately)
- Custom struct types (reference other defined structs)

## Modifiers

### Core Modifiers
- `required` - Field must have a value (default: false)
- `optional` - Field can be null/empty (explicit declaration)
- `unique` - Value must be unique across all instances
- `primary_key` - Designates the primary key field
- `auto_increment` - Automatically increments for new records

### Relationship Modifiers
- `foreign_key(StructName.field)` - Creates a foreign key relationship

### Array Modifiers
- `unique_items` - Array elements must be unique
- `min_items(n)` - Minimum number of items required (n = positive integer)

### Validation Modifiers
- `description("text")` - Documentation string explaining the field's purpose (required for all fields)
- `max_tokens(n)` - Maximum number of tokens/characters allowed (n = positive integer)

## Enum Definition

### Basic Enum
```
enum EnumName {
    Value1,
    Value2,
    Value3
}
```

### Enum with Explicit Values
```
enum Status {
    Inactive = 0,
    Active = 1,
    Suspended = 2
}
```

**Enum Rules:**
- Enum values are comma-separated
- Last value can optionally omit the trailing comma
- Values can be explicitly assigned integers using `=`
- If no value is specified, it auto-increments from the previous value (starting at 0)

## Complete Syntax Grammar

### Structure Definition
```
struct_definition ::= "struct" identifier "{" field_list "}"
field_list ::= field_definition*
field_definition ::= type ":" identifier ":" modifier_list ";"
modifier_list ::= modifier (":" modifier)*
```

### Field Types
```
type ::= primitive_type | array_type | enum_type | struct_type
primitive_type ::= "int8" | "int16" | "int32" | "int64" | "uint8" | "uint16" | "uint32" | "uint64" | "float" | "double" | "bool" | "string" | "char" | "uchar" | "void"
array_type ::= "array" "<" type ">"
enum_type ::= identifier (must be previously defined enum)
struct_type ::= identifier (must be previously defined struct)
```

### Modifiers
```
modifier ::= simple_modifier | parameterized_modifier
simple_modifier ::= "required" | "optional" | "unique" | "primary_key" | "auto_increment" | "unique_items"
parameterized_modifier ::= "foreign_key" "(" identifier "." identifier ")" | "description" "(" string ")" | "min_items" "(" integer ")" | "max_tokens" "(" integer ")"
```

### Enum Definition
```
enum_definition ::= "enum" identifier "{" enum_value_list "}"
enum_value_list ::= enum_value ("," enum_value)* ","?
enum_value ::= identifier ("=" integer)?
```

### Lexical Rules
- **Identifiers**: Start with letter or underscore, followed by letters, numbers, or underscores
- **Strings**: Enclosed in double quotes, support escape sequences with backslash
- **Integers**: Sequence of digits (0-9)
- **Comments**: Not explicitly supported in current implementation
- **Whitespace**: Spaces, tabs, and newlines are ignored except within strings

## Examples

### Simple Struct
```
struct Organization {
    int64: id: primary_key: required: unique: auto_increment: description("The unique identifier of the organization");
    string: name: required: description("The name of the organization");
    string: description: required: description("The description of the organization");
}
```

### Struct with Array Field
```
struct SCP {
    int64: id: primary_key: required: unique: auto_increment: description("The unique identifier of the SCP");
    string: name: required: description("The common name of the SCP");
    Classification: objectClass: required: description("The object class of the SCP");
    array<Addendum>: addenda: optional: description("The addenda of the SCP"): unique_items: min_items(4);
}
```

### Struct with Foreign Key
```
struct DClass {
    int64: id: primary_key: required: unique: auto_increment: description("The unique identifier of the D-Class"): foreign_key(Personel.id);
    string: designation: required: description("The designation of the D-Class");
    string: reason: required: description("The reason for the D-Class being in foundation possession");
}
```

### Enum with Mixed Value Assignment
```
enum Classification {
    None,
    Safe,
    Euclid,
    Keter,
    Thaumiel,
    Neutralized,
    Pending,
    Explained,
    Esoteric,
}
```

### Complex Struct with Multiple Field Types
```
struct Personel {
    int64: id: primary_key: required: unique: auto_increment: description("The unique identifier of the personel");
    Title: title: required: description("The title of the personel");
    string: first_name: required: description("The first name of the personel");
    string: second_name: optional: description("The middle name of the personel");
    string: last_name: required: description("The last name of the personel");
    Sex: sex: optional: description("the sex of the personel");
    string: email: required: description("The email of the personel");
    string: phone: required: description("The phone number of the personel");
    Organization: organization: required: description("The organization the personel is a part of");
}
```

## Best Practices

1. **Always include descriptions** - Every field must have a description explaining its purpose
2. **Use meaningful names** - Choose clear, descriptive names for structs, fields, and enums
3. **Specify constraints explicitly** - Always declare `required` or `optional` for clarity
4. **Use appropriate data types** - Choose the right size integers, use `int64` for IDs
5. **Define relationships** - Use foreign keys when linking between structs
6. **Use enums for constrained values** - Define enums for fields with a fixed set of possible values
7. **Include validation constraints** - Use `min_items`, `max_tokens`, `unique_items` where appropriate
8. **Organize definitions** - Define enums before structs that use them
9. **Follow naming conventions** - Use consistent naming patterns across your schema

## Syntax Rules and Constraints

- **Field Termination**: All field definitions must end with a semicolon (`;`)
- **Modifier Separation**: Multiple modifiers are separated by colons (`:`)
- **Array Syntax**: Array types use angle brackets: `array<Type>`
- **Enum Values**: Comma-separated, with optional trailing comma
- **String Literals**: Must be enclosed in double quotes, support escape sequences
- **Foreign Key Format**: Must reference existing struct and field: `StructName.field`
- **Case Sensitivity**: All identifiers are case-sensitive
- **Reserved Words**: Cannot use data type names as identifiers

## Error Handling

The parser provides detailed error messages for common syntax errors:
- Missing colons or semicolons
- Invalid type names
- Malformed array syntax
- Invalid foreign key references
- Missing or malformed descriptions
- Invalid modifier parameters

## Code Generation

SchemaLang can generate code for multiple targets, each with specific capabilities:

### Object-Oriented Language Generators

- **C++**: Complete classes with private member variables, getters/setters, constructors, and support for generator drop-ins
- **Java**: Complete classes with private member variables, getters/setters, constructors, and support for generator drop-ins (currently in development)

### Schema and Database Generators

- **JSON Schema**: Complete JSON schema definitions with validation rules and type constraints
- **MySQL**: Database schema with CREATE TABLE statements, indexes, and foreign key constraints
- **SQLite**: Database schema with CREATE TABLE statements, indexes, and foreign key constraints

### Generator Capabilities

#### Standalone Generation

Each generator can work independently:

- **C++/Java**: Produces clean, well-structured classes suitable for any application
- **JSON Schema**: Creates comprehensive schemas for API validation and documentation
- **MySQL/SQLite**: Generates complete database schemas ready for deployment

#### Drop-In Enhanced Generation

When combined with C++ or Java generators, specialized generators can enhance the generated classes:

**JSON + C++/Java:**

- Creates base classes for serialization (`HasJsonSchema`)
- Injects `toJSON()`, `fromJSON()`, and `getSchema()` methods
- Provides complete JSON serialization/deserialization implementations

**SQLite + C++/Java:**

- Directly injects database operation methods
- Adds SELECT methods for each field (e.g., `SQLiteSelectByid()`, `SQLiteSelectBytitle()`)
- Includes INSERT, UPDATE, and table creation methods
- Provides both static utility methods and instance methods

**MySQL + C++/Java:**

- Directly injects MySQL X DevAPI methods
- Adds SELECT methods returning vectors of objects
- Includes INSERT, UPDATE, and table creation methods
- Supports both static operations and instance methods

#### Multi-Generator Combinations

SchemaLang excels when multiple generators are used together:

- **C++ + JSON + SQLite**: Creates classes with JSON serialization and SQLite database operations
- **C++ + MySQL + JSON**: Combines MySQL database operations with JSON API capabilities
- **C++ + SQLite + MySQL + JSON**: Full-stack classes with multiple database backends and serialization

Each generator respects the modifiers and constraints defined in the schema, ensuring consistency across all generated outputs. The drop-in system allows for seamless integration between generators, creating powerful, unified classes that handle data persistence, serialization, and validation automatically.

## Generator Drop-In System

SchemaLang features an advanced **generator drop-in system** that allows specialized generators (SQLite, MySQL, JSON) to inject methods and base classes into object-oriented language generators (C++, Java). This creates a unified interface where database operations, serialization, and schema validation are seamlessly integrated into the generated classes.

### How the Drop-In System Works

When multiple generators are enabled (e.g., `--cpp --sqlite --json`), the system works as follows:

1. **Base Class Generation** (Optional): Some generators can instruct the C++/Java generator to create base classes with virtual methods
2. **Method Injection**: Database and serialization generators inject specific method definitions into the generated classes
3. **Implementation Drop-In**: Generators provide complete implementations for their injected methods
4. **Inheritance Setup** (When Base Classes Used): Generated classes inherit from appropriate base classes and implement virtual methods

**Note**: Base classes are optional - generators can choose to either create base classes with virtual methods (like JSON) or simply inject methods directly into classes (like SQLite and MySQL).

### Example: JSON Generator Integration

When the JSON generator is enabled with C++, it creates a base class `HasJsonSchema` and injects these methods:

```cpp
// Base class provided by JSON generator
class HasJsonSchema {
public:
    virtual json toJSON() = 0;
    virtual void fromJSON(json j) = 0;
    virtual json getSchema() = 0;
};

// Methods injected into generated classes
json toJSON() override;
void fromJSON(json j) override;
json getSchema() override;
```

### Example: SQLite Generator Integration

When the SQLite generator is enabled with C++, it directly injects database operation methods without creating a base class:

```cpp
// Select methods (by each field) - injected directly
void SQLiteSelectByid(sqlite3 * db, int64_t id);
void SQLiteSelectBytitle(sqlite3 * db, std::string title);
void SQLiteSelectBycontent(sqlite3 * db, std::string content);

// Insert methods - injected directly
static bool SQLiteInsert(sqlite3 * db, int64_t id, std::string title, std::string content);
bool SQLiteInsert(sqlite3 * db);

// Schema and table creation - injected directly
static std::string getSQLiteCreateTableStatement();
static bool SQLiteCreateTable(sqlite3 * db);
```

### Example: MySQL Generator Integration

Similarly, the MySQL generator directly injects MySQL-specific methods without a base class:

```cpp
// Select methods using MySQL X DevAPI - injected directly
static std::vector<AddendumSchema*> MySQLSelectByid(mysqlx::Session & session, int64_t id);
static std::vector<AddendumSchema*> MySQLSelectBytitle(mysqlx::Session & session, std::string title);
static std::vector<AddendumSchema*> MySQLSelectBycontent(mysqlx::Session & session, std::string content);

// Insert and update methods - injected directly
static bool MySQLInsert(mysqlx::Session & session, int64_t id, std::string title, std::string content);
bool MySQLInsert(mysqlx::Session & session);
static bool MySQLUpdateAddendum(mysqlx::Session & session, int64_t id, std::string title, std::string content);

// Schema and table creation - injected directly
static std::string getMySQLCreateTableStatement();
static bool MySQLCreateTable(mysqlx::Session & session);
```

### Complete Example: Addendum Class

When all generators are enabled, a struct like this:

```schemalang
struct Addendum {
    int64: id: required: unique: auto_increment: foreign_key(SCP.id): description("The SCP this addendum is attached to");
    string: title: required: description("The title of the addendum");
    string: content: required: description("The content of the addendum");
}
```

Generates a C++ class with all these capabilities:

```cpp
class AddendumSchema : public HasJsonSchema {  // Inherits from JSON base class
private:
    int64_t id;
    std::string title;
    std::string content;

public:
    // Standard getters and setters
    int64_t getId() const { return id; }
    void setId(int64_t value) { id = value; }
    std::string getTitle() const { return title; }
    void setTitle(const std::string& value) { title = value; }
    std::string getContent() const { return content; }
    void setContent(const std::string& value) { content = value; }

    // JSON serialization methods (from JSON generator - overridden from base class)
    json toJSON() override;
    void fromJSON(json j) override;
    json getSchema() override;

    // SQLite database methods (from SQLite generator - directly injected)
    void SQLiteSelectByid(sqlite3 * db, int64_t id);
    void SQLiteSelectBytitle(sqlite3 * db, std::string title);
    void SQLiteSelectBycontent(sqlite3 * db, std::string content);
    static bool SQLiteInsert(sqlite3 * db, int64_t id, std::string title, std::string content);
    bool SQLiteInsert(sqlite3 * db);
    static std::string getSQLiteCreateTableStatement();
    static bool SQLiteCreateTable(sqlite3 * db);

    // MySQL database methods (from MySQL generator - directly injected)
    static std::vector<AddendumSchema*> MySQLSelectByid(mysqlx::Session & session, int64_t id);
    static std::vector<AddendumSchema*> MySQLSelectBytitle(mysqlx::Session & session, std::string title);
    static std::vector<AddendumSchema*> MySQLSelectBycontent(mysqlx::Session & session, std::string content);
    static bool MySQLInsert(mysqlx::Session & session, int64_t id, std::string title, std::string content);
    bool MySQLInsert(mysqlx::Session & session);
    static bool MySQLUpdateAddendum(mysqlx::Session & session, int64_t id, std::string title, std::string content);
    static std::string getMySQLCreateTableStatement();
    static bool MySQLCreateTable(mysqlx::Session & session);
};
```

### Benefits of the Drop-In System

1. **Unified Interface**: Single class provides database operations, serialization, and validation
2. **Type Safety**: All operations use the correct types as defined in the schema
3. **Consistency**: Method naming and behavior is consistent across all generated classes
4. **Extensibility**: New generators can easily integrate with existing object-oriented generators
5. **Maintainability**: Changes to schema automatically update all related operations
6. **Performance**: Static methods avoid unnecessary object instantiation for utility operations

### Supported Generator Combinations

- **C++ & JSON**: Adds JSON serialization capabilities
- **C++ & SQLite**: Adds SQLite database operations
- **C++ & MySQL**: Adds MySQL database operations
- **C++ & Multiple**: Combines all selected generators into a single comprehensive class
- **Java & [Any]**: Same drop-in system (currently in development)

This drop-in system makes SchemaLang particularly powerful for full-stack development, allowing you to define your data model once and get complete database integration, API serialization, and type-safe operations across your entire application.

This comprehensive syntax enables you to define complex data structures with built-in validation, relationships, and documentation, making it ideal for generating database schemas, API specifications, or data validation code across multiple platforms and languages. The advanced generator drop-in system further enhances productivity by creating unified classes that combine database operations, serialization, and validation in a single, type-safe interface.
