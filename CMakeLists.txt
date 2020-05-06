cmake_minimum_required(VERSION 3.0)

project(eryn)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(NODE_ARCH STREQUAL "ia32")
    message(STATUS "Compiling for ia32")

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    endif()
endif()

include_directories(${CMAKE_JS_INC})

file(GLOB SOURCE_FILES
    src/*.hxx
    src/*.cxx
    src/*.dxx

    src/**/*.hxx
    src/**/*.cxx
    src/**/*.dxx

    lib/*.hxx
    lib/*.cxx
    lib/*.h
    lib/*.c

    lib/**/*.hxx
    lib/**/*.cxx
    lib/**/*.h
    lib/**/*.c)

add_definitions(-DMODULE_NAME=${PROJECT_NAME})
add_definitions(-DNAPI_VERSION=3)
#add_definitions(-DDEBUG)
#add_definitions(-DLOG_MEMORY_OPERATIONS)
add_definitions(-DLOG_MEMORY_FULLY_FREED)
#add_definitions(-DDUMP_OSH_FILES_ON_COMPILE)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

execute_process(COMMAND node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
)

string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})