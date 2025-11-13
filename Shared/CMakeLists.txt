#set the minimum required version of cmake
cmake_minimum_required(VERSION 3.10)
project(SchemaLangShared)
#set the C++ standard to use
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SHARED_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(${PROJECT_NAME} STATIC ${SHARED_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(SCHEMALANG_DEBUG_ENABLED)
	target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Debugger/include)
endif()

# Allow SCHEMALANG_DEBUG to be defined by parent targets
target_compile_definitions(${PROJECT_NAME} PUBLIC $<$<BOOL:${SCHEMALANG_DEBUG_ENABLED}>:SCHEMALANG_DEBUG>)

add_subdirectory(${PROJECT_NAME}_Resources)
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}_Resources)

find_package(nlohmann_json REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS dll)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json Boost::dll)