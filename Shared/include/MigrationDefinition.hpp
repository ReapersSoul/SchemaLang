#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <inja/inja.hpp>
#include <Debug.hpp>

struct MigrationOperation {
    enum class Type {
        AddField,
        RemoveField,
        RenameField,
        ChangeType,
        ChangeModifier,
        SetModifier,
        RemoveModifier,
        SetDescription
        // Add more as needed
    };

    Type type;
    std::string fieldName;
    std::optional<std::string> newFieldName; // For renames
    std::optional<std::string> oldType;
    std::optional<std::string> newType;
    std::optional<std::string> modifierName;
    std::optional<std::string> oldModifierValue;
    std::optional<std::string> newModifierValue;
    std::optional<std::string> description;
};

struct MigrationDefinition {
    std::string structName;
    Version fromVersion;
    Version toVersion;
    std::vector<MigrationOperation> operations;

    inja::json to_json() const {
        inja::json j;
        j["structName"] = structName;
        j["fromVersion"] = fromVersion;
        j["toVersion"] = toVersion;
        // Serialize operations as needed
        return j;
    }
};