cmake_minimum_required(VERSION 3.20)
project(tinyC VERSION 0.0.1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 17)
add_definitions(-DANTLR4CPP_STATIC)

set(ANTLR_EXECUTABLE /usr/share/java/antlr-complete.jar)
find_package(ANTLR REQUIRED)
find_package(antlr4-runtime REQUIRED)

ANTLR_TARGET(CLexer CLexer.g4 LEXER
    PACKAGE antlrcpptest)

ANTLR_TARGET(CParser CParser.g4 PARSER
    PACKAGE antlrcpptest
    DEPENDS_ANTLR CLexer
    COMPILE_FLAGS -lib ${ANTLR_CLexer_OUTPUT_DIR})

include_directories(${ANTLR_CLexer_OUTPUT_DIR})
include_directories(${ANTLR_CParser_OUTPUT_DIR})
include_directories(${ANTLR4_INCLUDE_DIR})
# message(FATAL_ERROR hello:${ANTLR4_INCLUDE_DIR})

add_executable(tinycc main.cpp
    ${ANTLR_CLexer_CXX_OUTPUTS}
    ${ANTLR_CParser_CXX_OUTPUTS}
)
target_link_libraries(tinycc PRIVATE antlr4_static)
