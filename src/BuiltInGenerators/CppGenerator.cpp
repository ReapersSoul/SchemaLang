#include <BuiltInGenerators/CppGenerator.hpp>

bool CppGenerator::generate_base_class_header_file(Generator *gen, ProgramStructure *ps, std::string out_path)
{
	std::ofstream baseClassFile(out_path + "/Has" + gen->base_class.identifier + "Schema.hpp");
	if (!baseClassFile.is_open())
	{
		std::cout << "Failed to open file: " << out_path + "/Has" + gen->base_class.identifier + "Schema.hpp" << std::endl;
		return false;
	}
	baseClassFile << "#pragma once\n";
	for (auto &include : gen->base_class.includes)
	{
		baseClassFile << "#include " << include.second << "\n";
	}
	for (auto &line : gen->base_class.before_lines)
	{
		baseClassFile << line.second << "\n";
	}
	baseClassFile << "class Has" + gen->base_class.identifier + "Schema{\n";
	baseClassFile << "public:\n";
	for (auto &f : gen->base_class.functions)
	{
		baseClassFile << "\tvirtual " << convert_to_local_type(ps, f.return_type) << " " << f.identifier << "(";
		for (int i = 0; i < f.parameters.size(); i++)
		{
			baseClassFile << convert_to_local_type(ps, f.parameters[i].first) << " " << f.parameters[i].second;
			if (i < f.parameters.size() - 1)
			{
				baseClassFile << ", ";
			}
		}
		baseClassFile << ") = 0;\n";
	}
	baseClassFile << "};\n";
	baseClassFile.close();
	return true;
}

CppGenerator::CppGenerator()
{
	name="Cpp";
	base_class.identifier = "Cpp";
	base_class.includes.insert({"Java", "<jni.h>"});

	// Add Java integration methods
	FunctionDefinition to_java_object;
	to_java_object.generator = "Cpp";
	to_java_object.identifier = "to_java_object";
	to_java_object.return_type.identifier() = "jobject";
	to_java_object.parameters.push_back(std::make_pair(TypeDefinition("JNIEnv*"), "env"));
	to_java_object.parameters.push_back(std::make_pair(TypeDefinition("jclass"), "clazz"));
	to_java_object.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\t// Create Java object from C++ object\n";
		structFile << "\tjmethodID constructor = env->GetMethodID(clazz, \"<init>\", \"()V\");\n";
		structFile << "\tif (!constructor) return nullptr;\n";
		structFile << "\tjobject jobj = env->NewObject(clazz, constructor);\n";
		structFile << "\tif (!jobj) return nullptr;\n\n";

		for (auto &mv : s.member_variables)
		{
			std::string setter_name = "set" + mv.identifier;
			setter_name[3] = std::toupper(setter_name[3]);

			structFile << "\t// Set field: " << mv.identifier << "\n";

			if (mv.type.is_integer())
			{
				structFile << "\tjmethodID " << mv.identifier << "_setter = env->GetMethodID(clazz, \"" << setter_name << "\", \"(I)V\");\n";
				structFile << "\tif (" << mv.identifier << "_setter) {\n";
				structFile << "\t\tenv->CallVoidMethod(jobj, " << mv.identifier << "_setter, (jint)" << mv.identifier << ");\n";
				structFile << "\t}\n\n";
			}
			else if (mv.type.is_real())
			{
				if (mv.type.identifier() == "float")
				{
					structFile << "\tjmethodID " << mv.identifier << "_setter = env->GetMethodID(clazz, \"" << setter_name << "\", \"(F)V\");\n";
					structFile << "\tif (" << mv.identifier << "_setter) {\n";
					structFile << "\t\tenv->CallVoidMethod(jobj, " << mv.identifier << "_setter, (jfloat)" << mv.identifier << ");\n";
					structFile << "\t}\n\n";
				}
				else
				{
					structFile << "\tjmethodID " << mv.identifier << "_setter = env->GetMethodID(clazz, \"" << setter_name << "\", \"(D)V\");\n";
					structFile << "\tif (" << mv.identifier << "_setter) {\n";
					structFile << "\t\tenv->CallVoidMethod(jobj, " << mv.identifier << "_setter, (jdouble)" << mv.identifier << ");\n";
					structFile << "\t}\n\n";
				}
			}
			else if (mv.type.is_bool())
			{
				structFile << "\tjmethodID " << mv.identifier << "_setter = env->GetMethodID(clazz, \"" << setter_name << "\", \"(Z)V\");\n";
				structFile << "\tif (" << mv.identifier << "_setter) {\n";
				structFile << "\t\tenv->CallVoidMethod(jobj, " << mv.identifier << "_setter, (jboolean)" << mv.identifier << ");\n";
				structFile << "\t}\n\n";
			}
			else if (mv.type.is_string())
			{
				structFile << "\tjstring j" << mv.identifier << " = env->NewStringUTF(" << mv.identifier << ".c_str());\n";
				structFile << "\tjmethodID " << mv.identifier << "_setter = env->GetMethodID(clazz, \"" << setter_name << "\", \"(Ljava/lang/String;)V\");\n";
				structFile << "\tif (" << mv.identifier << "_setter && j" << mv.identifier << ") {\n";
				structFile << "\t\tenv->CallVoidMethod(jobj, " << mv.identifier << "_setter, j" << mv.identifier << ");\n";
				structFile << "\t\tenv->DeleteLocalRef(j" << mv.identifier << ");\n";
				structFile << "\t}\n\n";
			}
			else if (mv.type.is_array())
			{
				structFile << "\t// Convert array " << mv.identifier << " to Java array\n";
				if (mv.type.element_type().is_integer())
				{
					structFile << "\tjintArray j" << mv.identifier << " = env->NewIntArray(" << mv.identifier << ".size());\n";
					structFile << "\tif (j" << mv.identifier << ") {\n";
					structFile << "\t\tfor (size_t i = 0; i < " << mv.identifier << ".size(); i++) {\n";
					structFile << "\t\t\tjint val = " << mv.identifier << "[i];\n";
					structFile << "\t\t\tenv->SetIntArrayRegion(j" << mv.identifier << ", i, 1, &val);\n";
					structFile << "\t\t}\n";
					structFile << "\t\tjmethodID " << mv.identifier << "_setter = env->GetMethodID(clazz, \"" << setter_name << "\", \"([I)V\");\n";
					structFile << "\t\tif (" << mv.identifier << "_setter) {\n";
					structFile << "\t\t\tenv->CallVoidMethod(jobj, " << mv.identifier << "_setter, j" << mv.identifier << ");\n";
					structFile << "\t\t}\n";
					structFile << "\t\tenv->DeleteLocalRef(j" << mv.identifier << ");\n";
					structFile << "\t}\n\n";
				}
				else if (mv.type.element_type().is_string())
				{
					structFile << "\tjobjectArray j" << mv.identifier << " = env->NewObjectArray(" << mv.identifier << ".size(), env->FindClass(\"java/lang/String\"), nullptr);\n";
					structFile << "\tif (j" << mv.identifier << ") {\n";
					structFile << "\t\tfor (size_t i = 0; i < " << mv.identifier << ".size(); i++) {\n";
					structFile << "\t\t\tjstring jstr = env->NewStringUTF(" << mv.identifier << "[i].c_str());\n";
					structFile << "\t\t\tenv->SetObjectArrayElement(j" << mv.identifier << ", i, jstr);\n";
					structFile << "\t\t\tenv->DeleteLocalRef(jstr);\n";
					structFile << "\t\t}\n";
					structFile << "\t\tjmethodID " << mv.identifier << "_setter = env->GetMethodID(clazz, \"" << setter_name << "\", \"([Ljava/lang/String;)V\");\n";
					structFile << "\t\tif (" << mv.identifier << "_setter) {\n";
					structFile << "\t\t\tenv->CallVoidMethod(jobj, " << mv.identifier << "_setter, j" << mv.identifier << ");\n";
					structFile << "\t\t}\n";
					structFile << "\t\tenv->DeleteLocalRef(j" << mv.identifier << ");\n";
					structFile << "\t}\n\n";
				}
			}
		}

		structFile << "\treturn jobj;\n";
		return true;
	};

	FunctionDefinition from_java_object;
	from_java_object.generator = "Cpp";
	from_java_object.identifier = "from_java_object";
	from_java_object.return_type.identifier() = "void";
	from_java_object.parameters.push_back(std::make_pair(TypeDefinition("JNIEnv*"), "env"));
	from_java_object.parameters.push_back(std::make_pair(TypeDefinition("jobject"), "jobj"));
	from_java_object.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\t// Populate C++ object from Java object\n";
		structFile << "\tif (!jobj) return;\n";
		structFile << "\tjclass clazz = env->GetObjectClass(jobj);\n";
		structFile << "\tif (!clazz) return;\n\n";

		for (auto &mv : s.member_variables)
		{
			std::string getter_name = "get" + mv.identifier;
			getter_name[3] = std::toupper(getter_name[3]);

			structFile << "\t// Get field: " << mv.identifier << "\n";

			if (mv.type.is_integer())
			{
				structFile << "\tjmethodID " << mv.identifier << "_getter = env->GetMethodID(clazz, \"" << getter_name << "\", \"()I\");\n";
				structFile << "\tif (" << mv.identifier << "_getter) {\n";
				structFile << "\t\t" << mv.identifier << " = env->CallIntMethod(jobj, " << mv.identifier << "_getter);\n";
				structFile << "\t}\n\n";
			}
			else if (mv.type.is_real())
			{
				if (mv.type.identifier() == "float")
				{
					structFile << "\tjmethodID " << mv.identifier << "_getter = env->GetMethodID(clazz, \"" << getter_name << "\", \"()F\");\n";
					structFile << "\tif (" << mv.identifier << "_getter) {\n";
					structFile << "\t\t" << mv.identifier << " = env->CallFloatMethod(jobj, " << mv.identifier << "_getter);\n";
					structFile << "\t}\n\n";
				}
				else
				{
					structFile << "\tjmethodID " << mv.identifier << "_getter = env->GetMethodID(clazz, \"" << getter_name << "\", \"()D\");\n";
					structFile << "\tif (" << mv.identifier << "_getter) {\n";
					structFile << "\t\t" << mv.identifier << " = env->CallDoubleMethod(jobj, " << mv.identifier << "_getter);\n";
					structFile << "\t}\n\n";
				}
			}
			else if (mv.type.is_bool())
			{
				structFile << "\tjmethodID " << mv.identifier << "_getter = env->GetMethodID(clazz, \"" << getter_name << "\", \"()Z\");\n";
				structFile << "\tif (" << mv.identifier << "_getter) {\n";
				structFile << "\t\t" << mv.identifier << " = env->CallBooleanMethod(jobj, " << mv.identifier << "_getter);\n";
				structFile << "\t}\n\n";
			}
			else if (mv.type.is_string())
			{
				structFile << "\tjmethodID " << mv.identifier << "_getter = env->GetMethodID(clazz, \"" << getter_name << "\", \"()Ljava/lang/String;\");\n";
				structFile << "\tif (" << mv.identifier << "_getter) {\n";
				structFile << "\t\tjstring j" << mv.identifier << " = (jstring)env->CallObjectMethod(jobj, " << mv.identifier << "_getter);\n";
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
		}

		structFile << "\tenv->DeleteLocalRef(clazz);\n";
		return true;
	};

	FunctionDefinition create_java_class;
	create_java_class.generator = "Cpp";
	create_java_class.identifier = "create_java_class";
	create_java_class.return_type.identifier() = "jclass";
	create_java_class.static_function = true;
	create_java_class.parameters.push_back(std::make_pair(TypeDefinition("JNIEnv*"), "env"));
	create_java_class.parameters.push_back(std::make_pair(TypeDefinition("const std::string&"), "class_name"));
	create_java_class.generate_function = [](Generator *gen, ProgramStructure *ps, StructDefinition &s, FunctionDefinition &fd, std::ostream &structFile)
	{
		structFile << "\t// Create and register Java class for this C++ schema\n";
		structFile << "\tstd::string full_class_name = class_name;\n";
		structFile << "\t// Replace :: with / for Java class path\n";
		structFile << "\tsize_t pos = 0;\n";
		structFile << "\twhile ((pos = full_class_name.find(\"::\", pos)) != std::string::npos) {\n";
		structFile << "\t\tfull_class_name.replace(pos, 2, \"/\");\n";
		structFile << "\t\tpos += 1;\n";
		structFile << "\t}\n";
		structFile << "\treturn env->FindClass(full_class_name.c_str());\n";
		return true;
	};

	base_class.functions.push_back(to_java_object);
	base_class.functions.push_back(from_java_object);
	base_class.functions.push_back(create_java_class);
}

bool CppGenerator::add_generator(Generator *gen)
{
	generators.push_back(gen);
	return true;
}

std::string CppGenerator::convert_to_local_type(ProgramStructure *ps, TypeDefinition type)
{
	// convert int types to "int"
	if (type.identifier() == INT8)
	{
		return "int8_t";
	}
	if (type.identifier() == INT16)
	{
		return "int16_t";
	}
	if (type.identifier() == INT32)
	{
		return "int32_t";
	}
	if (type.identifier() == INT64)
	{
		return "int64_t";
	}
	if (type.identifier() == UINT8)
	{
		return "uint8_t";
	}
	if (type.identifier() == UINT16)
	{
		return "uint16_t";
	}
	if (type.identifier() == UINT32)
	{
		return "uint32_t";
	}
	if (type.identifier() == UINT64)
	{
		return "uint64_t";
	}
	// convert float types to "float"
	if (type.identifier() == FLOAT)
	{
		return "float";
	}
	if (type.identifier() == DOUBLE)
	{
		return "double";
	}
	// convert bool to "bool"
	if (type.identifier() == BOOL)
	{
		return "bool";
	}
	// convert string to "std::string"
	if (type.identifier() == STRING)
	{
		return "std::string";
	}
	// convert char to "char"
	if (type.identifier() == CHAR)
	{
		return "char";
	}
	// convert array to "std::vector"f.parameters[i].first
	if (type.identifier() == ARRAY)
	{
		return "std::vector<" + convert_to_local_type(ps, type.element_type()) + ">";
	}

	// if the type is a enum, return the identifier
	if (ps->tokenIsEnum(type.identifier()))
	{
		return type.identifier() + "Schema";
	}

	// if the type is a struct, return the identifier
	if (ps->tokenIsStruct(type.identifier()))
	{
		return type.identifier() + "Schema *";
	}

	return type.identifier();
}

std::string CppGenerator::format_include(const std::string& filename) const
{
	std::string full_path = include_prefix.empty() ? filename : include_prefix + "/" + filename;
	if (use_angle_brackets)
	{
		return "<" + full_path + ">";
	}
	else
	{
		return "\"" + full_path + "\"";
	}
}

bool CppGenerator::add_generator_specific_content_to_struct(Generator *gen, ProgramStructure *ps, StructDefinition &s)
{
	return true;
}

#include <inja/inja.hpp>
#include <EmbeddedResources/EmbeddedResourcesEmbeddedVFS.hpp>

bool CppGenerator::generate_files(ProgramStructure ps, std::string out_path)
{
	if (!std::filesystem::exists(out_path))
	{
		std::filesystem::create_directories(out_path);
	}
	// generate the base class header files
	std::vector<StructDefinition> base_classes;

	for (auto &gen : generators)
	{
		if (gen == this)
		{          
			continue;
		}
		if (!gen->base_class.identifier.empty())
		{
			if (!generate_base_class_header_file(gen, &ps, out_path))
			{
				printf("Error: Failed to generate base class header file for %s\n", gen->base_class.identifier.c_str());
				return false;
			}
			base_classes.push_back(gen->base_class);
		}

		for (auto &s : ps.getStructs())
		{
			if (!gen->add_generator_specific_content_to_struct(this, &ps, s))
			{
				printf("Error: Failed to add Generator specific functions for %s\n", gen->base_class.identifier.c_str());
				return false;
			}
		}
	}

	inja::Environment env;
	env.set_trim_blocks(true);
	// env.set_lstrip_blocks(true);

	std::map<std::string, std::string> struct_name_content_pairs;
	// open file
	std::vector<std::string> files = listEmbeddedResourcesEmbeddedFiles("/Cpp/struct/");
	for (auto &file : files)
	{
		std::string content(reinterpret_cast<const char *>(loadEmbeddedResourcesEmbeddedFile(("/Cpp/struct/" + file).c_str()).data()), loadEmbeddedResourcesEmbeddedFile(("/Cpp/struct/" + file).c_str()).size());
		std::string filename = std::filesystem::path(file).filename().string();
		struct_name_content_pairs[filename] = content;
	}

	std::map<std::string, std::string> enum_name_content_pairs;
	files = listEmbeddedResourcesEmbeddedFiles("/Cpp/enum/");
	for (auto &file : files)
	{
		std::string content(reinterpret_cast<const char *>(loadEmbeddedResourcesEmbeddedFile(("/Cpp/enum/" + file).c_str()).data()), loadEmbeddedResourcesEmbeddedFile(("/Cpp/enum/" + file).c_str()).size());
		std::string filename = std::filesystem::path(file).filename().string();
		enum_name_content_pairs[filename] = content;
	}

	for (auto &s : ps.getStructs())
	{
		inja::json data;
		data["struct"] = s.identifier;
		data["struct_include"] = format_include(s.identifier + "Schema.hpp");

		data["generators"] = inja::json::object();

		data["functions"] = inja::json::array();
		for (auto &f : s.functions)
		{

			inja::json function_data;
			function_data["identifier"] = f.identifier;
			function_data["return_type"] = convert_to_local_type(&ps, f.return_type);
			function_data["static"] = f.static_function;
			function_data["parameters"] = inja::json::array();
			for (auto &p : f.parameters)
			{
				inja::json parameter_data;
				parameter_data["type"] = convert_to_local_type(&ps, p.first);
				parameter_data["identifier"] = p.second;
				function_data["parameters"].push_back(parameter_data);
			}
			function_data["can_generate_function"] = f.generate_function != nullptr;

			// add the function to the data
			if (f.generate_function)
			{
				std::stringstream ss;
				f.generate_function(this, &ps, s, f, ss);
				function_data["generate_function"] = ss.str();
			}
			if (f.generator.empty())
			{
				data["functions"].push_back(function_data);
			}
			else
			{
				if (data["generators"].find(f.generator) == data["generators"].end())
				{
					data["generators"][f.generator] = inja::json::object();
					data["generators"][f.generator]["functions"] = inja::json::array();
				}

				data["generators"][f.generator]["functions"].push_back(function_data);
			}
		}

		data["includes"] = inja::json::array();
		for (auto &include : s.includes)
		{
			if (include.first.empty())
			{
				data["includes"].push_back(include.second);
			}
			else
			{
				if (data["generators"].find(include.first) == data["generators"].end())
				{
					data["generators"][include.first] = inja::json::object();
					data["generators"][include.first]["includes"] = inja::json::array();
				}
				data["generators"][include.first]["includes"].push_back(include.second);
			}
		}

		data["base_classes"] = inja::json::array();
        for (auto &bc : base_classes)
        {
            if (bc.identifier.empty())
            {
                continue;
            }
            inja::json base_class_data;
            base_class_data["identifier"] = bc.identifier;
            base_class_data["formatted_include"] = format_include("Has" + bc.identifier + "Schema.hpp");
            
            // Add functions from base class
            base_class_data["functions"] = inja::json::array();
            for (auto &f : bc.functions)
            {
                inja::json function_data;
                function_data["identifier"] = f.identifier;
                function_data["return_type"] = convert_to_local_type(&ps, f.return_type);
                function_data["static"] = f.static_function;
                function_data["parameters"] = inja::json::array();
                for (auto &p : f.parameters)
                {
                    inja::json parameter_data;
                    parameter_data["type"] = convert_to_local_type(&ps, p.first);
                    parameter_data["identifier"] = p.second;
                    function_data["parameters"].push_back(parameter_data);
                }
                function_data["can_generate_function"] = f.generate_function != nullptr;
				if (f.generate_function)
				{
					std::stringstream ss;
					f.generate_function(this, &ps, s, f, ss);
					function_data["generate_function"] = ss.str();
				}
                function_data["is_override"] = true;
                
                base_class_data["functions"].push_back(function_data);
            }
            
            // Add includes from base class
            base_class_data["includes"] = inja::json::array();
            for (auto &include : bc.includes)
            {
                base_class_data["includes"].push_back(include.second);
            }
            
            data["base_classes"].push_back(base_class_data);
        }

		// Add schema includes for member variable types
		data["schema_includes"] = inja::json::array();
		for (auto &mv : s.member_variables)
		{
			// Include the header file for the member variable type if it is a struct or enum
			if (ps.tokenIsStruct(mv.type.identifier()) || ps.tokenIsEnum(mv.type.identifier()))
			{
				data["schema_includes"].push_back(format_include(mv.type.identifier() + "Schema.hpp"));
			}
			if (mv.type.identifier() == ARRAY)
			{
				if (ps.tokenIsStruct(mv.type.element_type().identifier()) || ps.tokenIsEnum(mv.type.element_type().identifier()))
				{
					data["schema_includes"].push_back(format_include(mv.type.element_type().identifier() + "Schema.hpp"));
				}
			}
		}

		data["member_variables"] = inja::json::array();
		for (auto &mv : s.member_variables)
		{
			inja::json mv_data;
			mv_data["identifier"] = mv.identifier;
			mv_data["type"] = convert_to_local_type(&ps, mv.type);
			mv_data["static"] = mv.static_member;
			mv_data["required"] = mv.required;
			if (mv.default_value.empty())
			{
				mv_data["default_value"] = false;
			}
			else
			{
				mv_data["default_value"] = mv.default_value;
			}
			data["member_variables"].push_back(mv_data);
		}
		data["private_variables"] = inja::json::array();
		for (auto &pv : s.private_variables)
		{
			inja::json pv_data;
			pv_data["identifier"] = pv.identifier;
			pv_data["type"] = convert_to_local_type(&ps, pv.type);
			pv_data["static"] = pv.static_member;
			pv_data["const"] = pv.const_member;
			data["private_variables"].push_back(pv_data);
		}

		//nested template data
		data["before_setter_lines"] = inja::json::array();
		for (auto &line : s.before_setter_lines)
		{
			inja::json line_data;
			line_data["line"] = env.render(line.second, data);
			data["before_setter_lines"].push_back(line_data);
		}
		data["before_getter_lines"] = inja::json::array();
		for (auto &line : s.before_getter_lines)
		{
			inja::json line_data;
			line_data["line"] = env.render(line.second, data);
			data["before_getter_lines"].push_back(line_data);
		}

		try
		{
			for (auto &file : struct_name_content_pairs)
			{
				std::ofstream of(env.render(out_path + "/" + file.first, data));
				if (!of.is_open())
				{
					std::cout << "Failed to open file: " << env.render(out_path + "/" + file.first, data);
				}
				of << env.render(file.second, data);
				of.close();
				std::cout << "Generated file: " << env.render(out_path + "/" + file.first, data) << std::endl;
			}
		}
		catch (const std::exception &e)
		{
			std::cout << "Error generating file for struct " << s.identifier << ": " << e.what() << std::endl;
			return false;
		}
	}

	for (auto &e : ps.getEnums())
	{
		inja::json data;
		data["enum"] = e.identifier;
		data["enum_include"] = format_include(e.identifier + "Schema.hpp");

		data["values"] = inja::json::array();
		for (auto &v : e.values)
		{
			inja::json value_data;
			value_data["identifier"] = v.first;
			value_data["value"] = v.second;
			data["values"].push_back(value_data);
		}

		try
		{
			for (auto &file : enum_name_content_pairs)
			{
				std::ofstream of(env.render(out_path + "/" + file.first, data));
				if (!of.is_open())
				{
					std::cout << "Failed to open file: " << env.render(out_path + "/" + file.first, data);
				}
				of << env.render(file.second, data);
				of.close();
				std::cout << "Generated file: " << env.render(out_path + "/" + file.first, data) << std::endl;
			}
		}
		catch (const std::exception &ex)
		{
			std::cout << "Error generating file for enum " << e.identifier << ": " << ex.what() << std::endl;
			return false;
		}
	}

	return true;
}
