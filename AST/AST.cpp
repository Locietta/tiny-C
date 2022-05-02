#include "AST.h"
#include <algorithm>
#include <fmt/core.h>
#include <fmt/os.h>

// Directly print graphviz without python script

// void ASTPrinter::edge_formatter(const Expr& e) {

// }

// void ASTPrinter::gv_formatter() {
//     fmt::format_to(std::back_inserter(buffer), "digraph {{ ");
//     edge_formatter(AST);
//     fmt::format_to(std::back_inserter(buffer), "}}");
// }

#define print2buf(fmt_str, ...) fmt::format_to(back_inserter(buffer), fmt_str, ##__VA_ARGS__)

using namespace std;

void ASTPrinter::sexp_fmt(const Expr &e) {
    match(
        e,
        [this](ConstVar const &var) {
            if (var.is<char>()) {
                print2buf(" {}", var.as<char>());
            } else if (var.is<int>()) {
                print2buf(" {}", var.as<int>());
            } else if (var.is<float>()) {
                print2buf(" {}", var.as<float>());
            } else if (var.is<string>()) {
                print2buf(" \"{}\"", var.as<string>());
            } else {
                unreachable();
            }
        },
        [this](Variable const &var) {
            print2buf(" (var:{}", var.m_var_name);
            print2buf(" type:{}", type_map[var.m_var_type]);
            sexp_fmt(*var.m_var_init);
            print2buf(")");
        },
        [this](InitExpr const &inits) {
            print2buf(" (init_expr");
            for (const auto &pVar : inits) {
                sexp_fmt(*pVar);
            }
            print2buf(")");
        },
        [this](Unary const &ua) {
            print2buf(" (unary:{}", op_map[ua.m_operator]);
            sexp_fmt(*ua.m_operand);
            print2buf(")");
        },
        [this](Binary const &bin) {
            print2buf(" (binary:{}", op_map[bin.m_operator]);
            sexp_fmt(*bin.m_operand1);
            sexp_fmt(*bin.m_operand1);
            print2buf(")");
        },
        [this](IfElse const &branch) {
            print2buf(" (if-block");
            sexp_fmt(*branch.m_condi);
            sexp_fmt(*branch.m_if);
            if (branch.m_else) {
                sexp_fmt(*branch.m_else);
            }
            print2buf(")");
        },
        [this](WhileLoop const &loop) {
            print2buf(" (while");
            sexp_fmt(*loop.m_condi);
            sexp_fmt(*loop.m_loop_body);
            print2buf(")");
        },
        [this](Return const &ret) {
            print2buf(" (return");
            sexp_fmt(*ret.m_expr);
            print2buf(")");
        },
        [this](FuncCall const &call) {
            print2buf(" (call:{}", call.m_func_name);
            for (const auto &p_para : call.m_para_list) {
                sexp_fmt(*p_para);
            }
            print2buf(")");
        },
        [this](FuncDef const &func) {
            print2buf(" (func:{}", func.m_name);
            print2buf(" ret_type:{}", type_map[func.m_return_type]);
            for (const auto &p_para : func.m_para_list) {
                sexp_fmt(*p_para);
            }
            sexp_fmt(*func.m_func_body);
            print2buf(")");
        },
        [this](NameRef const &name) { print2buf(" name_ref:{}", name); },
        [this](CompoundExpr const &comp) {
            print2buf(" (compound");
            for (const auto &expr : comp) {
                sexp_fmt(*expr);
            }
            print2buf(")");
        });
}

void ASTPrinter::ToPNG(const char *filename) {}

void ASTPrinter::ToPNG(const string &filename) {}