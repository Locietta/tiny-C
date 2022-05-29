cmake_minimum_required(VERSION 3.20)
project(tinyC VERSION 0.0.1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 20)
add_definitions(-DANTLR4CPP_STATIC)

set(ANTLR_EXECUTABLE /usr/share/java/antlr-complete.jar)
find_package(ANTLR REQUIRED)
find_package(antlr4-runtime REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)
find_package(fmt REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# ANTLR4 includes
include_directories(${ANTLR4_INCLUDE_DIR})
message(STATUS "Antlr4 Runtime Dir: ${ANTLR4_INCLUDE_DIR}")

# LLVM includes
include_directories(${LLVM_INCLUDE_DIRS})

# Top Level Public Headers
include_directories(${CMAKE_CURRENT_LIST_DIR}/utility)

# sub-directories
add_subdirectory(AST)
add_subdirectory(IR)

# top-level driver code
add_executable(tinycc main.cpp)
target_include_directories(tinycc
    PRIVATE
    ${ANTLR_CLexer_OUTPUT_DIR}
    ${ANTLR_CParser_OUTPUT_DIR}
    ${ANTLR4_INCLUDE_DIR}
)
target_link_libraries(tinycc PRIVATE AST IR)
add_custom_command( # install hooks
    TARGET tinycc
    PRE_BUILD
    COMMAND git config core.hooksPath ${PROJECT_SOURCE_DIR}/.github/.hooks
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/sexp_to_png.py ${PROJECT_BINARY_DIR}
    VERBATIM
)