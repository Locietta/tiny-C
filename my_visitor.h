#pragma once
#include "AST/AST.h"
#include "CParserBaseVisitor.h"
using namespace antlrcpp;

class my_visitor : public CParserBaseVisitor {
public:
    std::vector<std::unique_ptr<FuncDef>> m_func_roots;
    std::vector<std::unique_ptr<Expr>> m_global_vars;
    Expr *m_curr_parent = nullptr;

    std::any visitVar_decl(CParser::Var_declContext *ctx) override {
        std::any ret;

        for (int i = 0; i < ctx->simple_var_decl().size(); i++) {
            // intial node
            std::unique_ptr<Expr> new_node = std::make_unique<Expr>(InitExpr{});

            // init children
            Expr *tmp = m_curr_parent;
            m_curr_parent = new_node.get();
            // visit(ctx->type_spec());
            // visit(ctx->simple_var_decl(i));
            m_curr_parent = tmp;

            // move
            if (m_curr_parent->is<FuncDef>()) {
                reinterpret_cast<FuncDef *>(m_curr_parent)
                    ->m_func_body.push_back(std::move(new_node));
            } else {
                m_global_vars.push_back(std::move(new_node));
            }
        }

        return ret;
    }

    std::any visitType_spec(CParser::Type_specContext *ctx) override { return visitChildren(ctx); }
};
