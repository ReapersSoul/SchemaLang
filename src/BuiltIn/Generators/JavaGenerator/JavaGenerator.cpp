#include <BuiltIn/Generators/JavaGenerator/JavaGenerator.hpp>

void JavaGenerator::generate_enum_file(EnumDefinition e, std::string out_path)
{
    std::ofstream enumFile(out_path + "/" + e.identifier + ".java");
    if (!enumFile.is_open())
    {
        std::cout << "Failed to open file: " << out_path + "/" + e.identifier + ".java" << std::endl;
        return;
    }
    
    enumFile << "public enum " << e.identifier << " {\n";
    
    for (size_t i = 0; i < e.values.size(); i++)
    {
        enumFile << "    " << e.values[i].first << "(" << e.values[i].second << ")";
        if (i < e.values.size() - 1)
        {
            enumFile << ",\n";
        }
        else
        {
            enumFile << ";\n";
        }
    }
    
    enumFile << "\n";
    enumFile << "    private final int value;\n\n";
    
    enumFile << "    " << e.identifier << "(int value) {\n";
    enumFile << "        this.value = value;\n";
    enumFile << "    }\n\n";
    
    enumFile << "    public int getValue() {\n";
    enumFile << "        return value;\n";
    enumFile << "    }\n\n";
    
    enumFile << "    public static " << e.identifier << " fromValue(int value) {\n";
    enumFile << "        for (" << e.identifier << " e : " << e.identifier << ".values()) {\n";
    enumFile << "            if (e.getValue() == value) {\n";
    enumFile << "                return e;\n";
    enumFile << "            }\n";
    enumFile << "        }\n";
    enumFile << "        throw new IllegalArgumentException(\"Unknown enum value: \" + value);\n";
    enumFile << "    }\n\n";
    
    enumFile << "    public static " << e.identifier << " fromString(String name) {\n";
    enumFile << "        try {\n";
    enumFile << "            return " << e.identifier << ".valueOf(name.toUpperCase());\n";
    enumFile << "        } catch (IllegalArgumentException e) {\n";
    enumFile << "            throw new IllegalArgumentException(\"Unknown enum name: \" + name);\n";
    enumFile << "        }\n";
    enumFile << "    }\n";
    
    enumFile << "}\n";
    enumFile.close();
}

void JavaGenerator::generate_struct_file(StructDefinition s, ProgramStructure *ps, std::string out_path, std::vector<StructDefinition> base_classes)
{
    std::ofstream structFile(out_path + "/" + s.identifier + ".java");
    if (!structFile.is_open())
    {
        std::cout << "Failed to open file: " << out_path + "/" + s.identifier + ".java" << std::endl;
        return;
    }
    
    // Imports
    structFile << "import java.util.*;\n";
    structFile << "import java.util.Objects;\n";
    structFile << "import java.sql.*;\n";
    structFile << "import com.fasterxml.jackson.databind.ObjectMapper;\n";
    structFile << "import org.luaj.vm2.*;\n";
    structFile << "import org.luaj.vm2.lib.jse.*;\n";
    
    // Add imports for generator-specific classes
    for (auto &bc : base_classes)
    {
        if (!bc.identifier.empty())
        {
            for (auto &include : bc.includes)
            {
                if (include.find(".java") != std::string::npos || include.find("java.") != std::string::npos)
                {
                    structFile << "import " << include << ";\n";
                }
            }
        }
    }
    
    structFile << "\n";
    
    // Class declaration with interfaces
    structFile << "public class " << s.identifier;
    if (!base_classes.empty())
    {
        structFile << " implements ";
        bool first = true;
        for (auto &bc : base_classes)
        {
            if (!bc.identifier.empty())
            {
                if (!first) structFile << ", ";
                structFile << "Has" << bc.identifier;
                first = false;
            }
        }
    }
    structFile << " {\n";
    
    // Member variables
    for (auto &mv : s.member_variables)
    {
        structFile << "    private " << get_java_type(mv.type, ps) << " " << mv.identifier;
        if (!mv.required)
        {
            structFile << " = " << get_java_default_value(mv.type, ps);
        }
        structFile << ";\n";
    }
    
    structFile << "\n";
    
    // Default constructor
    structFile << "    public " << s.identifier << "() {\n";
    for (auto &mv : s.member_variables)
    {
        if (mv.required)
        {
            structFile << "        this." << mv.identifier << " = " << get_java_default_value(mv.type, ps) << ";\n";
        }
    }
    structFile << "    }\n\n";
    
    // Constructor with all parameters
    if (!s.member_variables.empty())
    {
        generate_constructor(s, ps, structFile);
    }
    
    // Getters and Setters
    for (auto &mv : s.member_variables)
    {
        generate_member_variable_getter(mv, ps, structFile);
        generate_member_variable_setter(mv, ps, structFile);
    }
    
    // Generate methods from other generators
    generate_generator_methods(s, ps, base_classes, structFile);
    
    // toString method
    generate_to_string_method(s, ps, structFile);
    
    // equals method
    generate_equals_method(s, ps, structFile);
    
    // hashCode method
    generate_hash_code_method(s, ps, structFile);
    
    structFile << "}\n";
    structFile.close();
}

void JavaGenerator::generate_member_variable_getter(MemberVariableDefinition &mv, ProgramStructure *ps, std::ofstream &structFile)
{
    std::string methodName = "get" + mv.identifier;
    methodName[3] = std::toupper(methodName[3]); // Capitalize first letter after "get"
    
    structFile << "    public " << get_java_type(mv.type, ps) << " " << methodName << "() {\n";
    structFile << "        return " << mv.identifier << ";\n";
    structFile << "    }\n\n";
}

void JavaGenerator::generate_member_variable_setter(MemberVariableDefinition &mv, ProgramStructure *ps, std::ofstream &structFile)
{
    std::string methodName = "set" + mv.identifier;
    methodName[3] = std::toupper(methodName[3]); // Capitalize first letter after "set"
    
    structFile << "    public void " << methodName << "(" << get_java_type(mv.type, ps) << " " << mv.identifier << ") {\n";
    structFile << "        this." << mv.identifier << " = " << mv.identifier << ";\n";
    structFile << "    }\n\n";
}

void JavaGenerator::generate_constructor(StructDefinition &s, ProgramStructure *ps, std::ofstream &structFile)
{
    structFile << "    public " << s.identifier << "(";
    
    for (size_t i = 0; i < s.member_variables.size(); i++)
    {
        structFile << get_java_type(s.member_variables[i].type, ps) << " " << s.member_variables[i].identifier;
        if (i < s.member_variables.size() - 1)
        {
            structFile << ", ";
        }
    }
    
    structFile << ") {\n";
    
    for (auto &mv : s.member_variables)
    {
        structFile << "        this." << mv.identifier << " = " << mv.identifier << ";\n";
    }
    
    structFile << "    }\n\n";
}

void JavaGenerator::generate_to_string_method(StructDefinition &s, ProgramStructure *ps, std::ofstream &structFile)
{
    structFile << "    @Override\n";
    structFile << "    public String toString() {\n";
    structFile << "        return \"" << s.identifier << "{\" +\n";
    
    for (size_t i = 0; i < s.member_variables.size(); i++)
    {
        structFile << "                \"" << s.member_variables[i].identifier << "=\" + " << s.member_variables[i].identifier;
        if (i < s.member_variables.size() - 1)
        {
            structFile << " + \", \" +\n";
        }
        else
        {
            structFile << " +\n";
        }
    }
    
    structFile << "                '}';\n";
    structFile << "    }\n\n";
}

void JavaGenerator::generate_equals_method(StructDefinition &s, ProgramStructure *ps, std::ofstream &structFile)
{
    structFile << "    @Override\n";
    structFile << "    public boolean equals(Object obj) {\n";
    structFile << "        if (this == obj) return true;\n";
    structFile << "        if (obj == null || getClass() != obj.getClass()) return false;\n";
    structFile << "        " << s.identifier << " that = (" << s.identifier << ") obj;\n";
    structFile << "        return ";
    
    for (size_t i = 0; i < s.member_variables.size(); i++)
    {
        auto &mv = s.member_variables[i];
        if (mv.type.is_array() || mv.type.is_struct(ps) || mv.type.is_string())
        {
            structFile << "Objects.equals(" << mv.identifier << ", that." << mv.identifier << ")";
        }
        else
        {
            structFile << mv.identifier << " == that." << mv.identifier;
        }
        
        if (i < s.member_variables.size() - 1)
        {
            structFile << " &&\n               ";
        }
    }
    
    structFile << ";\n";
    structFile << "    }\n\n";
}

void JavaGenerator::generate_hash_code_method(StructDefinition &s, ProgramStructure *ps, std::ofstream &structFile)
{
    structFile << "    @Override\n";
    structFile << "    public int hashCode() {\n";
    structFile << "        return Objects.hash(";
    
    for (size_t i = 0; i < s.member_variables.size(); i++)
    {
        structFile << s.member_variables[i].identifier;
        if (i < s.member_variables.size() - 1)
        {
            structFile << ", ";
        }
    }
    
    structFile << ");\n";
    structFile << "    }\n\n";
}

void JavaGenerator::generate_generator_methods(StructDefinition &s, ProgramStructure *ps, std::vector<StructDefinition> &base_classes, std::ofstream &structFile)
{
    // Generate methods from other generators
    for (auto &bc : base_classes)
    {
        if (bc.identifier.empty()) continue;
        
        for (auto &f : bc.functions)
        {
            structFile << "    @Override\n";
            if (f.static_function)
            {
                structFile << "    public static ";
            }
            else
            {
                structFile << "    public ";
            }
            
            // Handle return type properly for static methods
            if (f.identifier == "fromJson" || f.identifier == "loadFromDatabase" || f.identifier == "fromLuaTable")
            {
                structFile << s.identifier << " ";
            }
            else
            {
                structFile << convert_to_local_type(ps, f.return_type) << " ";
            }
            
            structFile << f.identifier << "(";
            
            // Generate parameters
            for (size_t i = 0; i < f.parameters.size(); i++)
            {
                structFile << convert_to_local_type(ps, f.parameters[i].first) << " " << f.parameters[i].second;
                if (i < f.parameters.size() - 1)
                {
                    structFile << ", ";
                }
            }
            
            structFile << ") {\n";
            
            // Generate method body - simplified Java implementation
            if (f.identifier == "toJSON" && bc.identifier == "Json")
            {
                structFile << "        // Convert to JSON using Jackson ObjectMapper\n";
                structFile << "        try {\n";
                structFile << "            ObjectMapper mapper = new ObjectMapper();\n";
                structFile << "            return mapper.writeValueAsString(this);\n";
                structFile << "        } catch (Exception e) {\n";
                structFile << "            throw new RuntimeException(\"Failed to serialize to JSON\", e);\n";
                structFile << "        }\n";
            }
            else if (f.identifier == "fromJSON" && bc.identifier == "Json")
            {
                structFile << "        // Parse from JSON using Jackson ObjectMapper\n";
                structFile << "        try {\n";
                structFile << "            ObjectMapper mapper = new ObjectMapper();\n";
                structFile << "            return mapper.readValue(" << f.parameters[0].second << ", " << s.identifier << ".class);\n";
                structFile << "        } catch (Exception e) {\n";
                structFile << "            throw new RuntimeException(\"Failed to parse from JSON\", e);\n";
                structFile << "        }\n";
            }
            else if (f.identifier == "validate" && bc.identifier == "Java")
            {
                structFile << "        // Validate required fields\n";
                for (auto &mv : s.member_variables)
                {
                    if (mv.required)
                    {
                        if (mv.type.is_string() || mv.type.is_struct(ps) || mv.type.is_array())
                        {
                            structFile << "        if (" << mv.identifier << " == null) return false;\n";
                        }
                        if (mv.type.is_string())
                        {
                            structFile << "        if (" << mv.identifier << ".isEmpty()) return false;\n";
                        }
                        if (mv.type.is_array())
                        {
                            structFile << "        if (" << mv.identifier << ".isEmpty()) return false;\n";
                        }
                    }
                }
                structFile << "        return true;\n";
            }
            else
            {
                // Use the existing generate_function if available
                if (f.generate_function)
                {
                    f.generate_function(this, ps, s, f, structFile);
                }
                else
                {
                    structFile << "        // TODO: Implement " << f.identifier << " method\n";
                    if (f.return_type.is_bool())
                    {
                        structFile << "        return false;\n";
                    }
                    else if (f.return_type.is_integer() || f.return_type.is_real())
                    {
                        structFile << "        return 0;\n";
                    }
                    else if (f.return_type.is_string())
                    {
                        structFile << "        return \"\";\n";
                    }
                    else if (f.return_type.identifier() != "void")
                    {
                        structFile << "        return null;\n";
                    }
                }
            }
            
            structFile << "    }\n\n";
        }
    }
}

bool JavaGenerator::add_generator(Generator *gen)
{
    generators.push_back(gen);
    return true;
}

std::string JavaGenerator::get_java_type(TypeDefinition type, ProgramStructure *ps)
{
    if (type.identifier() == INT8 || type.identifier() == INT16 || type.identifier() == INT32)
    {
        return "Integer";
    }
    if (type.identifier() == INT64)
    {
        return "Long";
    }
    if (type.identifier() == UINT8 || type.identifier() == UINT16 || type.identifier() == UINT32)
    {
        return "Integer";
    }
    if (type.identifier() == UINT64)
    {
        return "Long";
    }
    if (type.identifier() == FLOAT)
    {
        return "Float";
    }
    if (type.identifier() == DOUBLE)
    {
        return "Double";
    }
    if (type.identifier() == BOOL)
    {
        return "Boolean";
    }
    if (type.identifier() == STRING)
    {
        return "String";
    }
    if (type.identifier() == CHAR)
    {
        return "Character";
    }
    if (type.identifier() == ARRAY)
    {
        return "List<" + get_java_type(type.element_type(), ps) + ">";
    }
    
    // Check if it's an enum
    if (ps->tokenIsEnum(type.identifier()))
    {
        return type.identifier();
    }
    
    // Check if it's a struct
    if (ps->tokenIsStruct(type.identifier()))
    {
        return type.identifier();
    }
    
    return type.identifier();
}

std::string JavaGenerator::get_java_default_value(TypeDefinition type, ProgramStructure *ps)
{
    if (type.identifier() == INT8 || type.identifier() == INT16 || type.identifier() == INT32 ||
        type.identifier() == UINT8 || type.identifier() == UINT16 || type.identifier() == UINT32)
    {
        return "0";
    }
    if (type.identifier() == INT64 || type.identifier() == UINT64)
    {
        return "0L";
    }
    if (type.identifier() == FLOAT)
    {
        return "0.0f";
    }
    if (type.identifier() == DOUBLE)
    {
        return "0.0";
    }
    if (type.identifier() == BOOL)
    {
        return "false";
    }
    if (type.identifier() == STRING)
    {
        return "\"\"";
    }
    if (type.identifier() == CHAR)
    {
        return "'\\0'";
    }
    if (type.identifier() == ARRAY)
    {
        return "new ArrayList<>()";
    }
    
    // Check if it's an enum - use first value
    if (ps->tokenIsEnum(type.identifier()))
    {
        EnumDefinition e = ps->getEnum(type.identifier());
        if (!e.values.empty())
        {
            return type.identifier() + "." + e.values[0].first;
        }
        return "null";
    }
    
    // Check if it's a struct
    if (ps->tokenIsStruct(type.identifier()))
    {
        return "new " + type.identifier() + "()";
    }
    
    return "null";
}

JavaGenerator::JavaGenerator()
{
    base_class.identifier = "Java";
    
    // Add validation method
    FunctionDefinition validate;
    validate.identifier = "validate";
    validate.return_type.identifier() = "boolean";
    validate.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
    {
        structFile << "        // Validate required fields\n";
        for (auto &mv : s.member_variables)
        {
            if (mv.required)
            {
                if (mv.type.is_string() || mv.type.is_struct(ps) || mv.type.is_array())
                {
                    structFile << "        if (" << mv.identifier << " == null) return false;\n";
                }
                if (mv.type.is_string())
                {
                    structFile << "        if (" << mv.identifier << ".isEmpty()) return false;\n";
                }
                if (mv.type.is_array())
                {
                    structFile << "        if (" << mv.identifier << ".isEmpty()) return false;\n";
                }
            }
        }
        structFile << "        return true;\n";
        return true;
    };
    
    // Add clone method
    FunctionDefinition clone;
    clone.identifier = "clone";
    clone.return_type.identifier() = "Object";
    clone.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
    {
        structFile << "        " << s.identifier << " cloned = new " << s.identifier << "();\n";
        for (auto &mv : s.member_variables)
        {
            if (mv.type.is_array())
            {
                structFile << "        cloned." << mv.identifier << " = new ArrayList<>(" << mv.identifier << ");\n";
            }
            else if (mv.type.is_struct(ps))
            {
                structFile << "        cloned." << mv.identifier << " = (" << mv.identifier << " != null) ? (" << mv.type.identifier() << ") " << mv.identifier << ".clone() : null;\n";
            }
            else
            {
                structFile << "        cloned." << mv.identifier << " = " << mv.identifier << ";\n";
            }
        }
        structFile << "        return cloned;\n";
        return true;
    };
    
    // Add JSON serialization methods
    FunctionDefinition toJson;
    toJson.identifier = "toJson";
    toJson.return_type.identifier() = "String";
    toJson.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
    {
        structFile << "        // Convert to JSON using Jackson ObjectMapper\n";
        structFile << "        try {\n";
        structFile << "            ObjectMapper mapper = new ObjectMapper();\n";
        structFile << "            return mapper.writeValueAsString(this);\n";
        structFile << "        } catch (Exception e) {\n";
        structFile << "            throw new RuntimeException(\"Failed to serialize to JSON\", e);\n";
        structFile << "        }\n";
        return true;
    };
    
    FunctionDefinition fromJson;
    fromJson.identifier = "fromJson";
    fromJson.return_type.identifier() = "Object"; // Will be corrected in generation
    fromJson.static_function = true;
    fromJson.parameters.push_back(std::make_pair(TypeDefinition("String"), "json"));
    fromJson.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
    {
        structFile << "        // Parse from JSON using Jackson ObjectMapper\n";
        structFile << "        try {\n";
        structFile << "            ObjectMapper mapper = new ObjectMapper();\n";
        structFile << "            return mapper.readValue(json, " << s.identifier << ".class);\n";
        structFile << "        } catch (Exception e) {\n";
        structFile << "            throw new RuntimeException(\"Failed to parse from JSON\", e);\n";
        structFile << "        }\n";
        return true;
    };
    
    // Add database methods for MySQL/SQLite
    FunctionDefinition saveToDatabase;
    saveToDatabase.identifier = "saveToDatabase";
    saveToDatabase.return_type.identifier() = "void";
    saveToDatabase.parameters.push_back(std::make_pair(TypeDefinition("Connection"), "connection"));
    saveToDatabase.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
    {
        structFile << "        // Save to database using JDBC\n";
        structFile << "        String sql = \"INSERT INTO " << s.identifier << " (";
        
        // Generate column names
        bool first = true;
        for (auto &mv : s.member_variables)
        {
            if (!first) structFile << ", ";
            structFile << mv.identifier;
            first = false;
        }
        structFile << ") VALUES (";
        
        // Generate placeholders
        first = true;
        for (auto &mv : s.member_variables)
        {
            if (!first) structFile << ", ";
            structFile << "?";
            first = false;
        }
        structFile << ")\";\n";
        
        structFile << "        try (PreparedStatement stmt = connection.prepareStatement(sql)) {\n";
        
        int paramIndex = 1;
        for (auto &mv : s.member_variables)
        {
            structFile << "            stmt.setObject(" << paramIndex << ", " << mv.identifier << ");\n";
            paramIndex++;
        }
        
        structFile << "            stmt.executeUpdate();\n";
        structFile << "        } catch (SQLException e) {\n";
        structFile << "            throw new RuntimeException(\"Failed to save to database\", e);\n";
        structFile << "        }\n";
        return true;
    };
    
    FunctionDefinition loadFromDatabase;
    loadFromDatabase.identifier = "loadFromDatabase";
    loadFromDatabase.return_type.identifier() = "Object"; // Will be corrected in generation
    loadFromDatabase.static_function = true;
    loadFromDatabase.parameters.push_back(std::make_pair(TypeDefinition("Connection"), "connection"));
    loadFromDatabase.parameters.push_back(std::make_pair(TypeDefinition("Object"), "id"));
    loadFromDatabase.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
    {
        structFile << "        // Load from database using JDBC\n";
        structFile << "        String sql = \"SELECT * FROM " << s.identifier << " WHERE id = ?\";\n";
        structFile << "        try (PreparedStatement stmt = connection.prepareStatement(sql)) {\n";
        structFile << "            stmt.setObject(1, id);\n";
        structFile << "            try (ResultSet rs = stmt.executeQuery()) {\n";
        structFile << "                if (rs.next()) {\n";
        structFile << "                    " << s.identifier << " obj = new " << s.identifier << "();\n";
        
        for (auto &mv : s.member_variables)
        {
            structFile << "                    obj." << mv.identifier << " = ";
            if (mv.type.is_string())
            {
                structFile << "rs.getString(\"" << mv.identifier << "\");\n";
            }
            else if (mv.type.is_integer())
            {
                structFile << "rs.getInt(\"" << mv.identifier << "\");\n";
            }
            else if (mv.type.is_real())
            {
                structFile << "rs.getDouble(\"" << mv.identifier << "\");\n";
            }
            else if (mv.type.is_bool())
            {
                structFile << "rs.getBoolean(\"" << mv.identifier << "\");\n";
            }
            else
            {
                structFile << "rs.getObject(\"" << mv.identifier << "\");\n";
            }
        }
        
        structFile << "                    return obj;\n";
        structFile << "                }\n";
        structFile << "            }\n";
        structFile << "        } catch (SQLException e) {\n";
        structFile << "            throw new RuntimeException(\"Failed to load from database\", e);\n";
        structFile << "        }\n";
        structFile << "        return null;\n";
        return true;
    };
    
    // Add Lua integration methods
    FunctionDefinition toLuaTable;
    toLuaTable.identifier = "toLuaTable";
    toLuaTable.return_type.identifier() = "String";
    toLuaTable.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
    {
        structFile << "        // Convert to Lua table format using LuaJ\n";
        structFile << "        StringBuilder lua = new StringBuilder();\n";
        structFile << "        lua.append(\"{\");\n";
        
        bool first = true;
        for (auto &mv : s.member_variables)
        {
            if (!first)
            {
                structFile << "        lua.append(\", \");\n";
            }
            structFile << "        lua.append(\"" << mv.identifier << " = \");\n";
            
            if (mv.type.is_string())
            {
                structFile << "        lua.append(\"\\\"\").append(" << mv.identifier << ").append(\"\\\"\");\n";
            }
            else if (mv.type.is_bool())
            {
                structFile << "        lua.append(" << mv.identifier << " ? \"true\" : \"false\");\n";
            }
            else if (mv.type.is_array())
            {
                structFile << "        lua.append(\"{\");\n";
                structFile << "        for (int i = 0; i < " << mv.identifier << ".size(); i++) {\n";
                structFile << "            if (i > 0) lua.append(\", \");\n";
                if (mv.type.element_type().is_string())
                {
                    structFile << "            lua.append(\"\\\"\").append(" << mv.identifier << ".get(i)).append(\"\\\"\");\n";
                }
                else
                {
                    structFile << "            lua.append(" << mv.identifier << ".get(i));\n";
                }
                structFile << "        }\n";
                structFile << "        lua.append(\"}\");\n";
            }
            else
            {
                structFile << "        lua.append(" << mv.identifier << ");\n";
            }
            first = false;
        }
        
        structFile << "        lua.append(\"}\");\n";
        structFile << "        return lua.toString();\n";
        return true;
    };
    
    FunctionDefinition fromLuaTable;
    fromLuaTable.identifier = "fromLuaTable";
    fromLuaTable.return_type.identifier() = "Object"; // Will be corrected in generation
    fromLuaTable.static_function = true;
    fromLuaTable.parameters.push_back(std::make_pair(TypeDefinition("LuaValue"), "luaTable"));
    fromLuaTable.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
    {
        structFile << "        // Parse from Lua table using LuaJ\n";
        structFile << "        if (!luaTable.istable()) {\n";
        structFile << "            throw new IllegalArgumentException(\"Expected Lua table\");\n";
        structFile << "        }\n";
        structFile << "        " << s.identifier << " obj = new " << s.identifier << "();\n";
        structFile << "        LuaTable table = luaTable.checktable();\n";
        
        for (auto &mv : s.member_variables)
        {
            structFile << "        LuaValue " << mv.identifier << "Value = table.get(\"" << mv.identifier << "\");\n";
            structFile << "        if (!" << mv.identifier << "Value.isnil()) {\n";
            
            if (mv.type.is_string())
            {
                structFile << "            obj." << mv.identifier << " = " << mv.identifier << "Value.checkjstring();\n";
            }
            else if (mv.type.is_integer())
            {
                structFile << "            obj." << mv.identifier << " = " << mv.identifier << "Value.checkint();\n";
            }
            else if (mv.type.is_real())
            {
                structFile << "            obj." << mv.identifier << " = " << mv.identifier << "Value.checkdouble();\n";
            }
            else if (mv.type.is_bool())
            {
                structFile << "            obj." << mv.identifier << " = " << mv.identifier << "Value.checkboolean();\n";
            }
            else if (mv.type.is_array())
            {
                structFile << "            if (" << mv.identifier << "Value.istable()) {\n";
                structFile << "                LuaTable arrayTable = " << mv.identifier << "Value.checktable();\n";
                structFile << "                obj." << mv.identifier << " = new ArrayList<>();\n";
                structFile << "                for (int i = 1; i <= arrayTable.length(); i++) {\n";
                if (mv.type.element_type().is_string())
                {
                    structFile << "                    obj." << mv.identifier << ".add(arrayTable.get(i).checkjstring());\n";
                }
                else if (mv.type.element_type().is_integer())
                {
                    structFile << "                    obj." << mv.identifier << ".add(arrayTable.get(i).checkint());\n";
                }
                else
                {
                    structFile << "                    obj." << mv.identifier << ".add(arrayTable.get(i));\n";
                }
                structFile << "                }\n";
                structFile << "            }\n";
            }
            
            structFile << "        }\n";
        }
        
        structFile << "        return obj;\n";
        return true;
    };
    
    base_class.functions.push_back(validate);
    base_class.functions.push_back(clone);
    base_class.functions.push_back(toJson);
    base_class.functions.push_back(fromJson);
    base_class.functions.push_back(saveToDatabase);
    base_class.functions.push_back(loadFromDatabase);
    base_class.functions.push_back(toLuaTable);
    base_class.functions.push_back(fromLuaTable);
    
    // Add C++ integration methods (these generate C++ code when used by CppGenerator)
    FunctionDefinition to_cpp_object;
    to_cpp_object.identifier = "to_cpp_object";
    to_cpp_object.return_type.identifier() = "jobject";
    to_cpp_object.parameters.push_back(std::make_pair(TypeDefinition("JNIEnv*"), "env"));
    to_cpp_object.parameters.push_back(std::make_pair(TypeDefinition("jclass"), "java_class"));
    to_cpp_object.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
    {
        // Check if this is being generated for CppGenerator (C++ code) or JavaGenerator (Java code)
        std::string gen_type = gen->convert_to_local_type(ps, TypeDefinition("int"));
        if (gen_type == "int32_t" || gen_type == "int") // This is C++ generator
        {
            structFile << "\t// Create Java object from this C++ object\n";
            structFile << "\tjmethodID constructor = env->GetMethodID(java_class, \"<init>\", \"()V\");\n";
            structFile << "\tif (!constructor) return nullptr;\n";
            structFile << "\tjobject java_obj = env->NewObject(java_class, constructor);\n";
            structFile << "\tif (!java_obj) return nullptr;\n\n";
            
            for (auto &mv : s.member_variables)
            {
                std::string setter_name = "set" + mv.identifier;
                setter_name[3] = std::toupper(setter_name[3]);
                
                structFile << "\t// Set field: " << mv.identifier << "\n";
                
                if (mv.type.is_integer())
                {
                    structFile << "\tjmethodID " << mv.identifier << "_setter = env->GetMethodID(java_class, \"" << setter_name << "\", \"(I)V\");\n";
                    structFile << "\tif (" << mv.identifier << "_setter) {\n";
                    structFile << "\t\tenv->CallVoidMethod(java_obj, " << mv.identifier << "_setter, (jint)" << mv.identifier << ");\n";
                    structFile << "\t}\n\n";
                }
                else if (mv.type.is_string())
                {
                    structFile << "\tjstring j" << mv.identifier << " = env->NewStringUTF(" << mv.identifier << ".c_str());\n";
                    structFile << "\tjmethodID " << mv.identifier << "_setter = env->GetMethodID(java_class, \"" << setter_name << "\", \"(Ljava/lang/String;)V\");\n";
                    structFile << "\tif (" << mv.identifier << "_setter && j" << mv.identifier << ") {\n";
                    structFile << "\t\tenv->CallVoidMethod(java_obj, " << mv.identifier << "_setter, j" << mv.identifier << ");\n";
                    structFile << "\t\tenv->DeleteLocalRef(j" << mv.identifier << ");\n";
                    structFile << "\t}\n\n";
                }
                else if (mv.type.is_bool())
                {
                    structFile << "\tjmethodID " << mv.identifier << "_setter = env->GetMethodID(java_class, \"" << setter_name << "\", \"(Z)V\");\n";
                    structFile << "\tif (" << mv.identifier << "_setter) {\n";
                    structFile << "\t\tenv->CallVoidMethod(java_obj, " << mv.identifier << "_setter, (jboolean)" << mv.identifier << ");\n";
                    structFile << "\t}\n\n";
                }
            }
            
            structFile << "\treturn java_obj;\n";
        }
        else // This is Java generator
        {
            structFile << "    public " << fd.return_type.identifier() << " " << fd.identifier << "(JNIEnv env, Class javaClass) {\n";
            structFile << "        // Convert Java object to C++ object pointer (JNI implementation needed)\n";
            structFile << "        return nativeToCppObject(env, javaClass);\n";
            structFile << "    }\n\n";
            structFile << "    private native " << fd.return_type.identifier() << " nativeToCppObject(JNIEnv env, Class javaClass);\n\n";
        }
        return true;
    };
    
    FunctionDefinition from_cpp_object;
    from_cpp_object.identifier = "from_cpp_object";
    from_cpp_object.return_type.identifier() = "void";
    from_cpp_object.parameters.push_back(std::make_pair(TypeDefinition("JNIEnv*"), "env"));
    from_cpp_object.parameters.push_back(std::make_pair(TypeDefinition("jobject"), "java_obj"));
    from_cpp_object.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
    {
        // Check if this is being generated for CppGenerator (C++ code) or JavaGenerator (Java code)
        std::string gen_type = gen->convert_to_local_type(ps, TypeDefinition("int"));
        if (gen_type == "int32_t" || gen_type == "int") // This is C++ generator
        {
            structFile << "\t// Populate this C++ object from Java object\n";
            structFile << "\tif (!java_obj) return;\n";
            structFile << "\tjclass java_class = env->GetObjectClass(java_obj);\n";
            structFile << "\tif (!java_class) return;\n\n";
            
            for (auto &mv : s.member_variables)
            {
                std::string getter_name = "get" + mv.identifier;
                getter_name[3] = std::toupper(getter_name[3]);
                
                structFile << "\t// Get field: " << mv.identifier << "\n";
                
                if (mv.type.is_integer())
                {
                    structFile << "\tjmethodID " << mv.identifier << "_getter = env->GetMethodID(java_class, \"" << getter_name << "\", \"()I\");\n";
                    structFile << "\tif (" << mv.identifier << "_getter) {\n";
                    structFile << "\t\t" << mv.identifier << " = env->CallIntMethod(java_obj, " << mv.identifier << "_getter);\n";
                    structFile << "\t}\n\n";
                }
                else if (mv.type.is_string())
                {
                    structFile << "\tjmethodID " << mv.identifier << "_getter = env->GetMethodID(java_class, \"" << getter_name << "\", \"()Ljava/lang/String;\");\n";
                    structFile << "\tif (" << mv.identifier << "_getter) {\n";
                    structFile << "\t\tjstring j" << mv.identifier << " = (jstring)env->CallObjectMethod(java_obj, " << mv.identifier << "_getter);\n";
                    structFile << "\t\tif (j" << mv.identifier << ") {\n";
                    structFile << "\t\t\tconst char* cstr = env->GetStringUTFChars(j" << mv.identifier << ", nullptr);\n";
                    structFile << "\t\t\tif (cstr) {\n";
                    structFile << "\t\t\t\t" << mv.identifier << " = std::string(cstr);\n";
                    structFile << "\t\t\t\tenv->ReleaseStringUTFChars(j" << mv.identifier << ", cstr);\n";
                    structFile << "\t\t\t}\n";
                    structFile << "\t\t\tenv->DeleteLocalRef(j" << mv.identifier << ");\n";
                    structFile << "\t\t}\n";
                    structFile << "\t}\n\n";
                }
                else if (mv.type.is_bool())
                {
                    structFile << "\tjmethodID " << mv.identifier << "_getter = env->GetMethodID(java_class, \"" << getter_name << "\", \"()Z\");\n";
                    structFile << "\tif (" << mv.identifier << "_getter) {\n";
                    structFile << "\t\t" << mv.identifier << " = env->CallBooleanMethod(java_obj, " << mv.identifier << "_getter);\n";
                    structFile << "\t}\n\n";
                }
            }
            
            structFile << "\tenv->DeleteLocalRef(java_class);\n";
        }
        else // This is Java generator
        {
            structFile << "    public static " << s.identifier << " " << fd.identifier << "(JNIEnv env, Object javaObj) {\n";
            structFile << "        // Convert from Java object (JNI implementation needed)\n";
            structFile << "        " << s.identifier << " instance = new " << s.identifier << "();\n";
            structFile << "        instance.nativeFromCppObject(env, javaObj);\n";
            structFile << "        return instance;\n";
            structFile << "    }\n\n";
            structFile << "    private native void nativeFromCppObject(JNIEnv env, Object javaObj);\n\n";
        }
        return true;
    };
    
    FunctionDefinition create_jni_bridge;
    create_jni_bridge.identifier = "create_jni_bridge";
    create_jni_bridge.return_type.identifier() = "std::string";
    create_jni_bridge.static_function = true;
    create_jni_bridge.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ofstream &structFile)
    {
        // Check if this is being generated for CppGenerator (C++ code) or JavaGenerator (Java code)
        std::string gen_type = gen->convert_to_local_type(ps, TypeDefinition("int"));
        if (gen_type == "int32_t" || gen_type == "int") // This is C++ generator
        {
            structFile << "\t// Generate JNI bridge methods for Java interop\n";
            structFile << "\tstd::string bridge = \"// JNI Bridge for " << s.identifier << "Schema\\n\";\n";
            structFile << "\tbridge += \"extern \\\"C\\\" {\\n\";\n";
            structFile << "\tbridge += \"JNIEXPORT jlong JNICALL Java_\" + std::string(\"" << s.identifier << "\") + \"_nativeCreate(JNIEnv* env, jclass clazz) {\\n\";\n";
            structFile << "\tbridge += \"  return (jlong) new " << s.identifier << "Schema();\\n\";\n";
            structFile << "\tbridge += \"}\\n\";\n";
            structFile << "\tbridge += \"JNIEXPORT void JNICALL Java_\" + std::string(\"" << s.identifier << "\") + \"_nativeDestroy(JNIEnv* env, jclass clazz, jlong ptr) {\\n\";\n";
            structFile << "\tbridge += \"  delete (" << s.identifier << "Schema*) ptr;\\n\";\n";
            structFile << "\tbridge += \"}\\n\";\n";
            structFile << "\tbridge += \"JNIEXPORT jobject JNICALL Java_\" + std::string(\"" << s.identifier << "\") + \"_nativeToCppObject(JNIEnv* env, jobject thiz, jlong ptr) {\\n\";\n";
            structFile << "\tbridge += \"  " << s.identifier << "Schema* obj = (" << s.identifier << "Schema*) ptr;\\n\";\n";
            structFile << "\tbridge += \"  jclass clazz = env->GetObjectClass(thiz);\\n\";\n";
            structFile << "\tbridge += \"  return obj->to_cpp_object(env, clazz);\\n\";\n";
            structFile << "\tbridge += \"}\\n\";\n";
            structFile << "\tbridge += \"}\\n\";\n";
            structFile << "\treturn bridge;\n";
        }
        else // This is Java generator
        {
            structFile << "    public static String " << fd.identifier << "() {\n";
            structFile << "        // Generate JNI bridge documentation for C++ integration\n";
            structFile << "        StringBuilder sb = new StringBuilder();\n";
            structFile << "        sb.append(\"// JNI Bridge methods for " << s.identifier << "\\n\");\n";
            structFile << "        sb.append(\"// Use " << s.identifier << "Schema::create_jni_bridge() to get C++ implementation\\n\");\n";
            structFile << "        return sb.toString();\n";
            structFile << "    }\n\n";
        }
        return true;
    };
    
    base_class.functions.push_back(to_cpp_object);
    base_class.functions.push_back(from_cpp_object);
    base_class.functions.push_back(create_jni_bridge);
}

std::string JavaGenerator::convert_to_local_type(ProgramStructure *ps, TypeDefinition type)
{
    return get_java_type(type, ps);
}

bool JavaGenerator::add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
    return true;
}

bool JavaGenerator::generate_files(ProgramStructure ps, std::string out_path)
{
    if (!std::filesystem::exists(out_path))
    {
        std::filesystem::create_directories(out_path);
    }
    
    // Collect base classes from other generators
    std::vector<StructDefinition> base_classes;
    for (auto &gen : generators)
    {
        if (gen == this) continue;
        if (!gen->base_class.identifier.empty())
        {
            base_classes.push_back(gen->base_class);
            
            // Generate interface files for Java
            std::ofstream interfaceFile(out_path + "/Has" + gen->base_class.identifier + ".java");
            if (interfaceFile.is_open())
            {
                interfaceFile << "import java.util.*;\n\n";
                interfaceFile << "public interface Has" << gen->base_class.identifier << " {\n";
                
                for (auto &f : gen->base_class.functions)
                {
                    interfaceFile << "    " << convert_to_local_type(&ps, f.return_type) << " " << f.identifier << "(";
                    
                    for (size_t i = 0; i < f.parameters.size(); i++)
                    {
                        interfaceFile << convert_to_local_type(&ps, f.parameters[i].first) << " " << f.parameters[i].second;
                        if (i < f.parameters.size() - 1)
                        {
                            interfaceFile << ", ";
                        }
                    }
                    
                    interfaceFile << ");\n";
                }
                
                interfaceFile << "}\n";
                interfaceFile.close();
            }
        }
        
        // Allow other generators to add content to structs
        for (auto &s : ps.getStructs())
        {
            gen->add_generator_specific_content_to_struct(this, &ps, s);
        }
    }
    
    // Generate enum files
    for (auto &e : ps.getEnums())
    {
        generate_enum_file(e, out_path);
    }
    
    // Generate struct files with base classes
    for (auto &s : ps.getStructs())
    {
        generate_struct_file(s, &ps, out_path, base_classes);
    }
    
    return true;
}
