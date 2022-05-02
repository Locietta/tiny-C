#pragma once
#include "AST.h"
#include "CParserBaseVisitor.h"
#include <string>
using namespace antlrcpp;
using namespace std;

inline auto expr_cast(std::any any) {
    return any_cast<shared_ptr<Expr>>(any);
}

class my_visitor : public CParserBaseVisitor {
public:
    std::vector<std::shared_ptr<Expr>> m_func_roots;  // FuncDef
    std::vector<std::shared_ptr<Expr>> m_global_vars; // InitExpr
    bool is_global = true;

    std::any visitVar_decl(CParser::Var_declContext *ctx) override {
        // initial node
        auto ret = make_shared<Expr>(InitExpr{});
        auto &curr_node = ret->as<InitExpr>(); // InitExpr curr_node;

        auto type = any_cast<enum DataTypes>(visit(ctx->type_spec()));
        const auto &simple_var_decls = ctx->simple_var_decl();

        for (auto simple_var : simple_var_decls) {
            auto var = expr_cast(visit(simple_var))->as<Variable>();
            var.m_var_type = type;
            curr_node.push_back(make_shared<Expr>(move(var)));
        }

        if (is_global) {
            m_global_vars.push_back(move(ret));
            return {};
        } else {
            return move(ret);
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
            assert(false);
            unreachable();
        }
    }

    std::any visitNo_array_decl(CParser::No_array_declContext *ctx) override {
        auto ret = make_shared<Expr>(Variable{
            .m_var_name = ctx->Identifier()->toString(),
        });
        auto &curr_node = ret->as<Variable>(); // Variable curr_node;

        if (ctx->Assign()) {
            auto p_init = make_shared<Expr>(ConstVar{});
            auto &init = p_init->as<ConstVar>();

            std::string const_text = ctx->Constant()->getText();
            if (const_text[0] == '\'') {
                if (const_text == R"('\n')")
                    init = '\n';
                else if (const_text == R"('\t')")
                    init = '\t';
                else if (const_text == R"('\\')")
                    init = '\\';
                else {
                    // error
                    assert(false);
                    unreachable();
                }
            } else if (const_text.find('.') == std::string::npos) {
                init = atoi(const_text.c_str());
            } else {
                init = atof(const_text.c_str());
            }
            curr_node.m_var_init = make_shared<Expr>(init);
        }

        return ret;
    }

    std::any visitComp_stmt(CParser::Comp_stmtContext *ctx) override {
        // init node
        auto ret = make_shared<Expr>(CompoundExpr{});
        auto &curr_node = ret->as<CompoundExpr>();

        for (const auto &stmts = ctx->stmt(); const auto &stmt : stmts) {
            curr_node.push_back(expr_cast(visit(stmt)));
        }

        return ret;
    }

    std::any visitSelec_stmt(CParser::Selec_stmtContext *ctx) override {
        auto ret = make_shared<Expr>(IfElse{});
        auto &curr_node = ret->as<IfElse>();

        // condition
        curr_node.m_condi = expr_cast(visit(ctx->expr()));

        // if path
        any compound0 = visit(ctx->comp_stmt(0));
        any stmt0 = visit(ctx->stmt(0));
        if (compound0.has_value() && !stmt0.has_value())
            curr_node.m_if = expr_cast(compound0);
        else if (!compound0.has_value() && stmt0.has_value())
            curr_node.m_if = expr_cast(stmt0);
        else {
            // error
            assert(false);
            unreachable();
        }

        // else path
        if (ctx->Else()) {
            any compound1 = visit(ctx->comp_stmt(1));
            any stmt1 = visit(ctx->stmt(1));
            if (compound1.has_value() && !stmt1.has_value())
                curr_node.m_else = expr_cast(compound1);
            else if (!compound1.has_value() && stmt1.has_value())
                curr_node.m_else = expr_cast(stmt1);
            else {
                // error
                assert(false);
                unreachable();
            }
        }

        return ret;
    }

    std::any visitIter_stmt(CParser::Iter_stmtContext *ctx) override {
        auto ret = make_shared<Expr>(WhileLoop{});
        auto &curr_node = ret->as<WhileLoop>();

        // while condition
        curr_node.m_condi = expr_cast(visit(ctx->expr()));

        // loop body
        any comp = visit(ctx->comp_stmt());
        any stmt = visit(ctx->stmt());
        if (comp.has_value() && !stmt.has_value()) {
            curr_node.m_loop_body = expr_cast(comp);
        } else if (!comp.has_value() && stmt.has_value()) {
            curr_node.m_loop_body = expr_cast(stmt);
        } else {
            // error
            assert(false);
            unreachable();
        }

        return ret;
    }

    std::any visitReturn_stmt(CParser::Return_stmtContext *ctx) override {
        auto ret = make_shared<Expr>(Return{});
        auto &curr_node = ret->as<Return>();

        any return_value = visit(ctx->expr());
        if (return_value.has_value()) curr_node.m_expr = expr_cast(return_value);

        return ret;
    }

    std::any visitAssign_expr(CParser::Assign_exprContext *ctx) override {
        auto ret = make_shared<Expr>(Binary{});
        auto &curr_node = ret->as<Binary>();

        curr_node.m_operand1 = expr_cast(visit(ctx->var()));
        curr_node.m_operand2 = expr_cast(visit(ctx->expr()));
        curr_node.m_operator = any_cast<enum Operators>(visit(ctx->assign()));

        return ret;
    }

    std::any visitVar(CParser::VarContext *ctx) override {}
};
