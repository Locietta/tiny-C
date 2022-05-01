#pragma once
#include "AST.h"
#include "CParserBaseVisitor.h"
using namespace antlrcpp;

class my_visitor : public CParserBaseVisitor {
public:
    std::vector<std::unique_ptr<FuncDef>> m_func_roots;
    std::vector<std::unique_ptr<BaseNode>> m_global_vars;

    const BaseNode *tmp_ptr;
};
