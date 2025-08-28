#include <BuiltInGenerators/LuaGenerator.hpp>

LuaGenerator::LuaGenerator()
{
	name = "Lua";
	base_class.setIdentifier("Lua");
	
	// Add lua_push method
	FunctionDefinition lua_push;
	lua_push.generator = "Lua";
	lua_push.identifier = "lua_push";
	lua_push.return_type.identifier() = "void";
	lua_push.parameters.push_back(std::make_pair(TypeDefinition("lua_State*"), "L"));
	lua_push.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\t// Push this object to Lua stack as a table\n";
		structFile << "\tlua_newtable(L);\n";
		
		int field_index = 1;
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			structFile << "\t// Push field: " << mv.identifier << "\n";
			structFile << "\tlua_pushstring(L, \"" << mv.identifier << "\");\n";
			
			if (mv.type.is_integer())
			{
				structFile << "\tlua_pushinteger(L, " << mv.identifier << ");\n";
			}
			else if (mv.type.is_real())
			{
				structFile << "\tlua_pushnumber(L, " << mv.identifier << ");\n";
			}
			else if (mv.type.is_bool())
			{
				structFile << "\tlua_pushboolean(L, " << mv.identifier << ");\n";
			}
			else if (mv.type.is_string())
			{
				structFile << "\tlua_pushstring(L, " << mv.identifier << ".c_str());\n";
			}
			else if (mv.type.is_enum(ps))
			{
				structFile << "\tlua_pushstring(L, " << mv.type.identifier() << "SchemaToString(" << mv.identifier << ").c_str());\n";
			}
			else if (mv.type.is_array())
			{
				structFile << "\tlua_newtable(L);\n";
				structFile << "\tfor (size_t i = 0; i < " << mv.identifier << ".size(); i++) {\n";
				structFile << "\t\tlua_pushinteger(L, i + 1); // Lua arrays are 1-indexed\n";
				
				if (mv.type.element_type().is_integer())
				{
					structFile << "\t\tlua_pushinteger(L, " << mv.identifier << "[i]);\n";
				}
				else if (mv.type.element_type().is_real())
				{
					structFile << "\t\tlua_pushnumber(L, " << mv.identifier << "[i]);\n";
				}
				else if (mv.type.element_type().is_bool())
				{
					structFile << "\t\tlua_pushboolean(L, " << mv.identifier << "[i]);\n";
				}
				else if (mv.type.element_type().is_string())
				{
					structFile << "\t\tlua_pushstring(L, " << mv.identifier << "[i].c_str());\n";
				}
				else if (mv.type.element_type().is_struct(ps))
				{
					structFile << "\t\t" << mv.identifier << "[i]->lua_push(L);\n";
				}
				else if (mv.type.element_type().is_enum(ps))
				{
					structFile << "\t\tlua_pushstring(L, " << mv.type.element_type().identifier() << "SchemaToString(" << mv.identifier << "[i]).c_str());\n";
				}
				
				structFile << "\t\tlua_settable(L, -3);\n";
				structFile << "\t}\n";
			}
			else if (mv.type.is_struct(ps))
			{
				structFile << "\tif (" << mv.identifier << " != nullptr) {\n";
				structFile << "\t\t" << mv.identifier << "->lua_push(L);\n";
				structFile << "\t} else {\n";
				structFile << "\t\tlua_pushnil(L);\n";
				structFile << "\t}\n";
			}
			else
			{
				structFile << "\tlua_pushnil(L); // Unknown type\n";
			}
			
			structFile << "\tlua_settable(L, -3);\n";
		}
		
		return true;
	};
	
	// Add lua_to method
	FunctionDefinition lua_to;
	lua_to.generator = "Lua";
	lua_to.identifier = "lua_to";
	lua_to.return_type.identifier() = "void";
	lua_to.parameters.push_back(std::make_pair(TypeDefinition("lua_State*"), "L"));
	lua_to.parameters.push_back(std::make_pair(TypeDefinition("int"), "index"));
	lua_to.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\t// Convert from Lua table at given index to this object\n";
		structFile << "\tif (!lua_istable(L, index)) {\n";
		structFile << "\t\treturn; // Not a table\n";
		structFile << "\t}\n\n";
		
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			structFile << "\t// Convert field: " << mv.identifier << "\n";
			structFile << "\tlua_pushstring(L, \"" << mv.identifier << "\");\n";
			structFile << "\tlua_gettable(L, index);\n";
			structFile << "\tif (!lua_isnil(L, -1)) {\n";
			
			if (mv.type.is_integer())
			{
				structFile << "\t\tif (lua_isinteger(L, -1)) {\n";
				structFile << "\t\t\t" << mv.identifier << " = lua_tointeger(L, -1);\n";
				structFile << "\t\t}\n";
			}
			else if (mv.type.is_real())
			{
				structFile << "\t\tif (lua_isnumber(L, -1)) {\n";
				structFile << "\t\t\t" << mv.identifier << " = lua_tonumber(L, -1);\n";
				structFile << "\t\t}\n";
			}
			else if (mv.type.is_bool())
			{
				structFile << "\t\tif (lua_isboolean(L, -1)) {\n";
				structFile << "\t\t\t" << mv.identifier << " = lua_toboolean(L, -1);\n";
				structFile << "\t\t}\n";
			}
			else if (mv.type.is_string())
			{
				structFile << "\t\tif (lua_isstring(L, -1)) {\n";
				structFile << "\t\t\t" << mv.identifier << " = lua_tostring(L, -1);\n";
				structFile << "\t\t}\n";
			}
			else if (mv.type.is_enum(ps))
			{
				structFile << "\t\tif (lua_isstring(L, -1)) {\n";
				structFile << "\t\t\t" << mv.identifier << " = " << mv.type.identifier() << "SchemaFromString(lua_tostring(L, -1));\n";
				structFile << "\t\t}\n";
			}
			else if (mv.type.is_array())
			{
				structFile << "\t\tif (lua_istable(L, -1)) {\n";
				structFile << "\t\t\t" << mv.identifier << ".clear();\n";
				structFile << "\t\t\tsize_t len = lua_rawlen(L, -1);\n";
				structFile << "\t\t\tfor (size_t i = 1; i <= len; i++) {\n";
				structFile << "\t\t\t\tlua_rawgeti(L, -1, i);\n";
				
				if (mv.type.element_type().is_integer())
				{
					structFile << "\t\t\t\tif (lua_isinteger(L, -1)) {\n";
					structFile << "\t\t\t\t\t" << mv.identifier << ".push_back(lua_tointeger(L, -1));\n";
					structFile << "\t\t\t\t}\n";
				}
				else if (mv.type.element_type().is_real())
				{
					structFile << "\t\t\t\tif (lua_isnumber(L, -1)) {\n";
					structFile << "\t\t\t\t\t" << mv.identifier << ".push_back(lua_tonumber(L, -1));\n";
					structFile << "\t\t\t\t}\n";
				}
				else if (mv.type.element_type().is_bool())
				{
					structFile << "\t\t\t\tif (lua_isboolean(L, -1)) {\n";
					structFile << "\t\t\t\t\t" << mv.identifier << ".push_back(lua_toboolean(L, -1));\n";
					structFile << "\t\t\t\t}\n";
				}
				else if (mv.type.element_type().is_string())
				{
					structFile << "\t\t\t\tif (lua_isstring(L, -1)) {\n";
					structFile << "\t\t\t\t\t" << mv.identifier << ".push_back(lua_tostring(L, -1));\n";
					structFile << "\t\t\t\t}\n";
				}
				else if (mv.type.element_type().is_struct(ps))
				{
					structFile << "\t\t\t\tif (lua_istable(L, -1)) {\n";
					structFile << "\t\t\t\t\t" << mv.type.element_type().identifier() << "Schema* item = new " << mv.type.element_type().identifier() << "Schema();\n";
					structFile << "\t\t\t\t\titem->lua_to(L, lua_gettop(L));\n";
					structFile << "\t\t\t\t\t" << mv.identifier << ".push_back(item);\n";
					structFile << "\t\t\t\t}\n";
				}
				else if (mv.type.element_type().is_enum(ps))
				{
					structFile << "\t\t\t\tif (lua_isstring(L, -1)) {\n";
					structFile << "\t\t\t\t\t" << mv.identifier << ".push_back(" << mv.type.element_type().identifier() << "SchemaFromString(lua_tostring(L, -1)));\n";
					structFile << "\t\t\t\t}\n";
				}
				
				structFile << "\t\t\t\tlua_pop(L, 1);\n";
				structFile << "\t\t\t}\n";
				structFile << "\t\t}\n";
			}
			else if (mv.type.is_struct(ps))
			{
				structFile << "\t\tif (lua_istable(L, -1)) {\n";
				structFile << "\t\t\tif (" << mv.identifier << " == nullptr) {\n";
				structFile << "\t\t\t\t" << mv.identifier << " = new " << mv.type.identifier() << "Schema();\n";
				structFile << "\t\t\t}\n";
				structFile << "\t\t\t" << mv.identifier << "->lua_to(L, lua_gettop(L));\n";
				structFile << "\t\t}\n";
			}
			
			structFile << "\t}\n";
			structFile << "\tlua_pop(L, 1); // Remove field value from stack\n\n";
		}
		
		return true;
	};
	
	base_class.add_function(lua_push);
	base_class.add_function(lua_to);
	
	// Add lua_create_table static method
	FunctionDefinition lua_create_table;
	lua_create_table.generator = "Lua";
	lua_create_table.identifier = "lua_create_table";
	lua_create_table.return_type.identifier() = "void";
	lua_create_table.static_function = true;
	lua_create_table.parameters.push_back(std::make_pair(TypeDefinition("lua_State*"), "L"));
	lua_create_table.parameters.push_back(std::make_pair(TypeDefinition("const std::string&"), "schema_name"));
	lua_create_table.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\t// Create a Lua table constructor for this schema\n";
		structFile << "\tstd::string lua_code = \"local \" + schema_name + \" = {}\\n\";\n";
		structFile << "\tlua_code += \"function \" + schema_name + \".new(data)\\n\";\n";
		structFile << "\tlua_code += \"  local instance = {}\\n\";\n";
		
		for (auto& [generator, mv] : s.getMemberVariables())
		{
			structFile << "\tlua_code += \"  instance." << mv.identifier << " = \";\n";
			if (mv.type.is_string())
			{
				structFile << "\tlua_code += \"data and data." << mv.identifier << " or ''\\n\";\n";
			}
			else if (mv.type.is_integer() || mv.type.is_real())
			{
				structFile << "\tlua_code += \"data and data." << mv.identifier << " or 0\\n\";\n";
			}
			else if (mv.type.is_bool())
			{
				structFile << "\tlua_code += \"data and data." << mv.identifier << " or false\\n\";\n";
			}
			else if (mv.type.is_array())
			{
				structFile << "\tlua_code += \"data and data." << mv.identifier << " or {}\\n\";\n";
			}
			else
			{
				structFile << "\tlua_code += \"data and data." << mv.identifier << " or nil\\n\";\n";
			}
		}
		
		structFile << "\tlua_code += \"  return instance\\n\";\n";
		structFile << "\tlua_code += \"end\\n\";\n";
		structFile << "\tlua_code += \"return \" + schema_name + \"\\n\";\n";
		structFile << "\tluaL_dostring(L, lua_code.c_str());\n";
		
		return true;
	};
	
	base_class.add_function(lua_create_table);
	
	// Add required includes for Lua
	base_class.add_include("<lua.hpp>","Lua");
	base_class.add_include("<lauxlib.h>","Lua");
	base_class.add_include("<lualib.h>","Lua");
}

std::string LuaGenerator::lua_table_name(const std::string& identifier)
{
	return identifier;
}

std::string LuaGenerator::lua_field_name(const std::string& identifier)
{
	return identifier;
}

std::string LuaGenerator::lua_function_name(const std::string& identifier)
{
	return identifier;
}

std::string LuaGenerator::indent_string(int level)
{
	return std::string(level * 2, ' ');
}

std::string LuaGenerator::get_lua_default_value(ProgramStructure *ps, TypeDefinition type)
{
	if (type.is_bool())
	{
		return "false";
	}
	else if (type.is_integer() || type.is_real())
	{
		return "0";
	}
	else if (type.is_string() || type.is_char())
	{
		return "\"\"";
	}
	else if (type.is_array())
	{
		return "{}";
	}
	else if (type.is_struct(ps))
	{
		return "nil";
	}
	else if (type.is_enum(ps))
	{
		// Return the first enum value as default
		try {
			EnumDefinition e = ps->getEnum(type.identifier());
			if (!e.values.empty()) {
				return "\"" + e.values.begin()->first + "\"";
			}
		} catch (...) {
			// Fallback if enum not found
		}
		return "\"Unknown\"";
	}
	return "nil";
}

std::string LuaGenerator::escape_lua_string(std::string str)
{
	// Escape quotes and backslashes for Lua strings
	std::regex backslash_regex("\\\\");
	std::string escaped = std::regex_replace(str, backslash_regex, "\\\\");
	std::regex quote_regex("\"");
	escaped = std::regex_replace(escaped, quote_regex, "\\\"");
	std::regex newline_regex("\n");
	escaped = std::regex_replace(escaped, newline_regex, "\\n");
	std::regex tab_regex("\t");
	escaped = std::regex_replace(escaped, tab_regex, "\\t");
	return escaped;
}

std::string LuaGenerator::lua_type_comment(ProgramStructure *ps, TypeDefinition type)
{
	if (type.is_bool())
	{
		return "boolean";
	}
	else if (type.is_integer() || type.is_real())
	{
		return "number";
	}
	else if (type.is_string() || type.is_char())
	{
		return "string";
	}
	else if (type.is_array())
	{
		return "table (array of " + lua_type_comment(ps, type.element_type()) + ")";
	}
	else if (type.is_struct(ps))
	{
		return "table (" + type.identifier() + ")";
	}
	else if (type.is_enum(ps))
	{
		return "string (enum " + type.identifier() + ")";
	}
	return "any";
}

std::string LuaGenerator::lua_type_to_json_type(TypeDefinition type)
{
	if (type.is_bool())
	{
		return "boolean";
	}
	else if (type.is_integer() || type.is_real())
	{
		return "number";
	}
	else if (type.is_string() || type.is_char())
	{
		return "string";
	}
	else if (type.is_array())
	{
		return "array";
	}
	else
	{
		return "object";
	}
}

std::string LuaGenerator::generate_lua_enum_table(EnumDefinition &e)
{
	std::string lua_code = "-- Enum: " + e.identifier + "\n";
	lua_code += "local " + e.identifier + " = {\n";
	
	for (const auto &value : e.values)
	{
		lua_code += "  " + value.first + " = \"" + value.first + "\",\n";
	}
	
	lua_code += "}\n\n";
	
	// Add validation function
	lua_code += "function " + e.identifier + ".is_valid(value)\n";
	lua_code += "  for _, v in pairs(" + e.identifier + ") do\n";
	lua_code += "    if v == value then\n";
	lua_code += "      return true\n";
	lua_code += "    end\n";
	lua_code += "  end\n";
	lua_code += "  return false\n";
	lua_code += "end\n\n";
	
	return lua_code;
}

void LuaGenerator::generate_lua_field(ProgramStructure *ps, MemberVariableDefinition &mv, std::ofstream &luaFile, int indent)
{
	std::string ind = indent_string(indent);
	luaFile << ind << mv.identifier << " = " << get_lua_default_value(ps, mv.type) << ", -- " << lua_type_comment(ps, mv.type);
	
	if (!mv.description.empty())
	{
		luaFile << " - " << escape_lua_string(mv.description);
	}
	
	luaFile << "\n";
}

void LuaGenerator::generate_lua_constructor(ProgramStructure *ps, StructDefinition &s, std::ofstream &luaFile)
{
	luaFile << "function " << s.getIdentifier() << ".new(data)\n";
	luaFile << "  local instance = {\n";
	
	for (auto& [generator, mv] : s.getMemberVariables())
	{
		if (mv.type.is_array())
		{
			continue; // Skip arrays for now - need special handling
		}
		generate_lua_field(ps, mv, luaFile, 2);
	}
	
	luaFile << "  }\n\n";
	
	luaFile << "  -- Set values from data if provided\n";
	luaFile << "  if data then\n";
	for (auto& [generator, mv] : s.getMemberVariables())
	{
		if (mv.type.is_array())
		{
			continue;
		}
		luaFile << "    if data." << mv.identifier << " ~= nil then\n";
		luaFile << "      instance." << mv.identifier << " = data." << mv.identifier << "\n";
		luaFile << "    end\n";
	}
	luaFile << "  end\n\n";
	
	luaFile << "  setmetatable(instance, " << s.getIdentifier() << ")\n";
	luaFile << "  return instance\n";
	luaFile << "end\n\n";
}

void LuaGenerator::generate_lua_tostring(ProgramStructure *ps, StructDefinition &s, std::ofstream &luaFile)
{
	luaFile << "function " << s.getIdentifier() << ":__tostring()\n";
	luaFile << "  local result = \"" << s.getIdentifier() << " {\\n\"\n";
	
	for (auto& [generator, mv] : s.getMemberVariables())
	{
		if (mv.type.is_array())
		{
			continue;
		}
		
		if (mv.type.is_string() || mv.type.is_char() || mv.type.is_enum(ps))
		{
			luaFile << "  result = result .. \"  " << mv.identifier << " = '\" .. tostring(self." << mv.identifier << ") .. \"',\\n\"\n";
		}
		else
		{
			luaFile << "  result = result .. \"  " << mv.identifier << " = \" .. tostring(self." << mv.identifier << ") .. \",\\n\"\n";
		}
	}
	
	luaFile << "  result = result .. \"}\"\n";
	luaFile << "  return result\n";
	luaFile << "end\n\n";
}

void LuaGenerator::generate_lua_validate(ProgramStructure *ps, StructDefinition &s, std::ofstream &luaFile)
{
	luaFile << "function " << s.getIdentifier() << ":validate()\n";
	luaFile << "  local errors = {}\n\n";
	
	for (auto& [generator, mv] : s.getMemberVariables())
	{
		if (mv.type.is_array())
		{
			continue;
		}
		
		// Check required fields
		if (mv.required)
		{
			luaFile << "  if self." << mv.identifier << " == nil then\n";
			luaFile << "    table.insert(errors, \"" << mv.identifier << " is required\")\n";
			luaFile << "  end\n\n";
		}
		
		// Type validation for enums
		if (mv.type.is_enum(ps))
		{
			luaFile << "  if self." << mv.identifier << " ~= nil and not " << mv.type.identifier() << ".is_valid(self." << mv.identifier << ") then\n";
			luaFile << "    table.insert(errors, \"" << mv.identifier << " has invalid enum value\")\n";
			luaFile << "  end\n\n";
		}
		
		// Type validation for basic types
		if (mv.type.is_string())
		{
			luaFile << "  if self." << mv.identifier << " ~= nil and type(self." << mv.identifier << ") ~= \"string\" then\n";
			luaFile << "    table.insert(errors, \"" << mv.identifier << " must be a string\")\n";
			luaFile << "  end\n\n";
		}
		else if (mv.type.is_integer() || mv.type.is_real())
		{
			luaFile << "  if self." << mv.identifier << " ~= nil and type(self." << mv.identifier << ") ~= \"number\" then\n";
			luaFile << "    table.insert(errors, \"" << mv.identifier << " must be a number\")\n";
			luaFile << "  end\n\n";
		}
		else if (mv.type.is_bool())
		{
			luaFile << "  if self." << mv.identifier << " ~= nil and type(self." << mv.identifier << ") ~= \"boolean\" then\n";
			luaFile << "    table.insert(errors, \"" << mv.identifier << " must be a boolean\")\n";
			luaFile << "  end\n\n";
		}
	}
	
	luaFile << "  return #errors == 0, errors\n";
	luaFile << "end\n\n";
}

std::string LuaGenerator::generate_lua_table_struct(ProgramStructure *ps, StructDefinition &s)
{
	std::string lua_code = "-- Schema: " + s.getIdentifier() + "\n";
	lua_code += "local " + s.getIdentifier() + " = {}\n";
	lua_code += s.getIdentifier() + ".__index = " + s.getIdentifier() + "\n\n";
	
	return lua_code;
}

bool LuaGenerator::generate_struct_lua_file(ProgramStructure *ps, StructDefinition &s, std::string out_path, std::vector<StructDefinition> base_classes)
{
	std::ofstream luaFile(out_path + "/" + s.getIdentifier() + ".lua");
	if (!luaFile.is_open())
	{
		std::cout << "Failed to open file: " << out_path + "/" + s.getIdentifier() + ".lua" << std::endl;
		return false;
	}
	
	// File header
	luaFile << "-- " << s.getIdentifier() << " Schema\n";
	luaFile << "-- Auto-generated Lua schema file\n\n";
	
	// Include dependencies for enum types
	for (auto& [generator, mv] : s.getMemberVariables())
	{
		if (mv.type.is_enum(ps))
		{
			luaFile << "local " << mv.type.identifier() << " = require('" << mv.type.identifier() << "')\n";
		}
		else if (mv.type.is_array() && mv.type.element_type().is_enum(ps))
		{
			luaFile << "local " << mv.type.element_type().identifier() << " = require('" << mv.type.element_type().identifier() << "')\n";
		}
	}
	
	// Include dependencies for base classes
	for (auto &bc : base_classes)
	{
		if (!bc.getIdentifier().empty())
		{
			luaFile << "-- Support for " << bc.getIdentifier() << " generator methods\n";
		}
	}
	
	luaFile << "\n";
	
	// Generate the main table
	luaFile << generate_lua_table_struct(ps, s);
	
	// Generate constructor
	generate_lua_constructor(ps, s, luaFile);
	
	// Generate methods from other generators
	generate_generator_methods(ps, s, base_classes, luaFile);
	
	// Generate tostring method
	generate_lua_tostring(ps, s, luaFile);
	
	// Generate validation method
	generate_lua_validate(ps, s, luaFile);
	
	// Export the module
	luaFile << "return " << s.getIdentifier() << "\n";
	
	luaFile.close();
	return true;
}

bool LuaGenerator::generate_enum_lua_file(EnumDefinition &e, std::string out_path)
{
	std::ofstream luaFile(out_path + "/" + e.identifier + ".lua");
	if (!luaFile.is_open())
	{
		std::cout << "Failed to open file: " << out_path + "/" + e.identifier + ".lua" << std::endl;
		return false;
	}
	
	// File header
	luaFile << "-- " << e.identifier << " Enum\n";
	luaFile << "-- Auto-generated Lua enum file\n\n";
	
	// Generate the enum table
	luaFile << generate_lua_enum_table(e);
	
	// Export the module
	luaFile << "return " << e.identifier << "\n";
	
	luaFile.close();
	return true;
}

bool LuaGenerator::generate_main_lua_file(ProgramStructure &ps, std::string out_path)
{
	std::ofstream luaFile(out_path + "/schema.lua");
	if (!luaFile.is_open())
	{
		std::cout << "Failed to open file: " << out_path + "/schema.lua" << std::endl;
		return false;
	}
	
	// File header
	luaFile << "-- Main Schema Module\n";
	luaFile << "-- Auto-generated Lua schema aggregator\n\n";
	
	luaFile << "local Schema = {}\n\n";
	
	// Require all enums
	luaFile << "-- Enums\n";
	for (auto &e : ps.getEnums())
	{
		luaFile << "Schema." << e.identifier << " = require('" << e.identifier << "')\n";
	}
	luaFile << "\n";
	
	// Require all structs
	luaFile << "-- Structs\n";
	for (auto &s : ps.getStructs())
	{
		luaFile << "Schema." << s.getIdentifier() << " = require('" << s.getIdentifier() << "')\n";
	}
	luaFile << "\n";
	
	// Add utility functions
	luaFile << "-- Utility functions\n";
	luaFile << "function Schema.validate_all(data)\n";
	luaFile << "  local all_valid = true\n";
	luaFile << "  local all_errors = {}\n";
	luaFile << "  \n";
	luaFile << "  for schema_name, schema_data in pairs(data) do\n";
	luaFile << "    if Schema[schema_name] and type(schema_data) == 'table' then\n";
	luaFile << "      local instance = Schema[schema_name].new(schema_data)\n";
	luaFile << "      local valid, errors = instance:validate()\n";
	luaFile << "      if not valid then\n";
	luaFile << "        all_valid = false\n";
	luaFile << "        all_errors[schema_name] = errors\n";
	luaFile << "      end\n";
	luaFile << "    end\n";
	luaFile << "  end\n";
	luaFile << "  \n";
	luaFile << "  return all_valid, all_errors\n";
	luaFile << "end\n\n";
	
	// Export the module
	luaFile << "return Schema\n";
	
	luaFile.close();
	return true;
}

void LuaGenerator::generate_generator_methods(ProgramStructure *ps, StructDefinition &s, std::vector<StructDefinition> &base_classes, std::ofstream &luaFile)
{
	// Generate methods from other generators
	for (auto &bc : base_classes)
	{
		if (bc.getIdentifier().empty()) continue;
		
		for (auto & [generator, f] : bc.getFunctions())
		{		luaFile << "-- Method from " << bc.getIdentifier() << " generator\n";
		luaFile << "function " << s.getIdentifier() << ":" << f.identifier << "(";
		
		// Generate parameters
		for (size_t i = 0; i < f.parameters.size(); i++)
		{
			luaFile << f.parameters[i].second;
			if (i < f.parameters.size() - 1)
			{
				luaFile << ", ";
			}
		}
		
		luaFile << ")\n";
		
		// Generate method body with proper implementations
		if (f.identifier == "toJSON" && bc.getIdentifier() == "Json")
		{
			luaFile << "  -- Convert to JSON string using lua-cjson\n";
			luaFile << "  local cjson = require('cjson')\n";
			luaFile << "  return cjson.encode(self)\n";
		}
		else if (f.identifier == "fromJSON" && bc.getIdentifier() == "Json")
		{
			luaFile << "  -- Load from JSON string using lua-cjson\n";
			luaFile << "  local cjson = require('cjson')\n";
			luaFile << "  local data = cjson.decode(" << f.parameters[0].second << ")\n";
			luaFile << "  for k, v in pairs(data) do\n";
			luaFile << "    self[k] = v\n";
			luaFile << "  end\n";
		}
		else if (f.identifier == "getSchema" && bc.getIdentifier() == "Json")
		{
			luaFile << "  -- Return JSON schema definition\n";
			luaFile << "  return {\n";
			luaFile << "    type = 'object',\n";
			luaFile << "    properties = {\n";
			for (auto& [generator, mv] : s.getMemberVariables())
			{
				luaFile << "      " << mv.identifier << " = { type = '" << lua_type_to_json_type(mv.type) << "' },\n";
			}
			luaFile << "    },\n";
			luaFile << "    required = { ";
			bool first = true;
			for (auto& [generator, mv] : s.getMemberVariables())
			{
				if (mv.required)
				{
					if (!first) luaFile << ", ";
					luaFile << "'" << mv.identifier << "'";
					first = false;
				}
			}
			luaFile << " }\n";
			luaFile << "  }\n";
		}
		// SQLite methods
		else if (f.identifier.find("SQLite") != std::string::npos && bc.getIdentifier() == "Sqlite")
		{
			luaFile << "  -- SQLite operations using luasql-sqlite3\n";
			luaFile << "  local luasql = require('luasql.sqlite3')\n";
			if (f.identifier.find("Insert") != std::string::npos)
			{
				luaFile << "  local env = luasql.sqlite3()\n";
				luaFile << "  local conn = env:connect(" << f.parameters[0].second << ")\n";
				luaFile << "  local sql = \"INSERT INTO " << s.getIdentifier() << " (";
				bool first = true;
				for (auto& [generator, mv] : s.getMemberVariables())
				{
					if (!first) luaFile << ", ";
					luaFile << mv.identifier;
					first = false;
				}
				luaFile << ") VALUES (";
				first = true;
				for (auto& [generator, mv] : s.getMemberVariables())
				{
					if (!first) luaFile << ", ";
					luaFile << "?";
					first = false;
				}
				luaFile << ")\"\n";
				luaFile << "  local stmt = conn:prepare(sql)\n";
				luaFile << "  stmt:execute(";
				first = true;
				for (auto& [generator, mv] : s.getMemberVariables())
				{
					if (!first) luaFile << ", ";
					luaFile << "self." << mv.identifier;
					first = false;
				}
				luaFile << ")\n";
				luaFile << "  conn:close()\n";
				luaFile << "  env:close()\n";
				luaFile << "  return true\n";
			}
			else if (f.identifier.find("Select") != std::string::npos)
			{
				luaFile << "  local env = luasql.sqlite3()\n";
				luaFile << "  local conn = env:connect(" << f.parameters[0].second << ")\n";
				luaFile << "  local sql = \"SELECT * FROM " << s.getIdentifier() << " WHERE \" .. " << f.parameters[1].second << " .. \" = ?\"\n";
				luaFile << "  local cursor = conn:execute(sql, " << f.parameters[1].second << ")\n";
				luaFile << "  local results = {}\n";
				luaFile << "  local row = cursor:fetch({})\n";
				luaFile << "  while row do\n";
				luaFile << "    local instance = " << s.getIdentifier() << ".new({\n";
				int i = 0;
				for (auto& [generator, mv] : s.getMemberVariables())
				{
					luaFile << "      " << mv.identifier << " = row[" << (i+1) << "],\n";
					i++;
				}
				luaFile << "    })\n";
				luaFile << "    table.insert(results, instance)\n";
				luaFile << "    row = cursor:fetch({})\n";
				luaFile << "  end\n";
				luaFile << "  cursor:close()\n";
				luaFile << "  conn:close()\n";
				luaFile << "  env:close()\n";
				luaFile << "  return results\n";
			}
			else
			{
				luaFile << "  -- Generic SQLite operation\n";
				luaFile << "  return nil\n";
			}
		}
		// MySQL methods
		else if (f.identifier.find("MySQL") != std::string::npos && bc.getIdentifier() == "Mysql")
		{
			luaFile << "  -- MySQL operations using luasql-mysql\n";
			luaFile << "  local luasql = require('luasql.mysql')\n";
			if (f.identifier.find("Insert") != std::string::npos)
			{
				luaFile << "  local env = luasql.mysql()\n";
				luaFile << "  local conn = env:connect(" << f.parameters[0].second << ")\n";
				luaFile << "  local sql = \"INSERT INTO " << s.getIdentifier() << " (";
				bool first = true;
				for (auto& [generator, mv] : s.getMemberVariables())
				{
					if (!first) luaFile << ", ";
					luaFile << mv.identifier;
					first = false;
				}
				luaFile << ") VALUES (";
				first = true;
				for (auto& [generator, mv] : s.getMemberVariables())
				{
					if (!first) luaFile << ", ";
					luaFile << "'\" .. tostring(self." << mv.identifier << ") .. \"'";
					first = false;
				}
				luaFile << ")\"\n";
				luaFile << "  local result = conn:execute(sql)\n";
				luaFile << "  conn:close()\n";
				luaFile << "  env:close()\n";
				luaFile << "  return result\n";
			}
			else
			{
				luaFile << "  -- Generic MySQL operation\n";
				luaFile << "  return nil\n";
			}
		}
		// Java integration methods
		else if (bc.getIdentifier() == "Java")
		{
			if (f.identifier == "validate")
			{
				luaFile << "  -- Enhanced validation with Java-style checks\n";
				luaFile << "  local errors = {}\n";
				for (auto& [generator, mv] : s.getMemberVariables())
				{
					luaFile << "  if self." << mv.identifier << " == nil then\n";
					luaFile << "    table.insert(errors, '" << mv.identifier << " is required')\n";
					luaFile << "  end\n";
					if (mv.type.is_string())
					{
						luaFile << "  if self." << mv.identifier << " and self." << mv.identifier << " == '' then\n";
						luaFile << "    table.insert(errors, '" << mv.identifier << " cannot be empty')\n";
						luaFile << "  end\n";
					}
				}
				luaFile << "  return #errors == 0, errors\n";
			}
			else if (f.identifier == "clone")
			{
				luaFile << "  -- Create a deep copy of this object\n";
				luaFile << "  local copy = " << s.getIdentifier() << ".new()\n";
				for (auto& [generator, mv] : s.getMemberVariables())
				{
					if (mv.type.is_array())
					{
						luaFile << "  copy." << mv.identifier << " = {}\n";
						luaFile << "  for i, v in ipairs(self." << mv.identifier << ") do\n";
						luaFile << "    copy." << mv.identifier << "[i] = v\n";
						luaFile << "  end\n";
					}
					else
					{
						luaFile << "  copy." << mv.identifier << " = self." << mv.identifier << "\n";
					}
				}
				luaFile << "  return copy\n";
			}
			else if (f.identifier == "toJson")
			{
				luaFile << "  -- Convert to JSON using lua-cjson (alias for Jackson-style)\n";
				luaFile << "  local cjson = require('cjson')\n";
				luaFile << "  return cjson.encode(self)\n";
			}
			else if (f.identifier == "fromJson")
			{
				luaFile << "  -- Parse from JSON using lua-cjson (alias for Jackson-style)\n";
				luaFile << "  local cjson = require('cjson')\n";
				luaFile << "  local data = cjson.decode(" << f.parameters[0].second << ")\n";
				luaFile << "  local instance = " << s.getIdentifier() << ".new(data)\n";
				luaFile << "  return instance\n";
			}
			else if (f.identifier == "saveToDatabase")
			{
				luaFile << "  -- Save to database using connection object\n";
				luaFile << "  -- This would require a Lua database wrapper\n";
				luaFile << "  error('Database operations require proper connection setup')\n";
			}
			else if (f.identifier == "loadFromDatabase")
			{
				luaFile << "  -- Load from database using connection object\n";
				luaFile << "  -- This would require a Lua database wrapper\n";
				luaFile << "  error('Database operations require proper connection setup')\n";
			}
			else if (f.identifier == "toLuaTable")
			{
				luaFile << "  -- Convert to Lua table representation\n";
				luaFile << "  return {\n";
				for (auto& [generator, mv] : s.getMemberVariables())
				{
					luaFile << "    " << mv.identifier << " = self." << mv.identifier << ",\n";
				}
				luaFile << "  }\n";
			}
			else if (f.identifier == "fromLuaTable")
			{
				luaFile << "  -- Create instance from Lua table\n";
				luaFile << "  return " << s.getIdentifier() << ".new(" << f.parameters[0].second << ")\n";
			}
			else
			{
				luaFile << "  -- Java integration method (requires JNI setup)\n";
				luaFile << "  error('Java integration requires proper JNI environment')\n";
			}
		}
		// C++ integration methods
		else if (bc.getIdentifier() == "Cpp")
		{
			if (f.identifier == "to_java_object")
			{
				luaFile << "  -- Convert to Java object (requires JNI setup)\n";
				luaFile << "  error('Java object conversion requires JNI environment')\n";
			}
			else if (f.identifier == "from_java_object")
			{
				luaFile << "  -- Populate from Java object (requires JNI setup)\n";
				luaFile << "  error('Java object conversion requires JNI environment')\n";
			}
			else if (f.identifier == "create_java_class")
			{
				luaFile << "  -- Create Java class (requires JNI setup)\n";
				luaFile << "  error('Java class creation requires JNI environment')\n";
			}
			else
			{
				luaFile << "  -- C++ integration method\n";
				luaFile << "  error('C++ integration requires proper native setup')\n";
			}
		}
		else
		{
			luaFile << "  -- Generic method implementation\n";
			if (f.return_type.is_bool())
			{
				luaFile << "  return false\n";
			}
			else if (f.return_type.is_integer() || f.return_type.is_real())
			{
				luaFile << "  return 0\n";
			}
			else if (f.return_type.is_string())
			{
				luaFile << "  return ''\n";
			}
			else if (f.return_type.identifier() != "void")
			{
				luaFile << "  return nil\n";
			}
		}
		
		luaFile << "end\n\n";
		}
	}
}

bool LuaGenerator::add_generator(Generator *gen)
{
	generators.push_back(gen);
	return true;
}

std::string LuaGenerator::convert_to_local_type(ProgramStructure *ps, TypeDefinition type)
{
	// Lua is dynamically typed, so we return descriptive comments for documentation
	return lua_type_comment(ps, type);
}

bool LuaGenerator::add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	// Lua generator doesn't add content to C++ structs
	return true;
}

bool LuaGenerator::generate_files(ProgramStructure ps, std::string out_path)
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
		if (!gen->base_class.getIdentifier().empty())
		{
			base_classes.push_back(gen->base_class);
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
		if (!generate_enum_lua_file(e, out_path))
		{
			std::cout << "Failed to generate Lua file for enum: " << e.identifier << std::endl;
			return false;
		}
	}
	
	// Generate struct files with base classes
	for (auto &s : ps.getStructs())
	{
		if (!generate_struct_lua_file(&ps, s, out_path, base_classes))
		{
			std::cout << "Failed to generate Lua file for struct: " << s.getIdentifier() << std::endl;
			return false;
		}
	}
	
	// Generate main aggregator file
	if (!generate_main_lua_file(ps, out_path))
	{
		std::cout << "Failed to generate main Lua schema file" << std::endl;
		return false;
	}
	
	return true;
}
