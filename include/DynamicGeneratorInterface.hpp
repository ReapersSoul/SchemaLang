#pragma once
#include <Generator/Generator.hpp>
#include <ArgParser/ArgParser.hpp>
#include <string>
#include <vector>

// Simple structs for passing flag/parameter information across library boundaries
struct FlagInfo {
    std::string name;
    bool required;
    int priority;
    std::string description;
};

struct ParameterInfo {
    std::string name;
    bool required;
    int priority;
    std::string description;
};

// Example interface for dynamic generators
// Dynamic generator libraries should implement this interface
// and export the required functions

extern "C" {
    // This function should be exported by dynamic generator libraries
    // It should return a pointer to a Generator instance
    Generator* getGeneratorInstance();
    
    // Required: to identify the generator and determine output directory name
    const char* getGeneratorName();
    
    // Optional: called to register flags and parameters with the argument parser
    // This allows dynamic generators to add their own command-line options
    void registerArguments(argumentParser* parser);
}

// Example implementation (this would be in a separate .cpp file for the dynamic library):
/*
#include "DynamicGeneratorInterface.hpp"
#include "YourCustomGenerator.hpp"

static YourCustomGenerator* g_generator = nullptr;
static bool g_customFlag = false;
static std::string g_customParam = "";

extern "C" {
    Generator* getGeneratorInstance() {
        if (!g_generator) {
            g_generator = new YourCustomGenerator();
        }
        return g_generator;
    }
    
    const char* getGeneratorName() {
        return "YourCustomGenerator";
    }
    
    void registerArguments(argumentParser* parser) {
        // Create flags and parameters for this generator
        Flag* customFlag = new Flag("customFlag", false, []() {
            g_customFlag = true;
            // Handle custom flag logic here
        }, 1);
        
        Parameter* customParam = new Parameter("customParam", false, [](std::string value) {
            g_customParam = value;
            // Handle custom parameter logic here
        }, 1);
        
        // Register them with the parser
        parser->addFlag(customFlag);
        parser->addParameter(customParam);
    }
}
*/
