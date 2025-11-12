#define SCHEMALANG_DEBUG
#include "SchemaLangDebugger.hpp"
#include <ProgramStructure.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    ProgramStructure ps;
    SchemaLangDebugger debugger(&ps);
    
    std::cout << "SchemaLang Interactive Debugger" << std::endl;
    std::cout << "================================" << std::endl;
    
    if (argc > 1) {
        std::string schema_file = argv[1];
        debugger.start(schema_file);
    } else {
        std::cout << "\nUsage: " << argv[0] << " [schema_file]" << std::endl;
        std::cout << "You can also use the 'run' command to load a file." << std::endl;
        std::cout << std::endl;
    }
    
    debugger.run();
    
    return 0;
}
