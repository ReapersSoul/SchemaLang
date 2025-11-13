#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define private public
#define protected public
#include <SchemaLangDebugger.hpp>
#undef private
#undef protected

namespace {

std::filesystem::path schema_root() {
#ifndef ANYRPG_SCHEMAS_DIR
#error "ANYRPG_SCHEMAS_DIR must be defined"
#endif
    return std::filesystem::path(ANYRPG_SCHEMAS_DIR);
}

std::filesystem::path schema_path(const std::string& filename) {
    auto path = schema_root() / filename;
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Schema file not found: " + path.string());
    }
    return path;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int find_line_containing(const std::filesystem::path& path, const std::string& needle) {
    std::ifstream file(path);
    std::string line;
    int line_number = 0;
    while (std::getline(file, line)) {
        ++line_number;
        if (line.find(needle) != std::string::npos) {
            return line_number;
        }
    }
    throw std::runtime_error("Unable to find needle '" + needle + "' in " + path.string());
}

int find_token_index(const std::vector<Token>& tokens, const std::string& value) {
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].value == value) {
            return static_cast<int>(i);
        }
    }
    throw std::runtime_error("Token not found: " + value);
}

} // namespace

TEST(SchemaLangDebuggerTests, LineBreakpointTriggersWhenContextMatches) {
    ProgramStructure ps;
    SchemaLangDebugger debugger(&ps);

    const auto path = schema_path("Ability.schema");
    const int struct_line = find_line_containing(path, "struct Ability{");
    const auto location = path.string() + ":" + std::to_string(struct_line);

    const int breakpoint_id = debugger.addBreakpoint(BreakpointType::LINE, location);
    ASSERT_GT(breakpoint_id, 0);

    debugger.context.current_file = path.string();
    debugger.context.current_line = struct_line;

    EXPECT_TRUE(debugger.checkBreakpoint());
    EXPECT_EQ(1, debugger.breakpoints.at(breakpoint_id).hit_count);
}

TEST(SchemaLangDebuggerTests, DisabledBreakpointDoesNotTrigger) {
    ProgramStructure ps;
    SchemaLangDebugger debugger(&ps);

    const auto path = schema_path("Ability.schema");
    const int line = find_line_containing(path, "string: description:");
    const std::string location = path.string() + ":" + std::to_string(line);

    const int breakpoint_id = debugger.addBreakpoint(BreakpointType::LINE, location);
    ASSERT_GT(breakpoint_id, 0);

    debugger.enableBreakpoint(breakpoint_id, false);

    debugger.context.current_file = path.string();
    debugger.context.current_line = line;

    EXPECT_FALSE(debugger.checkBreakpoint());
    EXPECT_EQ(0, debugger.breakpoints.at(breakpoint_id).hit_count);
}

TEST(SchemaLangDebuggerTests, TokenBreakpointTriggersOnMatchingToken) {
    ProgramStructure ps;
    SchemaLangDebugger debugger(&ps);

    const auto path = schema_path("Spell.schema");
    const auto file_contents = read_file(path);
    const auto tokens = ps.tokenizeWithPosition(file_contents, path.string());

    const int breakpoint_id = debugger.addBreakpoint(BreakpointType::TOKEN, "struct");
    ASSERT_GT(breakpoint_id, 0);

    debugger.context.current_file = path.string();
    debugger.context.current_tokens = tokens;
    debugger.context.token_index = find_token_index(tokens, "struct");

    EXPECT_TRUE(debugger.checkBreakpoint());
    EXPECT_EQ(1, debugger.breakpoints.at(breakpoint_id).hit_count);
}

TEST(SchemaLangDebuggerTests, WatchpointDetectsValueChanges) {
    ProgramStructure ps;
    SchemaLangDebugger debugger(&ps);

    const int watch_id = debugger.addWatchpoint("token");
    ASSERT_GT(watch_id, 0);

    const auto path = schema_path("Memory.schema");
    debugger.context.current_tokens = ps.tokenizeWithPosition(read_file(path), path.string());
    debugger.context.token_index = find_token_index(debugger.context.current_tokens, "include");
    debugger.should_break = false;

    debugger.checkWatchpoints();
    EXPECT_TRUE(debugger.should_break);
    EXPECT_EQ(debugger.context.current_tokens.at(0).value, debugger.watchpoints.at(watch_id).last_value);

    debugger.should_break = false;
    debugger.checkWatchpoints();
    EXPECT_FALSE(debugger.should_break);
    EXPECT_EQ(debugger.context.current_tokens.at(0).value, debugger.watchpoints.at(watch_id).last_value);

    debugger.context.current_tokens[0].value = "updated_token";
    debugger.should_break = false;
    debugger.checkWatchpoints();
    EXPECT_TRUE(debugger.should_break);
    EXPECT_EQ("updated_token", debugger.watchpoints.at(watch_id).last_value);
}

TEST(SchemaLangDebuggerTests, EvaluateExpressionReturnsContextValues) {
    ProgramStructure ps;
    SchemaLangDebugger debugger(&ps);

    const auto path = schema_path("Character.schema");
    const int line = find_line_containing(path, "struct Character{");

    debugger.context.current_file = path.string();
    debugger.context.current_line = line;
    debugger.context.current_column = 1;
    debugger.context.current_tokens = ps.tokenizeWithPosition(read_file(path), path.string());
    debugger.context.token_index = 0;

    EXPECT_EQ(path.string(), debugger.evaluateExpression("file"));
    EXPECT_EQ(std::to_string(line), debugger.evaluateExpression("line"));
    EXPECT_EQ("1", debugger.evaluateExpression("column"));
    EXPECT_EQ(debugger.context.current_tokens.at(0).value, debugger.evaluateExpression("token"));
    EXPECT_EQ("0", debugger.evaluateExpression("token_index"));
}

TEST(SchemaLangDebuggerTests, ProcessCommandBreakLineAddsBreakpoint) {
    ProgramStructure ps;
    SchemaLangDebugger debugger(&ps);

    const auto path = schema_path("Ability.schema");
    debugger.context.current_file = path.string();

    const std::size_t before_count = debugger.breakpoints.size();
    const int line = find_line_containing(path, "string: name:");
    const std::string command = "break line " + std::to_string(line);

    EXPECT_TRUE(debugger.processCommand(command));
    EXPECT_EQ(before_count + 1, debugger.breakpoints.size());

    const auto id = debugger.breakpoints.rbegin()->first;
    debugger.context.current_line = line;
    EXPECT_TRUE(debugger.checkBreakpoint());
    EXPECT_EQ(1, debugger.breakpoints.at(id).hit_count);
}
