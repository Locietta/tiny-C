#pragma once
#include "AST.h"
#include "CParserBaseVisitor.h"
#include <string>
using namespace antlrcpp;
using namespace std;

class my_visitor : public CParserBaseVisitor {
public:
    std::vector<std::shared_ptr<FuncDef>> m_func_roots;
    std::vector<std::shared_ptr<InitExpr>> m_global_vars;
    // Expr *m_curr_parent = nullptr;
    bool is_global = true;

    std::any visitVar_decl(CParser::Var_declContext *ctx) override {
        // initial node
        shared_ptr<InitExpr> curr_node = make_shared<InitExpr>();

        for (int i = 0; i < ctx->simple_var_decl().size(); i++) {
            // init children
            curr_node->m_vars.push_back(
                any_cast<shared_ptr<Variable>>(visit(ctx->simple_var_decl(i))));
            curr_node->m_vars.back()->m_var_type =
                any_cast<enum DataTypes>(visit(ctx->type_spec()));
        }

        if (is_global) {
            m_global_vars.push_back(curr_node);
            return {};
        } else {
            return curr_node;
        }
    }

    std::any visitType_spec(CParser::Type_specContext *ctx) override {
        if (ctx->Char()) {
            return Char;
        } else if (ctx->Int()) {
            return Int;
        } else if (ctx->Long()) {
            return Long;
        } else if (ctx->Float()) {
            return Float;
        } else if (ctx->Short()) {
            return Short;
        } else if (ctx->Void()) {
            return Void;
        } else {
            // error
            return {};
        }
    }

    std::any visitNo_array_decl(CParser::No_array_declContext *ctx) override {
        shared_ptr<Variable> curr_node = make_shared<Variable>();
        curr_node->m_var_name = ctx->Identifier()->toString();
        if (ctx->Assign()) {
            curr_node->m_var_init = make_shared<ConstVar>();
            std::string const_text = ctx->Constant()->getText();
            if (const_text[0] == '\'') {
                if (const_text == R"('\n')")
                    *(curr_node->m_var_init) = '\n';
                else if (const_text == R"('\t')")
                    *(curr_node->m_var_init) = '\t';
                else if (const_text == R"('\\')")
                    *(curr_node->m_var_init) = '\\';
                else {
                    // error
                }
            } else if (const_text.find('.') == std::string::npos) {
                *(curr_node->m_var_init) = atoi(const_text.c_str());
            } else {
                *(curr_node->m_var_init) = atof(const_text.c_str());
            }
        }

        return curr_node;
    }

    std::any visitComp_stmt(CParser::Comp_stmtContext *ctx) override {
        // init node
        shared_ptr<Expr> curr_node = make_shared<Expr>(CompoundExpr{});

        int n = ctx->stmt().size();
        for (int i = 0; i < n; i++) {
            curr_node->as<CompoundExpr>().m_expr_list.push_back(
                any_cast<shared_ptr<Expr>>(visit(ctx->stmt(i))));
        }

        return curr_node;
    }
};
