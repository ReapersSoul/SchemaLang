#pragma once
#include "SchemaLangShared_Resources.hpp"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <zstd.h>
#include <physfs.h>
#include <vector>
#include <string>
bool initSchemaLangShared_ResourcesEmbeddedVFS(char* programName);
unsigned char* decompressSchemaLangShared_ResourcesZipInMemory(unsigned char* inputBuffer, unsigned int inputBufferSize);
bool mountSchemaLangShared_ResourcesEmbeddedVFS();
std::vector<std::string> listSchemaLangShared_ResourcesEmbeddedFiles(const char* path, bool fullPath = false);
std::vector<std::string> listSchemaLangShared_ResourcesEmbeddedFilesRelativeTo(const char* path, const char* relativeTo);
std::vector<unsigned char> loadSchemaLangShared_ResourcesEmbeddedFile(const char* path);
std::string loadSchemaLangShared_ResourcesEmbeddedFileAsString(const char* path);
bool existsSchemaLangShared_ResourcesEmbeddedFile(const char* path);
bool isSchemaLangShared_ResourcesEmbeddedFolder(const char* path);
bool isSchemaLangShared_ResourcesEmbeddedFile(const char* path);
bool extractSchemaLangShared_ResourcesTo(const char* embeddedPath, const char* realPath);
