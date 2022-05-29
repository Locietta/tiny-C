
add_executable(tinycc main.cpp
${ANTLR_CLexer_CXX_OUTPUTS}
${ANTLR_CParser_CXX_OUTPUTS}
)
target_link_libraries(tinycc PRIVATE antlr4_static AST IR)
add_custom_command( # install hooks
TARGET tinycc
PRE_BUILD
COMMAND git config core.hooksPath ${PROJECT_SOURCE_DIR}/.github/.hooks
COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/sexp_to_png.py ${PROJECT_BINARY_DIR}
VERBATIM
)