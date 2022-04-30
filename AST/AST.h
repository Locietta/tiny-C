#pragma once

#include "variant_magic.hpp"
#include <memory>
#include <string>
#include <vector>

// ------------------------------ Operator Type ---------------------------
enum Operators {
    Plus,
    PlusPlus,
    Minus,
    MinusMinus,
    Mul,
    Div,
    Mod,
    Equal,        // ==
    Greater,      // >
    Less,         // <
    GreaterEqual, // >=
    LessEqual,    // <=
    NotEqual      // !=

    // etc
};

enum DataTypes {
    Void,
    Char,
    Int,
    Float,
    Double,
    String,
    // etc
};

// ------------------------------ AST Nodes --------------------------------

/// Forward Declaration
struct Expr; // generic node

struct Variable {
    enum DataTypes m_var_type;
    std::string m_var_name;
};

struct ConstVar {
    std::variant<char, int, float, double, std::string> m_value;
};

struct InitExpr {
    std::unique_ptr<Variable> m_var;
    std::unique_ptr<ConstVar> m_value;
};

struct Unary {
    std::unique_ptr<Expr> m_operand;
    enum Operators m_operator;
};

struct Binary {
    std::unique_ptr<Expr> m_operand1;
    std::unique_ptr<Expr> m_operand2;
    enum Operators m_operator;
};

struct IfElse {
    std::unique_ptr<Expr> m_condi;
    std::vector<std::unique_ptr<Expr>> m_if;
    std::vector<std::unique_ptr<Expr>> m_else;
};

//> Every loop can be converted to while-loop
struct WhileLoop {
    std::unique_ptr<Expr> m_condi;
    std::vector<std::unique_ptr<Expr>> m_loop_body;
};

/**
 * @code:
 * for (init;condition;iter) {
 *     loop_body
 * }
 *
 */
// struct ForLoop {
//     std::unique_ptr<Expr> m_init; // init
//     std::unique_ptr<Expr> m_condi; // condition
//     std::vector<std::unique_ptr<Expr>> m_loop_body; // loop body + iter
// };

// struct DoWhileLoop {
//     std::unique_ptr<Expr> m_condi;
//     std::vector<std::unique_ptr<Expr>> m_loop_body;
// };

struct Return {
    std::unique_ptr<Expr> m_expr;
};

struct FuncCall {
    std::vector<std::unique_ptr<Expr>> m_para_list;
    std::string m_func_name;
};

struct FuncDef {
    std::string m_name;
    std::vector<std::unique_ptr<Expr>> m_para_list; // should put `Variable` here
    std::vector<std::unique_ptr<Expr>> m_func_body;
    enum DataTypes m_return_type;
};

namespace impl { // Magic Base
using Base = std::variant<Variable, ConstVar, InitExpr, Unary, Binary, IfElse, WhileLoop, Return,
                          FuncCall, FuncDef>;
}

// dummy warpper for variant
struct Expr : public impl::Base {
    using impl::Base::Base;      // inhert ctors
    using impl::Base::operator=; // inhert assignments
};