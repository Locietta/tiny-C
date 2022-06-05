project(IR)

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

execute_process(COMMAND llvm-config --system-libs --libs all
    OUTPUT_VARIABLE llvm_libs)
string(STRIP ${llvm_libs} llvm_libs)

aux_source_directory(pass PASS_SRCS)
add_library(IR IRGenerator.cpp IRGenerator.h ${PASS_SRCS})
target_include_directories(
    IR
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/pass
    INTERFACE ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(
    IR
    PRIVATE AST
    PUBLIC ${llvm_libs}
)
target_precompile_headers(IR PUBLIC ${CMAKE_CURRENT_LIST_DIR}/pch.h)