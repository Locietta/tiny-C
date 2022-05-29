project(AST)

find_package(fmt REQUIRED)

ANTLR_TARGET(CLexer CLexer.g4 LEXER
    PACKAGE antlrcpp)

ANTLR_TARGET(CParser CParser.g4 PARSER
    PACKAGE antlrcpp
    DEPENDS_ANTLR CLexer
    COMPILE_FLAGS -lib ${ANTLR_CLexer_OUTPUT_DIR}
    VISITOR
)

add_library(AST ASTPrinter.cpp
    ${ANTLR_CLexer_CXX_OUTPUTS}
    ${ANTLR_CParser_CXX_OUTPUTS})
target_include_directories(AST
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${ANTLR_CLexer_OUTPUT_DIR} ${ANTLR_CParser_OUTPUT_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(AST fmt antlr4_static)

# add_executable(testAST test_AST.cpp)
# target_link_libraries(testAST AST)
# target_include_directories(AST PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})