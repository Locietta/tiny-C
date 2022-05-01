#pragma once
#include "AST/AST.h"
#include "CParserBaseVisitor.h"
using namespace antlrcpp;

class my_visitor : public CParserBaseVisitor {
public:
    std::vector<std::unique_ptr<FuncDef>> m_func_roots;
    std::vector<std::unique_ptr<Expr>> m_global_vars;
    Expr *m_curr_context = nullptr;

    std::any visitVar_decl(CParser::Var_declContext *ctx) override {
        std::any ret;
        // intial node
        std::unique_ptr<Expr> p_curr_node = std::make_unique<Expr>(InitExpr{});
        // move
        if (p_curr_node->is<FuncDef>()) {

        } else {
            m_global_vars.push_back(std::move(p_curr_node));
        }
        return ret;
    }
};
