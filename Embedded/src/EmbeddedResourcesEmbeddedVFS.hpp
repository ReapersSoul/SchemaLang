#pragma once
#include "EmbeddedResources.hpp"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <zstd.h>
#include <physfs.h>
#include <vector>
#include <string>
bool initEmbeddedResourcesEmbeddedVFS(char* programName);
unsigned char* decompressEmbeddedResourcesZipInMemory(unsigned char* inputBuffer, unsigned int inputBufferSize);
bool mountEmbeddedResourcesEmbeddedVFS();
std::vector<std::string> listEmbeddedResourcesEmbeddedFiles(const char* path);
std::vector<unsigned char> loadEmbeddedResourcesEmbeddedFile(const char* path);
std::string loadEmbeddedResourcesEmbeddedFileAsString(const char* path);
bool existsEmbeddedResourcesEmbeddedFile(const char* path);
