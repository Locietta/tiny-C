#pragma once
#include "AST.h"
#include "CParserBaseVisitor.h"
#include <string>
using namespace antlrcpp;
using namespace std;

inline auto expr_cast(std::any any) {
    return any_cast<shared_ptr<Expr>>(any);
}

class ASTBuilder : public CParserBaseVisitor {
    using TerminalNode = antlr4::tree::TerminalNode;
    using ParseTreeType = antlr4::tree::ParseTreeType;

public:
    // Top level declarations, vector of `FuncDef` or `InitExpr`
    std::vector<std::shared_ptr<Expr>> m_decls;
    bool is_global = true;

    std::any visitTerminal(TerminalNode *pTerminal) override {
        assert(TerminalNode::is(pTerminal) && "It's not terminal, why?"); // sanity check
        string const_text = pTerminal->getText();

        if (const_text.front() == '\'') {
            if (const_text[1] != '\\') { // non-escape
                return ConstVar{const_text[1]};
            } else { // escapes
                char hint = const_text[2];
                if (hint == 'n') {
                    return ConstVar{'\n'};
                } else if (hint == 't') {
                    return ConstVar{'\t'};
                } else if (hint == '\\') {
                    return ConstVar{'\\'};
                } else {
                    assert(false && "Unsupported escape sequence!");
                    unreachable();
                }
            }
        } else if (const_text.find('.') == std::string::npos) {
            return ConstVar{atoi(const_text.c_str())};
        } else {
            return ConstVar{atof(const_text.c_str())};
        }

        return defaultResult();
    }

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
            m_decls.push_back(move(ret));
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

    std::any visitFunc_decl(CParser::Func_declContext *ctx) override {
        is_global = false;

        auto ret = make_shared<Expr>(FuncDef{
            .m_name = ctx->Identifier()->toString(),
            .m_para_list = any_cast<std::vector<std::shared_ptr<Expr>>>(visit(ctx->params())),
            .m_body = expr_cast(visit(ctx->comp_stmt())),
            .m_return_type = any_cast<enum DataTypes>(visit(ctx->type_spec())),
        });

        m_decls.push_back(ret);
        is_global = true;
        return ret;
    }

    std::any visitParams(CParser::ParamsContext *ctx) override {
        std::vector<std::shared_ptr<Expr>> ret;

        if (ctx->param_list()) {
            ret = any_cast<std::vector<std::shared_ptr<Expr>>>(visit(ctx->param_list()));
        } else if (ctx->Void()) {
            ret.emplace_back(
                make_shared<Expr>(Variable{.m_var_type = Void, .m_var_name = "<void>"}));
        }
        return ret;
    }

    std::any visitParam_list(CParser::Param_listContext *ctx) override {
        std::vector<std::shared_ptr<Expr>> ret(ctx->param().size());
        for (const auto &params = ctx->param(); const auto &param : params) {
            ret.push_back(any_cast<shared_ptr<Expr>>(visit(param)));
        }

        return ret;
    }

    std::any visitParam(CParser::ParamContext *ctx) override {
        return make_shared<Expr>(Variable{
            .m_var_type = any_cast<enum DataTypes>(visit(ctx->type_spec())),
            .m_var_name = ctx->Identifier()->toString(),
        });
    }

    std::any visitNo_array_decl(CParser::No_array_declContext *ctx) override {
        return make_shared<Expr>(Variable{
            .m_var_name = ctx->Identifier()->toString(),
            .m_var_init = ctx->Assign()
                              ? make_shared<Expr>(any_cast<ConstVar>(visit(ctx->Constant())))
                              : nullptr,
        });
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
        return make_shared<Expr>(IfElse{
            .m_condi = expr_cast(visit(ctx->expr())),                         // condition
            .m_if = expr_cast(visit(ctx->stmt(0))),                           // if path
            .m_else = ctx->Else() ? expr_cast(visit(ctx->stmt(1))) : nullptr, // else path
        });
    }

    std::any visitWhile_loop(CParser::While_loopContext *ctx) override {
        return make_shared<Expr>(WhileLoop{
            .m_condi = expr_cast(visit(ctx->expr())),
            .m_loop_body = expr_cast(visit(ctx->stmt())),
        });
    }

    std::any visitFor_loop(CParser::For_loopContext *ctx) override {
        auto ret = make_shared<Expr>(ForLoop{});
        auto &curr_node = ret->as<ForLoop>();

        if (ctx->for_init()) curr_node.m_init = expr_cast(visit(ctx->for_init()));

        if (ctx->for_condi()) curr_node.m_condi = expr_cast(visit(ctx->for_condi()));

        if (ctx->for_iter()) curr_node.m_iter = expr_cast(visit(ctx->for_iter()));

        curr_node.m_loop_body = expr_cast(visit(ctx->stmt()));

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
        return make_shared<Expr>(Binary{
            .m_operand1 = expr_cast(visit(ctx->var())),
            .m_operand2 = expr_cast(visit(ctx->expr())),
            .m_operator = any_cast<enum Operators>(visit(ctx->assign())),
        });
    }

    std::any visitUnary_expr(CParser::Unary_exprContext *ctx) override {
        if (ctx->oror_expr()) {
            return visit(ctx->oror_expr());
        } else {
            auto ret = make_shared<Expr>(Unary{});
            auto &curr_node = ret->as<Unary>();

            curr_node.m_operand = expr_cast(visit(ctx->unary_expr()));
            curr_node.m_operator = any_cast<enum Operators>(visit(ctx->unary_operator()));

            return ret;
        }
    }

    std::any visitUnary_operator(CParser::Unary_operatorContext *ctx) override {
        enum Operators ret;
        if (ctx->Not()) {
            ret = Not;
        } else if (ctx->Plus()) {
            ret = Plus;
        } else if (ctx->Minus()) {
            ret = Minus;
        } else {
            // error
            assert(false);
            unreachable();
        }

        return ret;
    }

    std::any visitVar(CParser::VarContext *ctx) override {
        auto ret = make_shared<Expr>(NameRef{});
        auto &curr_node = ret->as<NameRef>();

        curr_node = ctx->Identifier()->getText();

        return ret;
    }

    std::any visitAssign(CParser::AssignContext *ctx) override {
        enum Operators ret;

        if (ctx->Assign()) {
            ret = Assign;
        } else if (ctx->PlusAssign()) {
            ret = PlusAssign;
        } else if (ctx->MinusAssign()) {
            ret = MinusAssign;
        } else if (ctx->MulAssign()) {
            ret = MulAssign;
        } else if (ctx->DivAssign()) {
            ret = DivAssign;
        } else if (ctx->ModAssign()) {
            ret = ModAssign;
        }

        return ret;
    }

    std::any visitOror_expr(CParser::Oror_exprContext *ctx) override {
        if (ctx->oror_expr()) {
            // has operator
            auto ret = make_shared<Expr>(Binary{});
            auto &curr_node = ret->as<Binary>();

            // operator
            curr_node.m_operator = OrOr;
            // operand 2
            curr_node.m_operand2 = expr_cast(visit(ctx->andand_expr()));
            // operand 1
            curr_node.m_operand1 = expr_cast(visit(ctx->oror_expr()));

            return ret;
        } else {
            return visit(ctx->andand_expr());
        }
    }

    std::any visitAndand_expr(CParser::Andand_exprContext *ctx) override {
        if (ctx->andand_expr()) {
            // has operator
            auto ret = make_shared<Expr>(Binary{});
            auto &curr_node = ret->as<Binary>();

            // operator
            curr_node.m_operator = AndAnd;
            // operand 2
            curr_node.m_operand2 = expr_cast(visit(ctx->equal_expr()));
            // operand 1
            curr_node.m_operand1 = expr_cast(visit(ctx->andand_expr()));

            return ret;
        } else {
            return visit(ctx->equal_expr());
        }
    }

    std::any visitEqual_expr(CParser::Equal_exprContext *ctx) override {
        if (ctx->equal_expr()) {
            // has operator
            auto ret = make_shared<Expr>(Binary{});
            auto &curr_node = ret->as<Binary>();

            // operator
            if (ctx->Equal()) {
                curr_node.m_operator = Equal;
            } else if (ctx->NotEqual()) {
                curr_node.m_operator = NotEqual;
            } else {
                // error
                assert(false);
                unreachable();
            }

            // operand 2
            curr_node.m_operand2 = expr_cast(visit(ctx->compare_expr()));
            // operand 1
            curr_node.m_operand1 = expr_cast(visit(ctx->equal_expr()));

            return ret;
        } else {
            return visit(ctx->compare_expr());
        }
    }

    std::any visitCompare_expr(CParser::Compare_exprContext *ctx) override {
        if (ctx->compare_expr()) {
            // has operator
            auto ret = make_shared<Expr>(Binary{});
            auto &curr_node = ret->as<Binary>();

            // operator
            curr_node.m_operator = any_cast<enum Operators>(visit(ctx->relop()));
            // operand 2
            curr_node.m_operand2 = expr_cast(visit(ctx->add_expr()));
            // operand 1
            curr_node.m_operand1 = expr_cast(visit(ctx->compare_expr()));

            return ret;
        } else {
            return visit(ctx->add_expr());
        }
    }

    std::any visitRelop(CParser::RelopContext *ctx) override {
        enum Operators ret;
        if (ctx->Less()) {
            ret = Less;
        } else if (ctx->LessEqual()) {
            ret = LessEqual;
        } else if (ctx->Greater()) {
            ret = Greater;
        } else if (ctx->GreaterEqual()) {
            ret = GreaterEqual;
        } else {
            // error
            assert(false);
            unreachable();
        }

        return ret;
    }

    std::any visitAdd_expr(CParser::Add_exprContext *ctx) override {
        if (ctx->add_expr()) {
            // has operator
            auto ret = make_shared<Expr>(Binary{});
            auto &curr_node = ret->as<Binary>();

            // operator
            if (ctx->Plus()) {
                curr_node.m_operator = Plus;
            } else if (ctx->Minus()) {
                curr_node.m_operator = Minus;
            } else {
                // error
                assert(false);
                unreachable();
            }
            // operand 2
            curr_node.m_operand2 = expr_cast(visit(ctx->term()));
            // operand 1
            curr_node.m_operand1 = expr_cast(visit(ctx->add_expr()));

            return ret;
        } else {
            return visit(ctx->term());
        }
    }

    std::any visitTerm(CParser::TermContext *ctx) override {
        if (ctx->term()) {
            // has operator
            auto ret = make_shared<Expr>(Binary{});
            auto &curr_node = ret->as<Binary>();

            // operator
            if (ctx->Mul()) {
                curr_node.m_operator = Mul;
            } else if (ctx->Div()) {
                curr_node.m_operator = Div;
            } else if (ctx->Mod()) {
                curr_node.m_operator = Mod;
            } else {
                // error
                assert(false);
                unreachable();
            }
            // operand 2
            curr_node.m_operand2 = expr_cast(visit(ctx->factor()));
            // operand 1
            curr_node.m_operand1 = expr_cast(visit(ctx->term()));

            return ret;
        } else {
            return visit(ctx->factor());
        }
    }

    std::any visitConst_factor(CParser::Const_factorContext *ctx) override {
        auto const_var = any_cast<ConstVar>(visit(ctx->Constant()));
        return make_shared<Expr>(const_var);
    }

    std::any visitCall(CParser::CallContext *ctx) override {
        auto ret = make_shared<Expr>(FuncCall{});
        auto &curr_node = ret->as<FuncCall>();

        curr_node.m_func_name = ctx->Identifier()->getText();

        int n;
        if ((n = ctx->args()->expr().size()) > 0) {
            for (int i = 0; i < n; i++) {
                curr_node.m_para_list.push_back(expr_cast(visit(ctx->args()->expr(i))));
            }
        }

        return ret;
    }

    std::any visitExpr_stmt(CParser::Expr_stmtContext *ctx) override { return visit(ctx->expr()); }

    std::any visitParen_factor(CParser::Paren_factorContext *ctx) override {
        return visit(ctx->expr());
    }

    std::any visitArgs(CParser::ArgsContext *ctx) override {
        cout << "unexpected call func:visitArgs, by cdh" << endl;
        return visitChildren(ctx);
    }

    std::any visitBreak_stmt(CParser::Break_stmtContext *ctx) override {
        auto ret = make_shared<Expr>(Break{});
        auto &curr_node = ret->as<Break>();
        return ret;
    }

    std::any visitContinue_stmt(CParser::Continue_stmtContext *ctx) override {
        auto ret = make_shared<Expr>(Continue{});
        auto &curr_node = ret->as<Continue>();
        return ret;
    }

    std::any visitStmt_var_decl(CParser::Stmt_var_declContext *ctx) override {
        return visit(ctx->var_decl());
    }

    std::any visitDecl_var(CParser::Decl_varContext *ctx) override {
        return visit(ctx->var_decl());
    }
};
