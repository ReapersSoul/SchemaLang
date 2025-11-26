#pragma once
#include <string>

struct SourcePosition
{
	std::string file_path;
	int line;
	int column;
	
	SourcePosition() : line(1), column(1) {}
	SourcePosition(const std::string& file, int l, int c) : file_path(file), line(l), column(c) {}
};

struct Version
{
    int major = -1;
    int minor = -1;
    int patch = -1;
};

struct VersionWithPosition
{
    int major = -1;
    int minor = -1;
    int patch = -1;
    SourcePosition position;
};

struct Token
{
	std::string value;
	SourcePosition position;
	
	Token() {}
	Token(const std::string& val, const SourcePosition& pos) : value(val), position(pos) {}
	
	// Comparison operators for convenience
	bool operator==(const std::string& str) const { return value == str; }
	bool operator!=(const std::string& str) const { return value != str; }
	bool operator==(const char* str) const { return value == str; }
	bool operator!=(const char* str) const { return value != str; }
	
	// Implicit conversion to string for compatibility
	operator const std::string&() const { return value; }
};

enum class BreakpointType {
    LINE,           // Break at specific line
    TOKEN,          // Break at specific token
    STRUCT_DEF,     // Break when parsing struct definition
    ENUM_DEF,       // Break when parsing enum definition
    MEMBER_VAR,     // Break when parsing member variable
    VALIDATION,     // Break before validation
    FILE_LOAD       // Break when loading a file
};

struct Breakpoint {
    int id;
    BreakpointType type;
    bool enabled;
    std::string condition;  // Optional condition (e.g., "token == 'struct'")
    
    // Type-specific data
    std::string file_path;  // For LINE and FILE_LOAD
    int line_number;        // For LINE
    std::string token_value; // For TOKEN
    std::string struct_name; // For STRUCT_DEF
    std::string enum_name;   // For ENUM_DEF
    
    int hit_count;
    int ignore_count;       // Ignore first N hits
    
    Breakpoint() : id(0), type(BreakpointType::LINE), enabled(true), line_number(-1), hit_count(0), ignore_count(0) {}
};

struct WatchPoint {
    int id;
    std::string expression;
    std::string last_value;
    bool enabled;
    
    WatchPoint() : id(0), enabled(true) {}
};

enum class StepMode {
    NONE,
    STEP_TOKEN,     // Step through each token
    STEP_LINE,      // Step through each line
    STEP_PARSE,     // Step through parse operations (struct, enum, member)
    CONTINUE,       // Continue until breakpoint
    FINISH          // Finish current operation
};