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