#pragma once

#include "constexpr_map.hpp"
#include "utility.hpp"
#include "variant_magic.hpp"

#include <cassert>
#include <fmt/core.h>
#include <fmt/format.h>
#include <memory>
#include <string>
#include <vector>

// ------------------------------ Operator Type ---------------------------
enum Operators {
    Plus = 0,
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
    NotEqual,     // !=
    PlusAssign,
    MinusAssign,
    MulAssign,
    DivAssign,
    ModAssign,
    Assign,
    OrOr,
    AndAnd,

    enum_op_count
    // etc
};

enum DataTypes {
    Void = 0,
    Char,
    Int,
    Float,
    String,
    Short,
    Long,
    enum_type_count
    // etc
};

constexpr static ConstexprMap<enum Operators, std::string_view, enum_op_count> op_map{
    {Plus, "+"},
    {PlusPlus, "++"},
    {Minus, "-"},
    {MinusMinus, "--"},
    {Mul, "*"},
    {Div, "/"},
    {Mod, "%"},
    {Equal, "=="},
    {Greater, ">"},
    {Less, "<"},
    {GreaterEqual, ">="},
    {LessEqual, "<="},
    {NotEqual, "!="},
};

constexpr static ConstexprMap<enum DataTypes, std::string_view, enum_type_count> type_map{
    {Void, "void"},
    {Char, "char"},
    {Int, "int"},
    {Float, "float"},
    {String, "string"},
    {Short, "short"},
    {Long, "long"},
};

// ------------------------------ AST Nodes --------------------------------

/// Forward Declaration
struct Expr; // generic node

using CompoundExpr = std::vector<std::shared_ptr<Expr>>;

struct ConstVar : public std::variant<char, int, float, double, std::string> {
    using Base = std::variant<char, int, float, double, std::string>;
    using Base::Base;
    using Base::operator=;

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

using NameRef = std::string;

struct Variable {
    enum DataTypes m_var_type;
    NameRef m_var_name;
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
    NameRef m_name;
    std::vector<std::shared_ptr<Expr>> m_para_list; // should put `Variable` here
    std::shared_ptr<Expr> m_func_body;
    enum DataTypes m_return_type;
};

namespace impl { // Magic Base
using Base = std::variant<Variable, ConstVar, InitExpr, Unary, Binary, IfElse, WhileLoop, Return,
                          FuncCall, FuncDef, CompoundExpr, NameRef>;
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

class ASTPrinter {
public:
    const Expr &AST;
    void ToPNG(const char *filename);
    void ToPNG(const std::string &filename);

    ASTPrinter(const Expr &e) : AST{e} {}

private:
    void sexp_fmt(const Expr &e);
    fmt::memory_buffer buffer; // buf for graphviz file output
};
