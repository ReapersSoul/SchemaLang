project(SchemaLangTranspiler)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

list(FILTER SOURCES EXCLUDE REGEX "build/")
list(FILTER SOURCES EXCLUDE REGEX "bin/")

add_subdirectory(Embedded)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE EmbeddedResources)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package(nlohmann_json REQUIRED)
find_package(Boost REQUIRED COMPONENTS dll)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json Boost::dll)

#physfs
find_package(PhysFS REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${PHYSFS_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PHYSFS_LIBRARY})

#zstd
find_package(zstd CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE zstd::libzstd)