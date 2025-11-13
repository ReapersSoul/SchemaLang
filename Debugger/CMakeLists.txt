#set the minimum required version of cmake
cmake_minimum_required(VERSION 3.10)

project(SchemaLangDebugger)
#set the C++ standard to use
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SCHEMALANG_DEBUGGER_BUILD_TESTS "Build SchemaLang Debugger tests" ON)


file(GLOB_RECURSE SCHEMALANG_DEBUGGER_ALL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
set(SCHEMALANG_DEBUGGER_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
set(SCHEMALANG_DEBUGGER_CORE_SOURCES ${SCHEMALANG_DEBUGGER_ALL_SOURCES})
list(REMOVE_ITEM SCHEMALANG_DEBUGGER_CORE_SOURCES ${SCHEMALANG_DEBUGGER_MAIN})

add_executable(${PROJECT_NAME}
    ${SCHEMALANG_DEBUGGER_MAIN}
    ${SCHEMALANG_DEBUGGER_CORE_SOURCES}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE SCHEMALANG_DEBUG)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    -Wl,--whole-archive 
    SchemaLangShared 
    -Wl,--no-whole-archive
)


#create a ${BIN_PATH} in source and move the executable there after build
set(BIN_PATH bin)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/${BIN_PATH})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})

if(SCHEMALANG_DEBUGGER_BUILD_TESTS)
    enable_testing()
    set(SCHEMALANG_DEBUG_ENABLED ON CACHE BOOL "Enable SchemaLang debug hooks" FORCE)
    find_package(GTest CONFIG REQUIRED)

    add_executable(SchemaLangDebuggerTests
        tests/SchemaLangDebuggerTests.cpp
        ${SCHEMALANG_DEBUGGER_CORE_SOURCES}
    )

    target_compile_definitions(SchemaLangDebuggerTests PRIVATE 
        SCHEMALANG_DEBUG
        ANYRPG_SCHEMAS_DIR="${CMAKE_SOURCE_DIR}/AnyRPGShared/AnyRPGShared_Resources/Schemas"
    )
    target_include_directories(SchemaLangDebuggerTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/Shared/include
    )
    target_link_libraries(SchemaLangDebuggerTests PRIVATE
        -Wl,--whole-archive 
        SchemaLangShared 
        -Wl,--no-whole-archive
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(SchemaLangDebuggerTests)
endif()
