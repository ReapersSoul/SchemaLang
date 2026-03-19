#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <memory>
#include <ProgramStructure.hpp>
#include <StructDefinition.hpp>
#include <EnumDefinition.hpp>



struct DebugContext {
    // Current execution state
    std::string current_file;
    int current_line;
    int current_column;
    int token_index;
    std::vector<Token> current_tokens;
    
    // Parse stack
    std::vector<std::string> parse_stack;
    
    // Current parsing state
    std::string current_operation;  // e.g., "parsing struct", "reading member variable"
    StructDefinition* current_struct;
    EnumDefinition* current_enum;
    MemberVariableDefinition* current_member;
    
    DebugContext() : current_line(0), current_column(0), token_index(-1), 
                     current_struct(nullptr), current_enum(nullptr), current_member(nullptr) {}
};

class SchemaLangDebugger {
private:
    std::shared_ptr<ProgramStructure> program_structure;
    DebugContext context;
    
    // Breakpoint and watchpoint management
    std::map<int, Breakpoint> breakpoints;
    std::map<int, WatchPoint> watchpoints;
    int next_breakpoint_id;
    int next_watchpoint_id;
    
    // Execution control
    StepMode step_mode;
    bool is_running;
    bool should_break;
    int step_depth;  // For step-over/step-out functionality
    bool parsing_started;  // Track if we've begun parsing
    std::string pending_schema_file;  // File to parse when debugging starts
    
    // History
    std::vector<std::string> command_history;
    std::vector<Token> token_history;
    int history_index;  // Current position in history (-1 = not browsing)
    
    // Output control
    bool verbose_mode;
    bool trace_mode;  // Print all operations
    
public:
    SchemaLangDebugger(std::shared_ptr<ProgramStructure> ps);
    ~SchemaLangDebugger();
    
    // Main control interface
    void run();
    void start(const std::string& schema_file);
    
    // Breakpoint management
    int addBreakpoint(BreakpointType type, const std::string& location);
    bool removeBreakpoint(int id);
    bool enableBreakpoint(int id, bool enable);
    void listBreakpoints();
    bool checkBreakpoint();
    
    // Watchpoint management
    int addWatchpoint(const std::string& expression);
    bool removeWatchpoint(int id);
    void checkWatchpoints();
    void listWatchpoints();
    
    // Execution control
    void stepToken();
    void stepLine();
    void stepParse();
    void stepOver();
    void stepOut();
    void continueExecution();
    void pause();
    
    // Inspection
    void printCurrentToken();
    void printTokens(int count = 10);
    void printStack();
    void printStructs();
    void printEnums();
    void printStructDetail(const std::string& name);
    void printEnumDetail(const std::string& name);
    void printMemberDetail(const std::string& struct_name, const std::string& member_name);
    void printAST();
    void printCurrentContext();
    void printSourceContext(int lines = 5);
    void printVariable(const std::string& name);
    
    // Evaluation
    std::string evaluateExpression(const std::string& expr);
    
    // Internal hooks (called from modified ProgramStructure)
    void onTokenParsed(const Token& token);
    void onLineChanged(const SourcePosition& pos);
    void onParseOperation(const std::string& operation);
    void onStructParsing(StructDefinition* s);
    void onEnumParsing(EnumDefinition* e);
    void onMemberParsing(MemberVariableDefinition* m);
    void onFileLoaded(const std::string& file_path);
    void onValidation();
    void onError(const std::string& message, const SourcePosition& pos);

    void endParseOperation();

    // Command processing
    bool processCommand(const std::string& cmd);
    void printHelp();
    void printHistory(int count = 20);
    
private:
    void waitForUser();
    bool shouldBreakNow();
    void displayPrompt();
    void pushParseStack(const std::string& operation);
    void popParseStack();
    std::string tokenPreview(const Token& t);
    void printTokenRange(int start, int end);
};

// Helper macros for instrumenting ProgramStructure
#define DEBUG_HOOK_TOKEN(debugger, token) if(debugger) debugger->onTokenParsed(token)
#define DEBUG_HOOK_LINE(debugger, pos) if(debugger) debugger->onLineChanged(pos)
#define DEBUG_HOOK_PARSE(debugger, op) if(debugger) debugger->onParseOperation(op)
#define DEBUG_HOOK_STRUCT(debugger, s) if(debugger) debugger->onStructParsing(s)
#define DEBUG_HOOK_ENUM(debugger, e) if(debugger) debugger->onEnumParsing(e)
#define DEBUG_HOOK_MEMBER(debugger, m) if(debugger) debugger->onMemberParsing(m)
#define DEBUG_HOOK_FILE(debugger, file) if(debugger) debugger->onFileLoaded(file)
#define DEBUG_HOOK_VALIDATION(debugger) if(debugger) debugger->onValidation()
#define DEBUG_HOOK_ERROR(debugger, msg, pos) if(debugger) debugger->onError(msg, pos)
