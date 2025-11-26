# SchemaLang Migration File Format

## Overview

SchemaLang migration files describe changes between versions of a struct, enabling automated schema evolution and code generation for databases and object models. Migration files allow you to specify field additions, removals, renames, type changes, and modifier changes (such as `required`, `optional`, `unique`, etc.).

Migration files are typically named using the pattern:
```
<StructName>_migration_<fromVersion>_to_<toVersion>.schema.migration
```
and placed in a dedicated `migrations/` directory.

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

## Best Practices

- Always increment the struct version when making breaking changes.
- Document the rationale for each migration in comments.
- Test migrations with generated code and database scripts before deploying.
