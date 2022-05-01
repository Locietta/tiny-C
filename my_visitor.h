#pragma once
#include "AST.h"
#include "CParserBaseVisitor.h"
#include <string>
using namespace antlrcpp;

class my_visitor : public CParserBaseVisitor {
public:
    std::vector<std::unique_ptr<FuncDef>> m_func_roots;
    std::vector<std::unique_ptr<Expr>> m_global_vars;
    Expr *m_curr_parent = nullptr;

    std::any visitVar_decl(CParser::Var_declContext *ctx) override {
        for (int i = 0; i < ctx->simple_var_decl().size(); i++) {
            // intial node
            std::unique_ptr<Expr> new_node = std::make_unique<Expr>(InitExpr{});

            // init children
            Expr *tmp = m_curr_parent;

            InitExpr *inner_ptr = reinterpret_cast<InitExpr *>(new_node.get());
            inner_ptr->m_var = std::make_unique<Variable>();
            inner_ptr->m_value = std::make_unique<ConstVar>();
            m_curr_parent = reinterpret_cast<Expr *>(inner_ptr);
            visit(ctx->type_spec());
            visit(ctx->simple_var_decl(i));
            m_curr_parent = tmp;

            // move
            if (m_curr_parent == nullptr) {
                m_global_vars.push_back(std::move(new_node));
            } else if (m_curr_parent->is<CompoundExpr>()) {
                reinterpret_cast<CompoundExpr *>(m_curr_parent)
                    ->m_expr_list.push_back(std::move(new_node));
            } else {
                // error
            }
        }

        std::any ret;
        return ret;
    }

    std::any visitType_spec(CParser::Type_specContext *ctx) override {
        // at that time m_curr_parent must be InitExpr
        if (m_curr_parent && m_curr_parent->is<InitExpr>()) {
            InitExpr *init_ptr = reinterpret_cast<InitExpr *>(m_curr_parent);
            if (ctx->Char()) {
                init_ptr->m_var->m_var_type = Char;
            } else if (ctx->Int()) {
                init_ptr->m_var->m_var_type = Int;
            } else if (ctx->Long()) {
                init_ptr->m_var->m_var_type = Long;
            } else if (ctx->Float()) {
                init_ptr->m_var->m_var_type = Float;
            } else if (ctx->Short()) {
                init_ptr->m_var->m_var_type = Short;
            } else if (ctx->Void()) {
                init_ptr->m_var->m_var_type = Void;
            } else {
                // error
            }
        } else {
            // error
        }
        std::any ret;
        return ret;
    }

    std::any visitNo_array_decl(CParser::No_array_declContext *ctx) override {
        // at that time m_curr_parent must be InitExpr
        if (m_curr_parent && m_curr_parent->is<InitExpr>()) {
            InitExpr *init_ptr = reinterpret_cast<InitExpr *>(m_curr_parent);
            init_ptr->m_var->m_var_name = ctx->Identifier()->toString();
            if (ctx->Assign()) {
                std::string const_text = ctx->Constant()->getText();
                if (const_text[0] == '\'') {
                    if (const_text == R"('\n')")
                        init_ptr->m_value->m_value = '\n';
                    else if (const_text == R"('\t')")
                        init_ptr->m_value->m_value = '\t';
                    else if (const_text == R"('\\')")
                        init_ptr->m_value->m_value = '\\';
                    else {
                        // error
                    }
                } else if (const_text.find('.') == std::string::npos) {
                    init_ptr->m_value->m_value = atoi(const_text.c_str());
                } else {
                    init_ptr->m_value->m_value = atof(const_text.c_str());
                }
            }
        } else {
            // error
        }
        std::any ret;
        return ret;
    }
};
