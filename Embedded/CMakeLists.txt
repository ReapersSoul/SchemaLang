project(EmbeddedResources)

# Set highest optimization level with different flags for different compilers
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /Ob2 /DNDEBUG")
endif()

add_custom_target(${PROJECT_NAME}_source
	COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/../embedded_resources
	COMMAND zip -r - . | ${CMAKE_CURRENT_SOURCE_DIR}/EmbeddingCompiler --outputDirectory=${CMAKE_CURRENT_SOURCE_DIR}/src --fileName=${PROJECT_NAME}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#static lib for the embedded resources
add_library(${PROJECT_NAME} STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}EmbeddedVFS.cpp)

add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_source)

#add post build command to copy the header from ./src to ../include/EmbeddedResources/
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../include/EmbeddedResources/
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/../include/EmbeddedResources/
)

#zstd
find_package(zstd CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE zstd::libzstd)