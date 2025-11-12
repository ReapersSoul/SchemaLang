#set the minimum required version of cmake
cmake_minimum_required(VERSION 3.10)

project(SchemaLangDebugger)
#set the C++ standard to use
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB_RECURSE SCHEMALANGDEBUGGER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SCHEMALANGDEBUGGER_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    -Wl,--whole-archive 
    SchemaLangShared 
    -Wl,--no-whole-archive
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)


#create a ${BIN_PATH} in source and move the executable there after build
set(BIN_PATH bin)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/${BIN_PATH})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})
