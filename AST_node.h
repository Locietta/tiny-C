#pragma once
#include <any>
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
    Mod
    // etc
};

enum DataTypes {
    Int,
    Double
    // etc
};

// ------------------------------ AST Node --------------------------------
class BaseNode {
public:
    virtual ~BaseNode() {}
};

class Variable : public BaseNode {
public:
    virtual ~Variable() {}
    std::string m_var_name;
};

class ConstVar : public BaseNode {
public:
    virtual ~ConstVar() {}
    std::any m_value;
};

class Unary : public BaseNode {
public:
    virtual ~Unary() {}
    std::unique_ptr<BaseNode> m_operand;
    enum Operators m_operator;
};

class Binary : public BaseNode {
public:
    virtual ~Binary() {}
    std::unique_ptr<BaseNode> m_operand1;
    std::unique_ptr<BaseNode> m_operand2;
    enum Operators m_operator;
};

class FuncCall : public BaseNode {
public:
    virtual ~FuncCall() {}
    std::vector<std::unique_ptr<BaseNode>> m_para_list;
    std::string m_func_name;
};

class IfElse : public BaseNode {
public:
    virtual ~IfElse() {}
    std::unique_ptr<BaseNode> m_condi;
    std::vector<std::unique_ptr<BaseNode>> m_if;
    std::vector<std::unique_ptr<BaseNode>> m_elses;
};

class ForLoop : public BaseNode {
public:
    virtual ~ForLoop() {}
    std::unique_ptr<BaseNode> m_condi;
    std::unique_ptr<BaseNode> m_loop_end;
    std::vector<std::unique_ptr<BaseNode>> m_loop_body;
};

class WhileLoop : public BaseNode {
public:
    virtual ~WhileLoop() {}
    std::unique_ptr<BaseNode> m_condi;
    std::vector<std::unique_ptr<BaseNode>> m_loop_body;
};

class DoWhileLoop : public BaseNode {
public:
    virtual ~DoWhileLoop() {}
    std::unique_ptr<BaseNode> m_condi;
    std::vector<std::unique_ptr<BaseNode>> m_loop_body;
};

class Return : public BaseNode {
public:
    virtual ~Return() {}
    std::unique_ptr<BaseNode> m_expr;
};

class FuncDef {
public:
    virtual ~FuncDef() {}

private:
    std::string m_name;
    enum DataTypes m_return_type;
    std::vector<enum DataTypes> m_para_types;
    std::vector<std::unique_ptr<BaseNode>> m_func_body;
};
