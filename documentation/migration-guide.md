# SchemaLang Migration Guide — JSON Schema / Prisma → SchemaLang

Purpose

- Practical guide for moving models from JSON Schema or Prisma into SchemaLang with minimal disruption.

Overview

- SchemaLang generates OO classes, DB schemas, serialization, and scripting attachments from a single schema file. Migration focuses on converting type/constraint syntax and validating parity across generated outputs.

Quick Mapping Rules

- Types
  - JSON Schema `integer`/`number`: map to `int32/int64`/`float/double` depending on size; Prisma `Int` → `int32`/`int64`.
  - `string` → `string`; `boolean` → `bool`.
  - `array` → `array<Type>`: JSON arrays of objects map to `array<MyStruct>` or will be flattened to separate tables.
- Required/Optional
  - JSON Schema `required` array → `required` modifier on the field.
  - Prisma `?`/nullable fields map to `optional`.
- References & Relations
  - JSON Schema `$ref` → references to other struct types; use `reference(Struct.id)` for DB foreign keys if desired.
  - Prisma `relation` → `reference` and array patterns in SchemaLang with `array<RelationType>`.
- Enums
  - JSON Schema `enum` and Prisma `enum` → SchemaLang `enum` definitions.
- Validation
  - JSON Schema `minItems`, `maxItems`, `pattern`, `minimum/maximum` → use `min_items(n)`, `max_items(n)`, and inline descriptions; plus generator-specific constraints where supported.

Step-by-step Migration Workflow

1. Inventory and Prioritize
   - List existing models and dependencies.
   - Start with a small core module to validate changes (e.g., `User`, `Profile`).
2. Convert Types and Fields
   - Translate primitive types and enums using mapping rules above.
   - Ensure `id` fields are not included (SchemaLang auto-injects `id: int64`).
3. Convert Relations
   - For 1-to-many: either create a separate struct for the child with a `reference(parent.id)`, or use `array<Child>` on the parent.
   - For many-to-many: create a join struct explicitly.
4. Convert Validations & Constraints
   - Preserve essential constraints via modifiers (`required`, `unique`, `min_items`, `max_items`) and `description` text for context.
5. Add Generator Modifiers
   - Use `gens_enabled(Cpp,Json,SQLite,MySQL)` or `gens_disabled()` to fine-tune target outputs during migration.
6. Validate & Generate
   - Run `SchemaLangTranspiler -schema=<test-schema> -outputDirectory=./out -cpp -json -sqlite`.
   - Review generated sources and SQL for expected behavior.
7. Iterate and Expand
   - Migrate remaining models, adjust for non-trivial cases (conditional validations, union types) and verify behavior with runtime tests.

Verification & Rollout

- Dual-run Strategy: During rollout, generate SchemaLang artifacts alongside existing runtime models but don’t replace the runtime until parity tests pass.
- Migration Diffs: If migrating DB structure, develop migration scripts that transform the database to the new shape and validate with test data.
- Golden-file Tests: Use golden-file testing for generated outputs: C++ headers, JSON schema, SQL DDL for easy regression checks.

Practical Tips

- Always add `description("...")` to each field during migration — SchemaLang requires descriptions.
- Use `gens_enabled` selectively during migration to avoid noise from less relevant targets.
- For hand-written SQL: use the drop-in system’s hook points to inject or override queries when performance matters.
- Use `SchemaLangDebugger` to step through complex parse errors during conversion.

Example (JSON Schema → SchemaLang)

-- JSON Schema:

```json
{
  "type": "object",
  "properties": {
    "name": { "type": "string" },
    "age": { "type": "integer" }
  },
  "required": ["name"]
}
```

-- SchemaLang equivalent:

```schemalang
struct Person {
    string: name: required: description("Person's name");
    int32: age: optional: description("Person's age");
}
```

Closing

- Keep migrations incremental: migrate a few models, verify, then expand. The single-most helpful step is adding enforced `description` text during conversion so generated docs and tools are useful immediately.
