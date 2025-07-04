project(SchemaLangTranspiler)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

list(FILTER SOURCES EXCLUDE REGEX "build/")
list(FILTER SOURCES EXCLUDE REGEX "bin/")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package(nlohmann_json REQUIRED)
find_package(Boost REQUIRED COMPONENTS dll)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json Boost::dll)