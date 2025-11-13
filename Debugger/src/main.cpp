#include <SchemaLangDebugger.hpp>
#include <ProgramStructure.hpp>
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    ProgramStructure ps;
    SchemaLangDebugger debugger(&ps);
    
    std::cout << "SchemaLang Interactive Debugger" << std::endl;
    std::cout << "================================" << std::endl;
    
    if (argc > 1) {
        std::string schema_file = argv[1];
        
        // Validate file exists before attempting to load
        std::ifstream file_check(schema_file);
        if (!file_check.is_open()) {
            std::cerr << "Error: File does not exist or cannot be opened: " << schema_file << std::endl;
            return 1;
        }
        file_check.close();
        
        debugger.start(schema_file);
    } else {
        std::cout << "\nUsage: " << argv[0] << " [schema_file]" << std::endl;
        std::cout << "You can also use the 'run' command to load a file." << std::endl;
        std::cout << std::endl;
    }
    
    debugger.run();
    
    return 0;
}
