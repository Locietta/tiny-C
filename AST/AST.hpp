#pragma once

#include "constexpr_map.hpp"
#include "utility.hpp"
#include "variant_magic.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>
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
    PlusAssign,   // +=
    MinusAssign,  // -=
    MulAssign,    // *=
    DivAssign,    // /=
    ModAssign,    // %=
    Assign,       // =
    OrOr,         // ||
    AndAnd,       // &&
    Not,          // !

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

constexpr ConstexprMap<enum Operators, std::string_view, enum_op_count> op_to_str{
    {Plus, "+"},       {PlusPlus, "++"},   {Minus, "-"},         {MinusMinus, "--"},
    {Mul, "*"},        {Div, "/"},         {Mod, "%"},           {Equal, "=="},
    {Greater, ">"},    {Less, "<"},        {GreaterEqual, ">="}, {LessEqual, "<="},
    {NotEqual, "!="},  {PlusAssign, "+="}, {MinusAssign, "-="},  {MulAssign, "*="},
    {DivAssign, "/="}, {ModAssign, "%="},  {Assign, "="},        {OrOr, "||"},
    {AndAnd, "&&"},    {Not, "!"},
};

constexpr ConstexprMap<enum DataTypes, std::string_view, enum_type_count> type_to_str{
    {Void, "void"},
    {Char, "char"},
    {Int, "int"},
    {Float, "float"},
    {String, "string"},
    {Short, "short"},
    {Long, "long"},
};

namespace static_check {

constexpr auto check_map = []() { // magic: compile-time check
    static_assert(op_to_str.sz == enum_op_count, "Incomplete dispatch for op_enum->string map!");
    static_assert(type_to_str.sz == enum_type_count,
                  "Incomplete dispatch for type_enum->string map!");
    return true; // no-use
}();

} // namespace static_check

// ------------------------------ AST Nodes --------------------------------

/// Forward Declaration
struct Expr; // generic node

// dummy null expr node
struct Null {};

using CompoundExpr = std::vector<std::shared_ptr<Expr>>;

struct ConstVar : public std::variant<bool, char, int, float, double, std::string> {
    using Base = std::variant<bool, char, int, float, double, std::string>;
    using Base::Base;
    using Base::operator=;

    template <typename T>
    [[nodiscard]] constexpr bool is() const {
        return std::holds_alternative<T>(*this);
    }
    template <typename T>
    constexpr const T &as() const {
        return std::get<T>(*this);
    }
    template <typename T>
    constexpr T &as() {
        return std::get<T>(*this);
    }
};

using NameRef = std::string;

struct Variable {
    enum DataTypes m_var_type;
    NameRef m_var_name;
    std::shared_ptr<Expr> m_var_init; // ConstVar
    // int m_array_size;   // if variable is not array, then size = 0
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

struct Break {};

struct Continue {};

/**
 * @code:
 * for (init;condition;iter) {
 *     loop_body
 * }
 *
 */
struct ForLoop {
    std::shared_ptr<Expr> m_init;      // init
    std::shared_ptr<Expr> m_condi;     // condition
    std::shared_ptr<Expr> m_iter;      // iter
    std::shared_ptr<Expr> m_loop_body; // loop body
};

struct Return {
    std::shared_ptr<Expr> m_expr;
};

struct FuncCall {
    std::vector<std::shared_ptr<Expr>> m_para_list;
    NameRef m_func_name;
};

struct FuncProto {
    NameRef m_name;
    std::vector<std::shared_ptr<Expr>> m_para_list; // should put `Variable` here
    enum DataTypes m_return_type;
    [[nodiscard]] std::string_view getName() const { return m_name; }
};

struct FuncDef {
    std::shared_ptr<Expr> m_proto;
    std::shared_ptr<Expr> m_body;
    [[nodiscard]] std::string_view getName() const;
};

namespace impl { // Magic Base
using Base =
    std::variant<Variable, ConstVar, InitExpr, Unary, Binary, IfElse, WhileLoop, Return, FuncCall,
                 FuncProto, FuncDef, CompoundExpr, NameRef, Continue, Break, ForLoop, Null>;
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
    [[nodiscard]] constexpr const T &as() const {
        return std::get<T>(*this);
    }
    template <typename T>
    constexpr T &as() {
        return std::get<T>(*this);
    }
};

// ------------------- Inline Methods Implementation ---------------------

/// workaround forward declaration of Expr

inline std::string_view FuncDef::getName() const {
    return m_proto->as<FuncProto>().getName();
}
