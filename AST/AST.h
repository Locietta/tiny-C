#pragma once

#include "utility.hpp"
#include "variant_magic.hpp"
#include <cassert>
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
    Short,
    Long
    // etc
};

// ------------------------------ AST Nodes --------------------------------

/// Forward Declaration
struct Expr; // generic node

struct CompoundExpr : std::vector<std::shared_ptr<Expr>> {
    using Base = std::vector<std::shared_ptr<Expr>>;
    using Base::Base;
    using Base::operator=;
};

struct ConstVar : public std::variant<char, int, float, double, std::string> {
    using Base = std::variant<char, int, float, double, std::string>;
    using Base::Base;
    using Base::operator=;
};

struct Variable {
    enum DataTypes m_var_type;
    std::string m_var_name;
    std::shared_ptr<Expr> m_var_init; // ConstVar
};

struct InitExpr : public std::vector<std::shared_ptr<Expr>> {
    // vector of Variables
    using Base = std::vector<std::shared_ptr<Expr>>;
    using Base::Base;
    using Base::operator=;
};

struct Unary {
    std::shared_ptr<Expr> m_operand;
    enum Operators m_operator;
};

struct Binary {
    std::shared_ptr<Expr> m_operand1;
    std::shared_ptr<Expr> m_operand2;
    enum Operators m_operator;
};

struct IfElse {
    std::shared_ptr<Expr> m_condi;
    std::shared_ptr<Expr> m_if;
    std::shared_ptr<Expr> m_else;
};

//> Every loop can be converted to while-loop
struct WhileLoop {
    std::shared_ptr<Expr> m_condi;
    std::shared_ptr<Expr> m_loop_body;
};

/**
 * @code:
 * for (init;condition;iter) {
 *     loop_body
 * }
 *
 */
// struct ForLoop {
//     std::shared_ptr<Expr> m_init; // init
//     std::shared_ptr<Expr> m_condi; // condition
//     std::vector<std::shared_ptr<Expr>> m_loop_body; // loop body + iter
// };

// struct DoWhileLoop {
//     std::shared_ptr<Expr> m_condi;
//     std::vector<std::shared_ptr<Expr>> m_loop_body;
// };

struct Return {
    std::shared_ptr<Expr> m_expr;
};

struct FuncCall {
    std::vector<std::shared_ptr<Expr>> m_para_list;
    std::string m_func_name;
};

struct FuncDef {
    std::string m_name;
    std::vector<std::shared_ptr<Expr>> m_para_list; // should put `Variable` here
    std::shared_ptr<Expr> m_func_body;
    enum DataTypes m_return_type;
};

namespace impl { // Magic Base
using Base = std::variant<Variable, ConstVar, InitExpr, Unary, Binary, IfElse, WhileLoop, Return,
                          FuncCall, FuncDef, CompoundExpr>;
}

// dummy warpper for variant
struct Expr : public impl::Base {
    using impl::Base::Base;      // inhert ctors
    using impl::Base::operator=; // inhert

    // force move when copied
    // NOTE: empty warpper, so &Expr == &Base
    Expr(Expr const &other) : impl::Base(std::move(*((impl::Base *) &other))) {}

    template <typename T>
    [[nodiscard]] constexpr bool is() const {
        return std::holds_alternative<T>(*this);
    }
    template <typename T>
    [[nodiscard]] constexpr T &as() {
        if (auto *ptr = std::get_if<T>(this)) {
            return *ptr;
        } else {
            assert(false && "Bad conversion for variant!");
        }
        unreachable();
    }
};