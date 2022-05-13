#include "ASTPrinter.h"
#include "AST.hpp"
#include "utility.hpp"
#include <algorithm>
#include <cstring>
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
                auto ch = var.as<char>();
                if (ch == '\n') {
                    print2buf(" <NewLine>");
                } else if (ch == '\t') {
                    print2buf(" <Tab>");
                } else {
                    print2buf(" {}", var.as<char>());
                }
            } else if (var.is<int>()) {
                print2buf(" {}", var.as<int>());
            } else if (var.is<float>()) {
                print2buf(" {}", var.as<float>());
            } else if (var.is<double>()) {
                print2buf(" {}", var.as<double>());
            } else if (var.is<string>()) {
                print2buf(" \"{}\"", var.as<string>());
            } else {
                assert(false && "Unknown type, how?");
                unreachable();
            }
        },
        [this](Variable const &var) {
            assert(!var.m_var_name.empty() && "Var with no name OR default empty Expr");
            print2buf(" (var:{}", var.m_var_name);
            print2buf(" type:{}", type_to_str[var.m_var_type]);
            if (var.m_var_init) sexp_fmt(*var.m_var_init);
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
            print2buf(" (unary:{}", op_to_str[ua.m_operator]);
            sexp_fmt(*ua.m_operand);
            print2buf(")");
        },
        [this](Binary const &bin) {
            print2buf(" (binary:{}", op_to_str[bin.m_operator]);
            sexp_fmt(*bin.m_operand1);
            sexp_fmt(*bin.m_operand2);
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
            print2buf(" ret_type:{}", type_to_str[func.m_return_type]);
            for (const auto &p_para : func.m_para_list) {
                sexp_fmt(*p_para);
            }
            sexp_fmt(*func.m_body);
            print2buf(")");
        },
        [this](NameRef const &name) { print2buf(" name_ref:{}", name); },
        [this](CompoundExpr const &comp) {
            print2buf(" (compound");
            for (const auto &expr : comp) {
                sexp_fmt(*expr);
            }
            print2buf(")");
        },
        [this](Break const &) { print2buf(" [BREAK]"); },
        [this](Continue const &) { print2buf(" [CONTINUE]"); },
        [this](ForLoop const &loop) {
            print2buf(" (for");
            if (loop.m_init) sexp_fmt(*loop.m_init);
            if (loop.m_condi) sexp_fmt(*loop.m_condi);
            if (loop.m_iter) sexp_fmt(*loop.m_iter);
            sexp_fmt(*loop.m_loop_body);
            print2buf(")");
        },
        [this](Null const &) { print2buf(" [NULL]"); });
}

void ASTPrinter::ToPNG(fs::path const &exe_path, fs::path const &filename) {
    sexp_fmt(*AST);
    std::string out = fmt::format("echo \"{}\n{}\" | python {}/sexp_to_png.py",
                                  fmt::to_string(buffer),
                                  filename.c_str(),
                                  exe_path.parent_path().c_str());
    dbg_print("{}\n", fmt::to_string(buffer));
    system(out.c_str());
}