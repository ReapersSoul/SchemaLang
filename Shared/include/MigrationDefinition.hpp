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
        j["fromVersion"]["major"] = fromVersion.major;
        j["fromVersion"]["minor"] = fromVersion.minor;
        j["fromVersion"]["patch"] = fromVersion.patch;
        j["toVersion"]["major"] = toVersion.major;
        j["toVersion"]["minor"] = toVersion.minor;
        j["toVersion"]["patch"] = toVersion.patch;
        
        inja::json ops = inja::json::array();
        for (const auto& op : operations) {
            inja::json opJson;
            
            // Serialize operation type
            switch (op.type) {
                case MigrationOperation::Type::AddField:
                    opJson["type"] = "AddField";
                    break;
                case MigrationOperation::Type::RemoveField:
                    opJson["type"] = "RemoveField";
                    break;
                case MigrationOperation::Type::RenameField:
                    opJson["type"] = "RenameField";
                    break;
                case MigrationOperation::Type::ChangeType:
                    opJson["type"] = "ChangeType";
                    break;
                case MigrationOperation::Type::ChangeModifier:
                    opJson["type"] = "ChangeModifier";
                    break;
                case MigrationOperation::Type::SetModifier:
                    opJson["type"] = "SetModifier";
                    break;
                case MigrationOperation::Type::RemoveModifier:
                    opJson["type"] = "RemoveModifier";
                    break;
                case MigrationOperation::Type::SetDescription:
                    opJson["type"] = "SetDescription";
                    break;
            }
            
            opJson["fieldName"] = op.fieldName;
            
            if (op.newFieldName.has_value()) {
                opJson["newFieldName"] = op.newFieldName.value();
            }
            if (op.oldType.has_value()) {
                opJson["oldType"] = op.oldType.value();
            }
            if (op.newType.has_value()) {
                opJson["newType"] = op.newType.value();
            }
            if (op.modifierName.has_value()) {
                opJson["modifierName"] = op.modifierName.value();
            }
            if (op.oldModifierValue.has_value()) {
                opJson["oldModifierValue"] = op.oldModifierValue.value();
            }
            if (op.newModifierValue.has_value()) {
                opJson["newModifierValue"] = op.newModifierValue.value();
            }
            if (op.description.has_value()) {
                opJson["description"] = op.description.value();
            }
            
            ops.push_back(opJson);
        }
        j["operations"] = ops;
        
        return j;
    }
};