#include "AST.hpp"
#include "constexpr_map.hpp"
#include <bits/stdc++.h>

using namespace std;

// void parse(Expr &e) {
//     match(
//         move(e),
//         [](FuncDef &&expr) {
//             cout << "Function: " << expr.m_name << endl;
//             for (const auto &para : expr.m_para_list) {
//                 parse(*para);
//             }
//             for (const auto &func_expr : expr.m_func_body->as<CompoundExpr>().m_expr_list) {
//                 parse(*func_expr);
//             }
//             cout << "Return Type = " << expr.m_return_type << endl;
//         },
//         [](const Variable &expr) {
//             cout << "This is a variable: Type=" << expr.m_var_type << " Name=" << expr.m_var_name
//                  << endl;
//         },
//         [](const auto &) { cout << "Error!" << endl; } // wildcard
//     );
// }

// enum test_enum { AAA, BBB, CCC, COUNT };

// constexpr ConstexprMap<enum test_enum, std::string_view, test_enum::COUNT> enum_map{
//     {AAA, "AAA"}, {BBB, "BBB"}, {CCC, "CCC"}};

// // enum test_enum2 {DDD, COUNT};

// int main() {
//     // clang-format off
//     auto e = Expr{FuncDef{
//         "foo",
//         {
//             // make_unique<Expr>(Variable{
//             //     DataTypes::Int,
//             //     "x"
//             // }),
//         },
//         {},
//         DataTypes::Void
//     }};
//     // clang-format on

//     e.as<FuncDef>().m_para_list.emplace_back(make_unique<Expr>(Variable{DataTypes::Int, "x"}));
//     e.as<FuncDef>().m_func_body->as<CompoundExpr>().m_expr_list.emplace_back(
//         make_unique<Expr>(Return{make_unique<Expr>(ConstVar{2})}));
//     parse(e);

//     auto vv = make_unique<Expr>(Variable{DataTypes::Float, "v"});
//     cout << vv->is<Variable>() << endl; // true
//     cout << vv->is<InitExpr>() << endl; // false
//     cout << enum_map[AAA] << endl;
// }

int main() {}
