project(IR)

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(llvm_libs support core codegen engine irreader)

add_library(IR IRBuilder.cpp)
target_include_directories(
    IR
    INTERFACE ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(
    IR
    PRIVATE AST
    PUBLIC ${llvm_libs}
)
target_precompile_headers(IR PRIVATE [[pch.h]])