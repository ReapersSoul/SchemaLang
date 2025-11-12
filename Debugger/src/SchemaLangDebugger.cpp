#include "SchemaLangDebugger.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <termios.h>
#include <unistd.h>

// Terminal handling for arrow keys
static struct termios orig_termios;

static void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static std::string readLineWithHistory(const std::string& prompt, std::vector<std::string>& history, int& history_index) {
    std::cout << prompt << std::flush;
    
    enableRawMode();
    
    std::string line;
    std::string temp_line;  // Store current line when browsing history
    int cursor_pos = 0;
    
    while (true) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) break;
        
        if (c == '\x1b') {  // Escape sequence
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) break;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) break;
            
            if (seq[0] == '[') {
                if (seq[1] == 'A') {  // Up arrow
                    if (history.empty()) continue;
                    
                    // Save current line when starting to browse history
                    if (history_index == -1) {
                        temp_line = line;
                        history_index = history.size() - 1;
                    } else if (history_index > 0) {
                        history_index--;
                    } else {
                        continue;  // Already at oldest
                    }
                    
                    // Clear current line
                    while (cursor_pos > 0) {
                        std::cout << "\b \b" << std::flush;
                        cursor_pos--;
                    }
                    while (cursor_pos < (int)line.length()) {
                        std::cout << " " << std::flush;
                        cursor_pos++;
                    }
                    while (cursor_pos > 0) {
                        std::cout << "\b" << std::flush;
                        cursor_pos--;
                    }
                    
                    // Display history item
                    line = history[history_index];
                    std::cout << line << std::flush;
                    cursor_pos = line.length();
                    
                } else if (seq[1] == 'B') {  // Down arrow
                    if (history_index == -1) continue;  // Not browsing history
                    
                    history_index++;
                    
                    // Clear current line
                    while (cursor_pos > 0) {
                        std::cout << "\b \b" << std::flush;
                        cursor_pos--;
                    }
                    while (cursor_pos < (int)line.length()) {
                        std::cout << " " << std::flush;
                        cursor_pos++;
                    }
                    while (cursor_pos > 0) {
                        std::cout << "\b" << std::flush;
                        cursor_pos--;
                    }
                    
                    if (history_index >= (int)history.size()) {
                        // Restore temp line
                        line = temp_line;
                        history_index = -1;
                    } else {
                        line = history[history_index];
                    }
                    
                    std::cout << line << std::flush;
                    cursor_pos = line.length();
                    
                } else if (seq[1] == 'C') {  // Right arrow
                    if (cursor_pos < (int)line.length()) {
                        std::cout << line[cursor_pos] << std::flush;
                        cursor_pos++;
                    }
                } else if (seq[1] == 'D') {  // Left arrow
                    if (cursor_pos > 0) {
                        std::cout << "\b" << std::flush;
                        cursor_pos--;
                    }
                }
            }
        } else if (c == 127 || c == 8) {  // Backspace
            if (cursor_pos > 0) {
                line.erase(cursor_pos - 1, 1);
                cursor_pos--;
                std::cout << "\b" << std::flush;
                for (int i = cursor_pos; i < (int)line.length(); i++) {
                    std::cout << line[i] << std::flush;
                }
                std::cout << " \b" << std::flush;
                for (int i = line.length(); i > cursor_pos; i--) {
                    std::cout << "\b" << std::flush;
                }
            }
        } else if (c == '\n' || c == '\r') {  // Enter
            std::cout << std::endl;
            disableRawMode();
            history_index = -1;  // Reset history browsing
            return line;
        } else if (c == 4) {  // Ctrl-D (EOF)
            std::cout << std::endl;
            disableRawMode();
            return "";
        } else if (c >= 32 && c < 127) {  // Printable characters
            line.insert(cursor_pos, 1, c);
            std::cout << c << std::flush;
            cursor_pos++;
            for (int i = cursor_pos; i < (int)line.length(); i++) {
                std::cout << line[i] << std::flush;
            }
            for (int i = line.length(); i > cursor_pos; i--) {
                std::cout << "\b" << std::flush;
            }
        }
    }
    
    disableRawMode();
    return line;
}

SchemaLangDebugger::SchemaLangDebugger(ProgramStructure* ps)
    : program_structure(ps)
    , next_breakpoint_id(1)
    , next_watchpoint_id(1)
    , step_mode(StepMode::NONE)
    , is_running(false)
    , should_break(false)
    , step_depth(0)
    , parsing_started(false)
    , history_index(-1)
    , verbose_mode(false)
    , trace_mode(false)
{
}

SchemaLangDebugger::~SchemaLangDebugger() {
}

void SchemaLangDebugger::run() {
    std::cout << "SchemaLang Debugger v1.0" << std::endl;
    std::cout << "Type 'help' for a list of commands" << std::endl;
    std::cout << "Use arrow keys to navigate command history" << std::endl;
    
    while (true) {
        std::string cmd = readLineWithHistory("(sldb) ", command_history, history_index);
        
        if (cmd.empty() && std::cin.eof()) {
            break;  // EOF
        }
        
        // Handle history recall with !
        if (!cmd.empty() && cmd[0] == '!') {
            if (cmd == "!!") {
                // Repeat last command
                if (command_history.empty()) {
                    std::cout << "No command history." << std::endl;
                    continue;
                }
                cmd = command_history.back();
                std::cout << cmd << std::endl;
            } else {
                // Recall by number (e.g., !5)
                try {
                    int num = std::stoi(cmd.substr(1));
                    if (num > 0 && num <= (int)command_history.size()) {
                        cmd = command_history[num - 1];
                        std::cout << cmd << std::endl;
                    } else {
                        std::cout << "History index out of range: " << num << std::endl;
                        continue;
                    }
                } catch (...) {
                    std::cout << "Invalid history syntax. Use !<number> or !!" << std::endl;
                    continue;
                }
            }
        }
        
        if (!cmd.empty()) {
            command_history.push_back(cmd);
        }
        
        if (cmd == "quit" || cmd == "q" || cmd == "exit") {
            break;
        }
        
        if (!cmd.empty() && !processCommand(cmd)) {
            std::cout << "Unknown command. Type 'help' for a list of commands." << std::endl;
        }
    }
}

void SchemaLangDebugger::start(const std::string& schema_file) {
    std::cout << "Loading schema file: " << schema_file << std::endl;
    is_running = false;  // Don't run automatically - wait for user command
    step_mode = StepMode::CONTINUE;  // Start in continue mode, not stepping
    parsing_started = false;
    pending_schema_file = schema_file;
    
    // Set up debugger connection
    if (program_structure) {
        program_structure->debugger = this;
        context.current_file = schema_file;
        
        // Don't parse yet - parsing will happen when user issues 'continue' or 'step' command
        std::cout << "File loaded. Use 'continue', 'step', or 'next' to begin parsing." << std::endl;
        std::cout << "Use 'break' to set breakpoints before starting." << std::endl;
    }
}

int SchemaLangDebugger::addBreakpoint(BreakpointType type, const std::string& location) {
    Breakpoint bp;
    bp.id = next_breakpoint_id++;
    bp.type = type;
    bp.enabled = true;
    
    // Parse location based on type
    switch (type) {
        case BreakpointType::LINE: {
            // Format: "file:line" or just "line" for current file
            size_t colon = location.find(':');
            if (colon != std::string::npos) {
                bp.file_path = location.substr(0, colon);
                bp.line_number = std::stoi(location.substr(colon + 1));
            } else {
                bp.file_path = context.current_file;
                bp.line_number = std::stoi(location);
            }
            break;
        }
        case BreakpointType::TOKEN:
            bp.token_value = location;
            break;
        case BreakpointType::STRUCT_DEF:
            bp.struct_name = location;
            break;
        case BreakpointType::ENUM_DEF:
            bp.enum_name = location;
            break;
        case BreakpointType::FILE_LOAD:
            bp.file_path = location;
            break;
        default:
            break;
    }
    
    breakpoints[bp.id] = bp;
    std::cout << "Breakpoint " << bp.id << " set." << std::endl;
    return bp.id;
}

bool SchemaLangDebugger::removeBreakpoint(int id) {
    auto it = breakpoints.find(id);
    if (it != breakpoints.end()) {
        breakpoints.erase(it);
        std::cout << "Breakpoint " << id << " removed." << std::endl;
        return true;
    }
    std::cout << "No breakpoint with id " << id << std::endl;
    return false;
}

bool SchemaLangDebugger::enableBreakpoint(int id, bool enable) {
    auto it = breakpoints.find(id);
    if (it != breakpoints.end()) {
        it->second.enabled = enable;
        std::cout << "Breakpoint " << id << (enable ? " enabled" : " disabled") << std::endl;
        return true;
    }
    return false;
}

void SchemaLangDebugger::listBreakpoints() {
    if (breakpoints.empty()) {
        std::cout << "No breakpoints set." << std::endl;
        return;
    }
    
    std::cout << "Breakpoints:" << std::endl;
    for (const auto& [id, bp] : breakpoints) {
        std::cout << "  " << id << ": ";
        if (!bp.enabled) std::cout << "[DISABLED] ";
        
        switch (bp.type) {
            case BreakpointType::LINE:
                std::cout << bp.file_path << ":" << bp.line_number;
                break;
            case BreakpointType::TOKEN:
                std::cout << "token '" << bp.token_value << "'";
                break;
            case BreakpointType::STRUCT_DEF:
                std::cout << "struct " << bp.struct_name;
                break;
            case BreakpointType::ENUM_DEF:
                std::cout << "enum " << bp.enum_name;
                break;
            case BreakpointType::MEMBER_VAR:
                std::cout << "member variable";
                break;
            case BreakpointType::VALIDATION:
                std::cout << "validation";
                break;
            case BreakpointType::FILE_LOAD:
                std::cout << "file load: " << bp.file_path;
                break;
        }
        
        std::cout << " (hits: " << bp.hit_count << ")";
        if (bp.ignore_count > 0) {
            std::cout << " (ignore: " << bp.ignore_count << ")";
        }
        if (!bp.condition.empty()) {
            std::cout << " if " << bp.condition;
        }
        std::cout << std::endl;
    }
}

bool SchemaLangDebugger::checkBreakpoint() {
    for (auto& [id, bp] : breakpoints) {
        if (!bp.enabled) continue;
        
        bool hit = false;
        
        switch (bp.type) {
            case BreakpointType::LINE:
                hit = (context.current_file == bp.file_path && 
                       context.current_line == bp.line_number);
                break;
            case BreakpointType::TOKEN:
                if (context.token_index >= 0 && 
                    context.token_index < (int)context.current_tokens.size()) {
                    hit = (context.current_tokens[context.token_index].value == bp.token_value);
                }
                break;
            case BreakpointType::STRUCT_DEF:
                if (context.current_struct) {
                    hit = (context.current_struct->getIdentifier() == bp.struct_name);
                }
                break;
            case BreakpointType::ENUM_DEF:
                if (context.current_enum) {
                    hit = (context.current_enum->identifier == bp.enum_name);
                }
                break;
            case BreakpointType::FILE_LOAD:
                hit = (context.current_file == bp.file_path);
                break;
            default:
                break;
        }
        
        if (hit) {
            bp.hit_count++;
            if (bp.hit_count <= bp.ignore_count) {
                continue;
            }
            
            // Check condition if present
            if (!bp.condition.empty()) {
                std::string result = evaluateExpression(bp.condition);
                if (result != "true" && result != "1") {
                    continue;
                }
            }
            
            std::cout << "\nBreakpoint " << id << " hit at ";
            std::cout << context.current_file << ":" << context.current_line << std::endl;
            return true;
        }
    }
    return false;
}

int SchemaLangDebugger::addWatchpoint(const std::string& expression) {
    WatchPoint wp;
    wp.id = next_watchpoint_id++;
    wp.expression = expression;
    wp.enabled = true;
    wp.last_value = evaluateExpression(expression);
    
    watchpoints[wp.id] = wp;
    std::cout << "Watchpoint " << wp.id << " set for: " << expression << std::endl;
    std::cout << "  Current value: " << wp.last_value << std::endl;
    return wp.id;
}

bool SchemaLangDebugger::removeWatchpoint(int id) {
    auto it = watchpoints.find(id);
    if (it != watchpoints.end()) {
        watchpoints.erase(it);
        std::cout << "Watchpoint " << id << " removed." << std::endl;
        return true;
    }
    return false;
}

void SchemaLangDebugger::checkWatchpoints() {
    for (auto& [id, wp] : watchpoints) {
        if (!wp.enabled) continue;
        
        std::string new_value = evaluateExpression(wp.expression);
        if (new_value != wp.last_value) {
            std::cout << "\nWatchpoint " << id << ": " << wp.expression << std::endl;
            std::cout << "  Old value: " << wp.last_value << std::endl;
            std::cout << "  New value: " << new_value << std::endl;
            wp.last_value = new_value;
            should_break = true;
        }
    }
}

void SchemaLangDebugger::listWatchpoints() {
    if (watchpoints.empty()) {
        std::cout << "No watchpoints set." << std::endl;
        return;
    }
    
    std::cout << "Watchpoints:" << std::endl;
    for (const auto& [id, wp] : watchpoints) {
        std::cout << "  " << id << ": ";
        if (!wp.enabled) std::cout << "[DISABLED] ";
        std::cout << wp.expression << " = " << wp.last_value << std::endl;
    }
}

void SchemaLangDebugger::stepToken() {
    step_mode = StepMode::STEP_TOKEN;
    is_running = true;
    
    // Start parsing if not already started
    if (!parsing_started && !pending_schema_file.empty()) {
        parsing_started = true;
        if (!program_structure->readFile(pending_schema_file)) {
            std::cout << "Error: Failed to parse schema file" << std::endl;
            is_running = false;
        }
    }
}

void SchemaLangDebugger::stepLine() {
    step_mode = StepMode::STEP_LINE;
    is_running = true;
    
    // Start parsing if not already started
    if (!parsing_started && !pending_schema_file.empty()) {
        parsing_started = true;
        if (!program_structure->readFile(pending_schema_file)) {
            std::cout << "Error: Failed to parse schema file" << std::endl;
            is_running = false;
        }
    }
}

void SchemaLangDebugger::stepParse() {
    step_mode = StepMode::STEP_PARSE;
    is_running = true;
    
    // Start parsing if not already started
    if (!parsing_started && !pending_schema_file.empty()) {
        parsing_started = true;
        if (!program_structure->readFile(pending_schema_file)) {
            std::cout << "Error: Failed to parse schema file" << std::endl;
            is_running = false;
        }
    }
}

void SchemaLangDebugger::stepOver() {
    step_depth = (int)context.parse_stack.size();
    step_mode = StepMode::STEP_PARSE;
    is_running = true;
    
    // Start parsing if not already started
    if (!parsing_started && !pending_schema_file.empty()) {
        parsing_started = true;
        if (!program_structure->readFile(pending_schema_file)) {
            std::cout << "Error: Failed to parse schema file" << std::endl;
            is_running = false;
        }
    }
}

void SchemaLangDebugger::stepOut() {
    step_depth = (int)context.parse_stack.size() - 1;
    step_mode = StepMode::STEP_PARSE;
    is_running = true;
    
    // Start parsing if not already started
    if (!parsing_started && !pending_schema_file.empty()) {
        parsing_started = true;
        if (!program_structure->readFile(pending_schema_file)) {
            std::cout << "Error: Failed to parse schema file" << std::endl;
            is_running = false;
        }
    }
}

void SchemaLangDebugger::continueExecution() {
    step_mode = StepMode::CONTINUE;
    is_running = true;
    
    // Start parsing if not already started
    if (!parsing_started && !pending_schema_file.empty()) {
        parsing_started = true;
        if (!program_structure->readFile(pending_schema_file)) {
            std::cout << "Error: Failed to parse schema file" << std::endl;
            is_running = false;
        }
    }
}

void SchemaLangDebugger::pause() {
    should_break = true;
}

void SchemaLangDebugger::printCurrentToken() {
    if (context.token_index >= 0 && context.token_index < (int)context.current_tokens.size()) {
        const Token& t = context.current_tokens[context.token_index];
        std::cout << "Current token: '" << t.value << "' at "
                  << t.position.file_path << ":"
                  << t.position.line << ":" << t.position.column << std::endl;
    } else {
        std::cout << "No current token." << std::endl;
    }
}

void SchemaLangDebugger::printTokens(int count) {
    if (context.current_tokens.empty()) {
        std::cout << "No tokens loaded." << std::endl;
        return;
    }
    
    int start = std::max(0, context.token_index - count / 2);
    int end = std::min((int)context.current_tokens.size(), start + count);
    
    printTokenRange(start, end);
}

void SchemaLangDebugger::printTokenRange(int start, int end) {
    for (int i = start; i < end; i++) {
        std::cout << (i == context.token_index ? "=> " : "   ");
        std::cout << std::setw(4) << i << ": ";
        std::cout << tokenPreview(context.current_tokens[i]) << std::endl;
    }
}

void SchemaLangDebugger::printStack() {
    if (context.parse_stack.empty()) {
        std::cout << "Parse stack is empty." << std::endl;
        return;
    }
    
    std::cout << "Parse stack:" << std::endl;
    for (int i = (int)context.parse_stack.size() - 1; i >= 0; i--) {
        std::cout << "  #" << i << " " << context.parse_stack[i] << std::endl;
    }
}

void SchemaLangDebugger::printStructs() {
    if (!program_structure) {
        std::cout << "No program structure loaded." << std::endl;
        return;
    }
    
    auto& structs = program_structure->getStructs();
    if (structs.empty()) {
        std::cout << "No structs defined yet." << std::endl;
        return;
    }
    
    std::cout << "Defined structs (" << structs.size() << "):" << std::endl;
    for (auto& s : structs) {
        std::cout << "  struct " << s.getIdentifier();
        if (!s.getMemberVariables().empty()) {
            std::cout << " { " << s.getMemberVariables().size() << " members }";
        }
        std::cout << std::endl;
    }
}

void SchemaLangDebugger::printEnums() {
    if (!program_structure) {
        std::cout << "No program structure loaded." << std::endl;
        return;
    }
    
    auto& enums = program_structure->getEnums();
    if (enums.empty()) {
        std::cout << "No enums defined yet." << std::endl;
        return;
    }
    
    std::cout << "Defined enums (" << enums.size() << "):" << std::endl;
    for (const auto& e : enums) {
        std::cout << "  enum " << e.identifier;
        if (!e.values.empty()) {
            std::cout << " { " << e.values.size() << " values }";
        }
        std::cout << std::endl;
    }
}

void SchemaLangDebugger::printCurrentContext() {
    std::cout << "\n=== Current Context ===" << std::endl;
    std::cout << "File: " << context.current_file << std::endl;
    std::cout << "Line: " << context.current_line << ", Column: " << context.current_column << std::endl;
    std::cout << "Token Index: " << context.token_index;
    if (context.token_index >= 0 && context.token_index < (int)context.current_tokens.size()) {
        std::cout << " / " << context.current_tokens.size();
    }
    std::cout << std::endl;
    std::cout << "Operation: " << context.current_operation << std::endl;
    
    if (context.current_struct) {
        std::cout << "Current Struct: " << context.current_struct->getIdentifier() << std::endl;
    }
    if (context.current_enum) {
        std::cout << "Current Enum: " << context.current_enum->identifier << std::endl;
    }
    if (context.current_member) {
        std::cout << "Current Member: " << context.current_member->identifier << std::endl;
    }
    
    std::cout << "Parse Stack Depth: " << context.parse_stack.size() << std::endl;
    std::cout << "======================\n" << std::endl;
}

void SchemaLangDebugger::printSourceContext(int lines) {
    if (context.current_file.empty()) {
        std::cout << "No source file loaded." << std::endl;
        return;
    }
    
    std::ifstream file(context.current_file);
    if (!file.is_open()) {
        std::cout << "Could not open source file: " << context.current_file << std::endl;
        return;
    }
    
    std::vector<std::string> file_lines;
    std::string line;
    while (std::getline(file, line)) {
        file_lines.push_back(line);
    }
    
    int start_line = std::max(1, context.current_line - lines);
    int end_line = std::min((int)file_lines.size(), context.current_line + lines);
    
    std::cout << "\nSource context:" << std::endl;
    for (int i = start_line; i <= end_line; i++) {
        std::cout << (i == context.current_line ? "=> " : "   ");
        std::cout << std::setw(4) << i << " | " << file_lines[i - 1] << std::endl;
    }
    std::cout << std::endl;
}

void SchemaLangDebugger::printVariable(const std::string& name) {
    std::string value = evaluateExpression(name);
    std::cout << name << " = " << value << std::endl;
}

std::string SchemaLangDebugger::evaluateExpression(const std::string& expr) {
    // Simple expression evaluator
    if (expr == "file" || expr == "current_file") {
        return context.current_file;
    }
    if (expr == "line" || expr == "current_line") {
        return std::to_string(context.current_line);
    }
    if (expr == "column" || expr == "current_column") {
        return std::to_string(context.current_column);
    }
    if (expr == "token" || expr == "current_token") {
        if (context.token_index >= 0 && context.token_index < (int)context.current_tokens.size()) {
            return context.current_tokens[context.token_index].value;
        }
        return "<none>";
    }
    if (expr == "token_index") {
        return std::to_string(context.token_index);
    }
    if (expr == "operation") {
        return context.current_operation;
    }
    if (expr == "struct_count" && program_structure) {
        return std::to_string(program_structure->getStructs().size());
    }
    if (expr == "enum_count" && program_structure) {
        return std::to_string(program_structure->getEnums().size());
    }
    
    return "<unknown>";
}

void SchemaLangDebugger::onTokenParsed(const Token& token) {
    // Update current token only if it's different or if we're on a new file
    if (context.current_file != token.position.file_path) {
        context.current_tokens.clear();
        token_history.clear();
        context.token_index = -1;
    }
    
    // Add token to context if not already there or if position differs
    bool token_exists = false;
    for (size_t i = 0; i < context.current_tokens.size(); i++) {
        if (context.current_tokens[i].position.line == token.position.line &&
            context.current_tokens[i].position.column == token.position.column &&
            context.current_tokens[i].value == token.value) {
            context.token_index = i;
            token_exists = true;
            break;
        }
    }
    
    if (!token_exists) {
        context.current_tokens.push_back(token);
        context.token_index = (int)context.current_tokens.size() - 1;
        token_history.push_back(token);
    }
    
    context.current_line = token.position.line;
    context.current_column = token.position.column;
    context.current_file = token.position.file_path;
    
    if (trace_mode) {
        std::cout << "[TRACE] Token[" << context.token_index << "]: " << tokenPreview(token) << std::endl;
    }
    
    checkWatchpoints();
    
    if (step_mode == StepMode::STEP_TOKEN || checkBreakpoint() || should_break) {
        should_break = false;
        printCurrentToken();
        printSourceContext(2);
        waitForUser();
    }
}

void SchemaLangDebugger::onLineChanged(const SourcePosition& pos) {
    context.current_line = pos.line;
    context.current_column = pos.column;
    context.current_file = pos.file_path;
    
    // Check breakpoints even in CONTINUE mode
    if (step_mode == StepMode::STEP_LINE || checkBreakpoint() || should_break) {
        should_break = false;
        printSourceContext(3);
        waitForUser();
    }
}

void SchemaLangDebugger::onParseOperation(const std::string& operation) {
    context.current_operation = operation;
    pushParseStack(operation);
    
    if (trace_mode) {
        std::cout << "[TRACE] Parse[" << context.parse_stack.size() << "]: " << operation << std::endl;
    }
    
    if (step_mode == StepMode::STEP_PARSE || checkBreakpoint() || should_break) {
        should_break = false;
        std::cout << "\nParsing: " << operation << std::endl;
        printCurrentContext();
        printStack();
        waitForUser();
    }
}

void SchemaLangDebugger::onStructParsing(StructDefinition* s) {
    context.current_struct = s;
    onParseOperation("parsing struct: " + s->getIdentifier());
}

void SchemaLangDebugger::onEnumParsing(EnumDefinition* e) {
    context.current_enum = e;
    onParseOperation("parsing enum: " + e->identifier);
}

void SchemaLangDebugger::onMemberParsing(MemberVariableDefinition* m) {
    context.current_member = m;
    onParseOperation("parsing member: " + m->identifier);
}

void SchemaLangDebugger::onFileLoaded(const std::string& file_path) {
    context.current_file = file_path;
    context.current_line = 1;
    context.current_column = 1;
    context.current_tokens.clear();  // Reset tokens for new file
    context.token_index = -1;
    context.parse_stack.clear();  // Reset parse stack for new file
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Loaded file: " << file_path << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    if (checkBreakpoint()) {
        waitForUser();
    }
}

void SchemaLangDebugger::onValidation() {
    onParseOperation("validation");
    
    for (const auto& [id, bp] : breakpoints) {
        if (bp.enabled && bp.type == BreakpointType::VALIDATION) {
            std::cout << "\nBreakpoint " << id << " hit: validation" << std::endl;
            waitForUser();
            break;
        }
    }
}

void SchemaLangDebugger::onError(const std::string& message, const SourcePosition& pos) {
    std::cout << "\n*** ERROR ***" << std::endl;
    std::cout << "At " << pos.file_path << ":" << pos.line << ":" << pos.column << std::endl;
    std::cout << message << std::endl;
    
    context.current_file = pos.file_path;
    context.current_line = pos.line;
    context.current_column = pos.column;
    
    printSourceContext(3);
    printStack();
    
    std::cout << "\nEntering debugger..." << std::endl;
    waitForUser();
}

bool SchemaLangDebugger::processCommand(const std::string& cmd) {
    std::istringstream iss(cmd);
    std::string command;
    iss >> command;
    
    if (command == "help" || command == "h") {
        printHelp();
        return true;
    }
    
    // Breakpoint commands
    if (command == "break" || command == "b") {
        std::string type, location;
        iss >> type >> location;
        
        BreakpointType bpType = BreakpointType::LINE;
        if (type == "line" || type == "l") bpType = BreakpointType::LINE;
        else if (type == "token" || type == "t") bpType = BreakpointType::TOKEN;
        else if (type == "struct" || type == "s") bpType = BreakpointType::STRUCT_DEF;
        else if (type == "enum" || type == "e") bpType = BreakpointType::ENUM_DEF;
        else if (type == "file" || type == "f") bpType = BreakpointType::FILE_LOAD;
        else if (type == "validation" || type == "v") bpType = BreakpointType::VALIDATION;
        else {
            // Assume it's a line number in current file
            location = type;
            bpType = BreakpointType::LINE;
        }
        
        addBreakpoint(bpType, location);
        return true;
    }
    
    if (command == "delete" || command == "d") {
        int id;
        iss >> id;
        removeBreakpoint(id);
        return true;
    }
    
    if (command == "enable") {
        int id;
        iss >> id;
        enableBreakpoint(id, true);
        return true;
    }
    
    if (command == "disable") {
        int id;
        iss >> id;
        enableBreakpoint(id, false);
        return true;
    }
    
    if (command == "info" || command == "i") {
        std::string what;
        iss >> what;
        
        if (what == "breakpoints" || what == "b") {
            listBreakpoints();
        } else if (what == "watchpoints" || what == "w") {
            listWatchpoints();
        } else if (what == "stack" || what == "s") {
            printStack();
        } else if (what == "structs") {
            printStructs();
        } else if (what == "enums") {
            printEnums();
        } else {
            printCurrentContext();
        }
        return true;
    }
    
    // Watch commands
    if (command == "watch" || command == "w") {
        std::string expr;
        std::getline(iss, expr);
        if (!expr.empty() && expr[0] == ' ') expr = expr.substr(1);
        addWatchpoint(expr);
        return true;
    }
    
    // Execution control
    if (command == "step" || command == "s") {
        stepToken();
        return true;
    }
    
    if (command == "next" || command == "n") {
        stepLine();
        return true;
    }
    
    if (command == "parse" || command == "p") {
        stepParse();
        return true;
    }
    
    if (command == "continue" || command == "c") {
        continueExecution();
        return true;
    }
    
    // Display commands
    if (command == "list" || command == "l") {
        int lines = 10;
        iss >> lines;
        printSourceContext(lines);
        return true;
    }
    
    if (command == "tokens") {
        int count = 20;
        iss >> count;
        printTokens(count);
        return true;
    }
    
    if (command == "print") {
        std::string var;
        std::getline(iss, var);
        if (!var.empty() && var[0] == ' ') var = var.substr(1);
        printVariable(var);
        return true;
    }
    
    if (command == "where" || command == "backtrace" || command == "bt") {
        printStack();
        return true;
    }
    
    if (command == "context") {
        printCurrentContext();
        return true;
    }
    
    if (command == "history") {
        int count = 20;
        iss >> count;
        printHistory(count);
        return true;
    }
    
    // Settings
    if (command == "set") {
        std::string var, value;
        iss >> var >> value;
        
        if (var == "verbose") {
            verbose_mode = (value == "on" || value == "true" || value == "1");
            std::cout << "Verbose mode " << (verbose_mode ? "enabled" : "disabled") << std::endl;
        } else if (var == "trace") {
            trace_mode = (value == "on" || value == "true" || value == "1");
            std::cout << "Trace mode " << (trace_mode ? "enabled" : "disabled") << std::endl;
        }
        return true;
    }
    
    if (command == "run" || command == "r") {
        std::string file;
        std::getline(iss, file);
        if (!file.empty() && file[0] == ' ') file = file.substr(1);
        if (!file.empty()) {
            start(file);
        }
        return true;
    }
    
    return false;
}

void SchemaLangDebugger::printHelp() {
    std::cout << R"(
SchemaLang Debugger Commands:

BREAKPOINTS:
  break [type] <location>     Set breakpoint (b)
    Types: line, token, struct, enum, file, validation
    Examples:
      break line 42             Break at line 42 in current file
      break token struct        Break when token 'struct' is parsed
      break struct Character    Break when parsing struct Character
      break file main.schema    Break when loading file
  delete <id>                  Delete breakpoint (d)
  enable <id>                  Enable breakpoint
  disable <id>                 Disable breakpoint
  info breakpoints             List all breakpoints (info b)

WATCHPOINTS:
  watch <expression>           Set watchpoint (w)
    Examples:
      watch token               Watch current token value
      watch struct_count        Watch number of structs
  info watchpoints             List all watchpoints (info w)

EXECUTION:
  step                         Step to next token (s)
  next                         Step to next line (n)
  parse                        Step to next parse operation (p)
  continue                     Continue execution (c)
  run <file>                   Load and debug schema file (r)

INSPECTION:
  list [lines]                 Show source context (l)
  tokens [count]               Show tokens around current position
  print <expression>           Print expression value
  info stack                   Show parse stack (info s)
  info structs                 Show defined structs
  info enums                   Show defined enums
  context                      Show complete current context
  where                        Show parse stack (backtrace, bt)
  history [count]              Show command history (default 20)
  !<number>                    Recall command by history number (e.g., !5)
  !!                           Repeat last command

SETTINGS:
  set verbose on|off           Enable/disable verbose output
  set trace on|off             Enable/disable trace mode

OTHER:
  help                         Show this help (h)
  quit                         Exit debugger (q, exit)

)" << std::endl;
}

void SchemaLangDebugger::waitForUser() {
    is_running = false;
    
    // Enter a nested command loop - process commands until user continues execution
    while (!is_running) {
        std::string cmd = readLineWithHistory("(sldb) ", command_history, history_index);
        
        if (cmd.empty() && std::cin.eof()) {
            is_running = true;  // Exit on EOF
            break;
        }
        
        if (!cmd.empty() && !processCommand(cmd)) {
            std::cout << "Unknown command. Type 'help' for a list of commands." << std::endl;
        }
    }
}

bool SchemaLangDebugger::shouldBreakNow() {
    return should_break || checkBreakpoint();
}

void SchemaLangDebugger::displayPrompt() {
    // Prompt is displayed by readline
}

void SchemaLangDebugger::pushParseStack(const std::string& operation) {
    context.parse_stack.push_back(operation);
}

void SchemaLangDebugger::popParseStack() {
    if (!context.parse_stack.empty()) {
        context.parse_stack.pop_back();
    }
}

std::string SchemaLangDebugger::tokenPreview(const Token& t) {
    std::stringstream ss;
    ss << "'" << t.value << "' ";
    ss << "(" << t.position.file_path << ":" 
       << t.position.line << ":" << t.position.column << ")";
    return ss.str();
}

void SchemaLangDebugger::printHistory(int count) {
    if (command_history.empty()) {
        std::cout << "No command history." << std::endl;
        return;
    }
    
    int start = std::max(0, (int)command_history.size() - count);
    std::cout << "Command history:" << std::endl;
    for (int i = start; i < (int)command_history.size(); i++) {
        std::cout << "  " << std::setw(4) << (i + 1) << ": " << command_history[i] << std::endl;
    }
}
