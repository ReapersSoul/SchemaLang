# SchemaLang Writing Guide for AI Agents

## Core Syntax Rules

### Field Definition Format
**Every field must follow this exact pattern:**
```
type: field_name: modifiers: description("text");
```
- **Colons separate** type, name, modifiers, and description
- **Semicolon terminates** every field definition
- **Description is mandatory** - every field requires `description("...")` as the final modifier
- **Modifiers stack** - multiple modifiers separated by colons: `required: unique: auto_increment`

### Valid Primitive Types
```
int8, int16, int32, int64, uint8, uint16, uint32, uint64
float, double, bool, string, char, uchar
```

### Complex Types
- **Arrays:** `array<Type>` - e.g., `array<Character>`, `array<int64>`
- **Custom structs:** Reference other defined structs by name
- **Enums:** Reference defined enum names

### Struct Definition
```
struct StructName {
    field_definitions;
}

// With generator modifiers (optional)
struct StructName: gens_disabled(MySQL) {
    field_definitions;
}
```

**Critical:** Every struct automatically gets an `id: int64` primary key field injected. **Never manually add an `id` field** - the parser will reject it.

### Enum Definition
```
enum EnumName {
    Value1,
    Value2,
    Value3
}

// With explicit values
enum Status {
    Inactive = 0,
    Active = 1,
    Suspended = 2
}

// With generator modifiers
enum EndpointType: gens_disabled(MySQL) {
    REST,
    GraphQL
}
```

## Essential Modifiers

### Constraint Modifiers
- `required` - Field must have value (not nullable)
- `optional` - Field can be null
- `unique` - Value must be unique across all records
- `primary_key` - Marks field as primary key
- `auto_increment` - Auto-increments on new records

### Relationship Modifiers
- `reference(StructName.field_name)` - Creates foreign key to another struct
  - Example: `int64: character_id: required: reference(Character.id): description("The character ID");`

### Array Modifiers (only valid on array fields)
- `unique_items` - Array elements must be unique
- `min_items(n)` - Minimum array length (n = positive integer)
- `max_items(n)` - Maximum array length

### Generator Control
- `gens_enabled(Cpp,Json,SQLite)` - Whitelist: only these generators include this item
- `gens_disabled(MySQL,Lua)` - Blacklist: these generators exclude this item
- **Never use both on same item** - parser rejects whitelist + blacklist combination

## Critical Patterns from Real Examples

### Reference Pattern (Foreign Keys)
```schemalang
// Many-to-one: Party references Character as leader
struct Party {
    int64: leader_id: required: reference(Character.id): description("The leader character");
}

// Optional reference with nullable
struct Quest {
    int64: assignedCharacter_id: optional: reference(Character.id): description("Assigned character");
}
```

### Array of Primitives
```schemalang
// Array of IDs (common for many-to-many relationships)
struct Religion {
    array<int64>: god_ids: required: reference(Character.id): description("The gods worshipped");
}
```

### Array of Complex Types
```schemalang
// Array of structs - generates foreign key in AbilityEffect table
struct Ability {
    array<AbilityEffect>: effects: required: unique_items: min_items(1): description("Effects list");
}
// SQL Result: AbilityEffect table gets ability_id foreign key column
// Not: Effects are not embedded - separate table with relationship
```

### Boolean Flags
```schemalang
struct Character {
    bool: strip_asterisk: required: description("Whether character is a strip asterisk");
    bool: strip_quote: required: description("Whether character is a strip quote");
}
```

### Enum Usage
```schemalang
// Define enum first
enum EquipSlot {
    Head, Chest, Legs, Feet, Hands, Weapon, Ring, Necklace
}

// Use in struct
struct Items {
    EquipSlot: equipSlot: optional: description("The equip slot for this item");
}
```

## Include System
```schemalang
include "./other.schema"
include "./Character.schema"

// Circular dependency prevention: files included only once
// Relative paths start with "./" - parser resolves from current file's directory
// Absolute paths start with "/"
```

## Common Validation Errors to Avoid

1. **Missing semicolons** - Every field definition must end with `;`
2. **Missing description** - Every field must have `description("text")`
3. **Manual id fields** - Never add `id` field; it's auto-generated
4. **Circular references without reference()** - If struct A contains struct B and B contains A, at least one must use `reference()` modifier
5. **Array without element type** - Always specify: `array<Type>`, never just `array`
6. **Invalid reference format** - Must be `reference(StructName.field)`, not `reference(StructName)` or `reference(field)`
7. **Both gens_enabled and gens_disabled** - Pick one or neither, never both
8. **Missing colons between modifiers** - Each modifier separated by `:`

## Generator-Specific Behavior

### SQL Table Generation
SQL generators (MySQL/SQLite) create a table for **every struct and enum** defined in the schema:
```schemalang
struct CharacterAlias {
    string: alias: required: description("An alias");
}

struct Character {
    array<CharacterAlias>: aliases: required: description("Character aliases");
}
// Generates TWO tables: Character and CharacterAlias
// CharacterAlias table gets a character_id foreign key column automatically
// The array<CharacterAlias> creates the foreign key relationship, not the table itself
```

**Key principle:** Arrays with complex types (structs) generate foreign key columns in the referenced struct's table that point back to the parent struct.

### Optional Fields → Nullable Columns
`optional` modifier translates to:
- SQL: `NULL` columns
- C++: `std::optional<T>`
- JSON Schema: `"required": false`

### Reference Modifier Impact
`reference(Struct.field)` generates:
- SQL: `FOREIGN KEY` constraints
- C++: Separate field for the ID, getter/setter methods
- JSON: Reference documentation in schema

## Quick Checklist for Writing Valid Schemas

- ✓ Every field ends with semicolon
- ✓ Every field has `description("...")` as final modifier
- ✓ Colons separate type, name, modifiers, description
- ✓ No manual `id` fields (auto-injected)
- ✓ Arrays specify element type: `array<Type>`
- ✓ References use full format: `reference(Struct.field)`
- ✓ Either `required` or `optional` explicitly stated (best practice)
- ✓ Forward-declared structs (via `include` or earlier in file) before use
- ✓ Enum defined before struct uses it

## Real-World Example Patterns

```schemalang
// Complex struct with multiple relationship types
struct Quest {
    // Optional foreign keys (nullable references)
    int64: postedByFaction_id: optional: reference(Faction.id): description("Posting faction");
    int64: assignedParty_id: optional: reference(Party.id): description("Assigned party");
    
    // Required fields with validation
    bool: inProgress: required: description("Quest progress status");
    string: title: required: description("Quest title");
    string: description: required: description("Quest details");
}

// Struct with array constraints
struct Species {
    string: name: required: description("Species name");
    array<Ability>: abilities: required: unique_items: min_items(1): description("Species abilities");
}

// Generator-specific inclusion
struct Profile: gens_disabled(MySQL) {
    string: name: required: description("Profile name");
    float: temperature: required: description("Temperature setting");
    EndpointType: endpoint_type: required: description("API endpoint type");
}
```
