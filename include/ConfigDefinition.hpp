#pragma once
#include <string>
#include <set>

class ConfigDefinition
{
private:
    std::string output_directory; // Directory where generated files will be saved
    std::string input_file;        // Path to the input file containing the schema definitions
    std::set<std::string> extentions; // Set of extensions to be used for the generated files
    std::set<std::string> additional_generators; // Set of additional generators to be used
    bool verbose;                 // Flag to enable verbose outpu
    bool enable_exponential_operations; // Flag to enable exponential operations
    bool select_all_files; // Flag to generate select all files
    bool select_files;     // Flag to generate select files
    bool insert_files;     // Flag to generate insert files
    bool update_files;     // Flag to generate update files
    bool delete_files;     // Flag to generate delete files
public:
    // Getters
    const std::string& getOutputDirectory() const { return output_directory; }
    const std::string& getInputFile() const { return input_file; }
    bool getVerbose() const { return verbose; }
    bool getGenerateAll() const { return generate_all; }

    // Setters
    void setOutputDirectory(const std::string& dir) { output_directory = dir; }
    void setInputFile(const std::string& file) { input_file = file; }
    void setVerbose(bool v) { verbose = v; }
    void setGenerateAll(bool gen) { generate_all = gen; }
};

#include <stack>
class ConfigManager
{
    static std::stack<std::string> config_stack; // Stack to manage nested configurations
public:
    void pushConfig(const std::string& config) {
        config_stack.push(config);
    }

    void popConfig() {
        if (!config_stack.empty()) {
            config_stack.pop();
        }
    }

    std::string currentConfig() const {
        return config_stack.empty() ? "" : config_stack.top();
    }

    bool isEmpty() const {
        return config_stack.empty();
    }
};