
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