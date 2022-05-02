find_package(fmt)

add_library(AST AST.cpp)
target_include_directories(AST
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(AST fmt)

add_executable(testAST test_AST.cpp)
target_link_libraries(testAST AST)
target_include_directories(AST PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
