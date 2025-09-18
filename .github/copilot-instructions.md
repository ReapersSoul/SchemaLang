# SchemaLang AI Development Guide

## Project Overview

SchemaLang is a schema definition language with multi-target code generation that transpiles `.schema`/`.schemaLang` files into C++, JSON schemas, SQL, and more. The core architecture revolves around:

- **Parser**: Converts schema files into `ProgramStructure` containing `StructDefinition` and `EnumDefinition` objects
- **Generator System**: Pluggable generators that transform parsed structures into target languages  
- **Drop-In System**: Allows generators to inject methods/content into other generators' output (e.g., SQLite methods into C++ classes)
- **Embedded Template System**: Uses Inja templates stored in `embedded_resources/` for code generation

## Key Architectural Patterns

### Generator Drop-In System
The most critical pattern - generators can enhance each other's output:
```cpp
// CppGenerator can have SQLite methods injected:
class PersonSchema {
    // Standard C++ getters/setters
    std::string getName() const;
    
    // SQLite generator injects these methods:
    void SQLiteSelectByname(sqlite3* db, std::string name);
    static bool SQLiteCreateTable(sqlite3* db);
};
```

**Implementation**: Generators implement `add_generator_specific_content_to_struct()` to inject content into other generators. The `CppGenerator::add_generator()` method enables this cross-generator integration.

### Embedded Resource Templates
Templates are embedded at build time from `embedded_resources/` using a custom compression system:
- Structure: `embedded_resources/[TargetLang]/[SourceLang]/[Functions|Getters|Setters|Variables]/`
- Example: `embedded_resources/Cpp/Json/Functions/` contains C++ method templates for JSON serialization
- Access via: `loadEmbeddedResourcesEmbeddedFile()`, `listEmbeddedResourcesEmbeddedFiles()`

### Dynamic Generator Loading
Runtime loading of custom generators via shared libraries:
```cpp
// Dynamic generators must implement extern "C" functions:
extern "C" {
    Generator* getGeneratorInstance();
    const char* getGeneratorName(); 
    void registerArguments(argumentParser* parser); // optional
}
```

## Development Workflows

### Building
```bash
# Standard CMake build with vcpkg dependencies
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

**Key**: The build embeds resources from `embedded_resources/` into the binary. Changes to templates require rebuilding.

### Testing Schema Changes
```bash
# Test with sample schemas
./bin/SchemaLangTranspiler -schemaDirectory=bin/inputSchemas -outputDirectory=bin -cpp -json -sqlite
```

### Adding New Generators
1. Inherit from `Generator` base class
2. Implement required virtual methods: `convert_to_local_type()`, `generate_files()`, `add_generator_specific_content_to_struct()`
3. Create templates in `embedded_resources/[YourLang]/` 
4. For drop-in support, create cross-generator templates in `embedded_resources/[TargetLang]/[YourLang]/`

### Template Development
Templates use Inja syntax with custom callbacks:
- `format_include(ident)` - Format include statements per generator
- `format_default(type, value)` - Format default values per type
- Access to full schema context via JSON data

## Critical Implementation Details

### Parser Error Handling
Uses position-aware tokens (`Token` struct) for precise error reporting. Always use `reportError()` with position context.

### Type System
- Primitive types: `int8`-`uint64`, `bool`, `string`, `char`, `float`, `double`
- Complex types: `array<Type>`, custom structs, enums
- Special: `pointer` (references), `void` (functions)

### Generator Registration Order
In `main.cpp`, generators must be registered in correct order for drop-in system:
1. Load dynamic generators first
2. Add them to `CppGenerator` via `add_generator()`
3. Cross-register generators for bidirectional drop-in support

### Schema File Inclusion
Schema files can include others: `include "./other.schema"`. The parser maintains `already_included_files` to prevent circular dependencies.

## VS Code Extension Integration
The `lsp-schema-lang/` directory contains a Language Server Protocol implementation providing:
- Syntax highlighting for `.schema`/`.schemaLang` files
- Semantic token types for schema elements
- Error reporting during development

**Development**: Extension and transpiler are separate - changes to schema syntax require updating both parser logic and LSP grammar.

## Common Patterns

### Adding Schema Modifiers
1. Update parser in `ProgramStructure::readMemberVariable()`
2. Add modifier handling in relevant generators
3. Update embedded templates if needed

### Cross-Generator Communication
Use the `generators` vector in `Generator` base class to access other generators and their capabilities during code generation.

### Error Recovery
Parser continues after errors when possible. Use `validate()` method for final consistency checks across all parsed structures.