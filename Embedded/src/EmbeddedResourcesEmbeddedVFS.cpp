#include "EmbeddedResourcesEmbeddedVFS.hpp"
static unsigned char* inMemFilesystem = nullptr;
static unsigned int inMemFilesystemSize = 0;

unsigned char* decompressEmbeddedResourcesZipInMemory(unsigned char* inputBuffer, unsigned int inputBufferSize, unsigned int &outputSize)
{
	size_t const cBuffOutSize = ZSTD_getFrameContentSize(inputBuffer, inputBufferSize);
	unsigned char* zipBuffer = (unsigned char*)malloc(cBuffOutSize);
	size_t const cSize = ZSTD_decompress(zipBuffer, cBuffOutSize, inputBuffer, inputBufferSize);
	zipBuffer = (unsigned char*)realloc(zipBuffer, cSize);
	outputSize = cSize;
	return zipBuffer;
}

bool initEmbeddedResourcesEmbeddedVFS(char* programName)
{
	static bool isInitialized = false;
	if (isInitialized)
	{
	    return true;
	}
	if (PHYSFS_init(programName) == 0)
	{
	    std::cerr << "Failed to initialize PhysFS: " << PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()) << std::endl;
	    isInitialized = false;
	}
	else
	{
	    isInitialized = true;
	}
	return isInitialized;
}

bool mountEmbeddedResourcesEmbeddedVFS()
{
	unsigned int outputSize = 0;
	inMemFilesystem = decompressEmbeddedResourcesZipInMemory(getEmbeddedResourcesMemoryFile(), getEmbeddedResourcesMemoryFileSize(), outputSize);
	inMemFilesystemSize = outputSize;
	if (PHYSFS_mountMemory(inMemFilesystem, inMemFilesystemSize, nullptr, "/", "/", 0) == 0)
	{
		std::cerr << "Failed to mount memory filesystem: " << PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()) << std::endl;
		return false;
	}
	return true;
}

std::vector<std::string> listEmbeddedResourcesEmbeddedFiles(const char* path)
{
	std::vector<std::string> files;
	char **rc = PHYSFS_enumerateFiles(path);
	char **i;
	for (i = rc; *i != nullptr; i++)
	{
		files.push_back(*i);
	}
	PHYSFS_freeList(rc);
	return files;
}

std::vector<unsigned char> loadEmbeddedResourcesEmbeddedFile(const char* path)
{
	PHYSFS_file* file = PHYSFS_openRead(path);
	if (file == nullptr)
	{
		throw std::runtime_error("Failed to open file: " + std::string(path) + ": " + PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}
	PHYSFS_sint64 fileSize = PHYSFS_fileLength(file);
	std::vector<unsigned char> buffer(fileSize);
	PHYSFS_read(file, buffer.data(), 1, fileSize);
	PHYSFS_close(file);
	return buffer;
}

std::string loadEmbeddedResourcesEmbeddedFileAsString(const char* path)
{
	PHYSFS_file* file = PHYSFS_openRead(path);
	if (file == nullptr)
	{
		throw std::runtime_error("Failed to open file: " + std::string(path) + ": " + PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}
	PHYSFS_sint64 fileSize = PHYSFS_fileLength(file);
	std::vector<char> buffer(fileSize);
	PHYSFS_read(file, buffer.data(), 1, fileSize);
	PHYSFS_close(file);
	return std::string(buffer.begin(), buffer.end());
}

bool existsEmbeddedResourcesEmbeddedFile(const char* path)
{
	return PHYSFS_exists(path) != 0;
}
