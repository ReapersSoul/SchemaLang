# SchemaLang Migration File Format

## Overview

SchemaLang **automatically generates** migration files when struct versions change. These files describe the differences between versions, enabling automated schema evolution and code generation for databases and object models.

### File Naming and Location

Migration files are auto-generated with the naming pattern:
```
<StructName>_<fromVersion>_to_<toVersion>.schema.migration
```

Examples:
- `Player_1_0_0_to_1_1_0.schema.migration`
- `Quest_0_0_0_to_1_0_0.schema.migration`

Files are placed in the directory specified by `-migrationsPath=<path>` when running the transpiler.

### How Auto-Generation Works

When you run the transpiler with `-migrationsPath=<path>`, the system:

1. **Detects version changes** - Compares current struct versions against existing migrations
2. **Generates migration files** - Automatically creates `.schema.migration` files describing changes
3. **Parses migration files** - Reads all `.schema.migration` files from the migrations directory
4. **Validates operations** - Checks operations against struct definitions
5. **Embeds SQL** - Generates migration SQL and embeds it as C++ functions in database code
6. **Enables auto-migration** - Generated database classes apply migrations on connection

**Important:** 
- You **never manually create** migration files - they are auto-generated
- Migrations are **not** written as separate `.sql` files - they are compiled directly into your application binary as C++ functions
- This ensures migrations can never get out of sync with your code

---

## Syntax

A migration file consists of a migration block for a struct, specifying the source and target versions and a list of operations:

```schema
migration struct <StructName> from <fromVersion> to <toVersion> {
    <operation1>;
    <operation2>;
    ...
}
```

---

## Supported Operations

### 1. Add Field

Adds a new field to the struct.

```schema
add field <fieldName>: <type>: <modifiers>;
```

### 2. Remove Field

Removes a field from the struct.

```schema
remove field <fieldName>;
```

### 3. Rename Field

Renames a field.

```schema
rename field <oldFieldName> to <newFieldName>;
```

### 4. Change Field Type

Changes the type of a field.

```schema
change field <fieldName> type from <oldType> to <newType>;
```

### 5. Change Field Modifier

Changes a modifier (e.g., `required`, `optional`, `unique`, etc.) for a field.

```schema
change field <fieldName> modifier from <oldModifier> to <newModifier>;
change field <fieldName> modifier add <modifier>;
change field <fieldName> modifier remove <modifier>;
change field <fieldName> modifier set <modifier>(<value>);
```

### 6. Set Field Description

Updates the description of a field.

```schema
change field <fieldName> modifier set description("<new description>");
```

---

## Example Migration File

```schema
migration struct Country from 1.0.0 to 1.1.0 {
    add field population: int64: required;
    rename field leader_id to president_id;
    change field city_ids modifier from optional to required;
    change field region_ids modifier add unique;
    change field location_ids modifier remove min_items;
    change field capital_id modifier set reference(CapitalCity.id);
    change field description modifier set description("Updated description for v1.1.0");
}
```

---

## Notes

- Multiple migration blocks can be included in a single file if needed.
- All operations should be terminated with a semicolon (`;`).
- Modifiers can include: `required`, `optional`, `unique`, `min_items`, `reference`, `description`, etc.
- Migration files should be kept in version control to track schema evolution.

---

## Generated Migration Code

For database generators (SQLite, MySQL), migration operations are converted to SQL and embedded in the generated code:

### Example: Generated SQLite Migration Function

```cpp
// In SQLiteDB.cpp
std::string SQLiteDB::migrate_Country_table_1_0_0_to_1_1_0() {
    return R"SQL(
    BEGIN TRANSACTION;
    
    -- add field population: int64: required;
    ALTER TABLE Country ADD COLUMN population INTEGER NOT NULL DEFAULT 0;
    
    -- rename field leader_id to president_id;
    ALTER TABLE Country RENAME COLUMN leader_id TO president_id;
    
    -- Update schema version
    UPDATE _schema_versions 
    SET major=1, minor=1, patch=0 
    WHERE struct_name='Country';
    
    COMMIT;
    )SQL";
}
```

### Example: Auto-Migration in connect()

```cpp
void SQLiteDB::connect(const std::string& db_path) {
    // ... connection setup ...
    
    // Check Country table version
    auto current_version = get_struct_version("Country");
    if (current_version.major == 1 && 
        current_version.minor == 0 && 
        current_version.patch == 0) {
        
        std::string migration_sql = migrate_Country_table_1_0_0_to_1_1_0();
        execute(migration_sql);
        PLOGI << "Applied migration: Country 1.0.0 -> 1.1.0";
    }
    
    // ... additional version checks ...
}
```

## Auto-Generation Workflow

1. **Define struct with initial version:**
   ```schemalang
   struct Country: version(1.0.0) {
       string: name: required: description("Country name");
       int64: leader_id: required: description("Leader ID");
   }
   ```

2. **Initial transpile with migrations:**
   ```bash
   SchemaLangTranspiler -schema=./schemas -outputDirectory=./output -cpp -sqlite -migrationsPath=./migrations
   ```
   Creates: `migrations/Country_0_0_0_to_1_0_0.schema.migration` (initial migration)

3. **Update struct and increment version:**
   ```schemalang
   struct Country: version(1.1.0) {
       string: name: required: description("Country name");
       int64: president_id: required: description("President ID");
       int64: population: required: description("Population count");
   }
   ```

4. **Regenerate - migration file is auto-created:**
   ```bash
   SchemaLangTranspiler -schema=./schemas -outputDirectory=./output -cpp -sqlite -migrationsPath=./migrations
   ```
   **Automatically creates:** `migrations/Country_1_0_0_to_1_1_0.schema.migration` with:
   ```schemalang
   migration struct Country from 1.0.0 to 1.1.0 {
       add field population: int64: required;
       rename field leader_id to president_id;
   }
   ```

5. **Deploy** - Your application now contains embedded migration logic

## Best Practices

- **Always increment the struct version** when making changes
- **Always specify `-migrationsPath`** to enable auto-generation
- **Test generated code** before deploying to production
- **Use semantic versioning** appropriately:
  - Patch (x.y.Z) - Bug fixes, description changes
  - Minor (x.Y.0) - New optional fields, non-breaking additions
  - Major (X.0.0) - Breaking changes, removed fields, type changes
- **Keep migration files in version control** - Track auto-generated `.schema.migration` files
- **Never manually edit migration files** - They are regenerated on each transpile
- **Review auto-generated migrations** before committing to ensure they match your intent
- **Test migrations** on a copy of production data before deployment
- **Commit schema and migrations together** - Keep them in sync in version control
