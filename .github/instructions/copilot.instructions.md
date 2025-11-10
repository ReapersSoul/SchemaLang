# SchemaLang AI Development Guide

## Project Overview

SchemaLang is a schema-to-code transpiler that converts `.schema`/`.schemaLang` files into multiple target languages. The architecture centers around a **drop-in generator system** where specialized generators (SQLite, JSON, MySQL) inject methods into object-oriented generators (C++, Java, Lua), creating unified classes with database operations, serialization, and validation.

**Core Architecture:**
- **Parser**: `ProgramStructure` converts schema files into `StructDefinition`/`EnumDefinition` objects with position-aware error reporting
- **Generator System**: Base `Generator` class with pluggable implementations for each target language
- **Drop-In System**: Cross-generator method injection via `add_generator_specific_content_to_struct()`
- **Embedded Templates**: Inja templates stored in compressed virtual filesystem for code generation
- **Dynamic Loading**: Runtime loading of custom generators from `.so/.dll` files

## Essential Development Patterns

### Embedded Resource Template System
**Critical**: Templates are embedded at build time from `embedded_resources/` directory structure:
```
embedded_resources/[TargetLang]/[SourceLang]/[Functions|Getters|Setters|Variables|Files]/
```

**Access Pattern:**
```cpp
// Load templates during generation
std::vector<uint8_t> templateData = loadEmbeddedResourcesEmbeddedFile("/Cpp/Json/Functions/toJSON.cpp");
std::string template_content(reinterpret_cast<const char*>(templateData.data()), templateData.size());
inja::Environment env = getEnv(gen, ps);
std::string generated_code = env.render(template_content, json_data);
```

**Build Dependency**: Changes to `embedded_resources/` require full rebuild to update embedded filesystem.

### Generator Drop-In System
The most critical pattern - generators enhance each other's output:

```cpp
// CppGenerator receives SQLite methods:
bool SqliteGenerator::add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s) {
    if (gen->name == "Cpp") {
        // Inject SQLite methods into C++ classes
        return fetch_additions(ps, gen, additions, data);
    }
}
```

**Registration Order**: In `main.cpp`, generators must be added to `CppGenerator` via `add_generator()` for drop-in system to work.

### Dynamic Generator Loading
**Interface Contract** (see `DynamicGeneratorInterface.hpp`):
```cpp
extern "C" {
    Generator* getGeneratorInstance();           // Required: returns generator instance
    const char* getGeneratorName();              // Required: for output directory naming
    void registerArguments(argumentParser* parser); // Optional: custom CLI args
}
```

**Loading Process**: `main.cpp` uses Boost.DLL to scan `-additionalGenerators` directory for `.so/.dll` files.

## Critical Implementation Details

### Parser Position Tracking
Always use `Token` objects with `SourcePosition` for error reporting:
```cpp
void reportError(const std::string& message, const Token& token) {
    // Provides file:line:column context
}
```

### Type System & Conversion
Each generator implements `convert_to_local_type()` for schema-to-target type mapping:
- Primitive: `int8`-`uint64`, `bool`, `string`, `char`, `float`, `double`
- Complex: `array<Type>`, custom structs, enums, `pointer` (references)

### Template Variable Access
Templates have access to full schema context via JSON with custom Inja callbacks:
- `{{GeneratorName_format_include(ident)}}` - Include statement formatting
- `{{GeneratorName_format_default(type, value)}}` - Default value formatting
- `{{GeneratorName_convert_to_local_type(type)}}` - Type conversion

### VFS Initialization Pattern
**Always Required**: VFS must be initialized before template access:
```cpp
if (!initEmbeddedResourcesEmbeddedVFS(argv[0])) return 1;
if (!mountEmbeddedResourcesEmbeddedVFS()) return 1;
```

## Development Workflows

### Building & Testing
```bash
# Standard CMake build (vcpkg required for dependencies)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build

# Test with sample schemas (drop-in system demonstration)
./bin/SchemaLangTranspiler -schemaDirectory=bin/inputSchemas -outputDirectory=bin/Schemas -cpp -json -sqlite
```

### Adding New Built-in Generators
1. Inherit from `Generator` in `include/BuiltInGenerators/YourGenerator.hpp`
2. Implement required virtuals: `convert_to_local_type()`, `generate_files()`, `add_generator_specific_content_to_struct()`
3. Create templates in `embedded_resources/YourLang/` directory structure
4. Register in `main.cpp` and add to `CppGenerator` for drop-in support
5. Add cross-generator templates in `embedded_resources/Cpp/YourLang/` for method injection

### Schema Include System
Schema files support includes with circular dependency prevention:
```
include "./base.schema"  // Parsed once, merged into ProgramStructure
```

### Template Development Tips
- Use `{{#if}}` conditions to handle optional fields
- Access nested data via dot notation: `{{struct.member_variables}}`
- Loop with `{{#for item in items}}{{item.identifier}}{{/for}}`
- Reference other generators: `{{#for key,gen in generators}}{{gen.functions}}{{/for}}`

## VS Code Integration

The `lsp-schema-lang/` directory contains a complete Language Server Protocol implementation:
- Syntax highlighting for `.schema`/`.schemaLang` files
- Semantic tokens and error reporting
- **Development Note**: Schema syntax changes require updating both parser logic AND LSP grammar

## Error Handling Patterns

### Parser Error Recovery
Continue parsing after errors when possible using position-aware tokens. Use `validate()` for final consistency checks.

### Generator Error Handling
```cpp
// Template rendering errors
try {
    content = env.render(template_content, data);
} catch (const std::exception& e) {
    throw std::runtime_error("Template error in " + filename + ": " + e.what());
}
```

## Common Anti-Patterns to Avoid

- **Don't** call template loading functions before VFS initialization
- **Don't** modify `embedded_resources/` without rebuilding - changes won't take effect
- **Don't** add generators to drop-in system after argument parsing - registration order matters
- **Don't** use raw strings in templates - use proper escaping and JSON serialization
- **Don't** forget to implement `add_generator_specific_content_to_struct()` for drop-in compatibility

## Project-Specific Conventions

- Generator names determine template paths and output directories
- All member variables use camelCase identifiers in generated code
- Foreign key relationships automatically create parent-child links in database generators
- Array fields in schemas map to separate tables with foreign keys in SQL generators
- Optional fields use `std::optional<>` in C++, nullable columns in SQL

This architecture enables powerful code generation where a single schema definition produces complete, integrated classes with database operations, serialization, and validation across multiple languages.