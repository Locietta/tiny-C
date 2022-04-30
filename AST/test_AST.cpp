#include "AST.h"
#include <bits/stdc++.h>

using namespace std;

void parse(Expr &e) {
    match<void>(
        move(e),
        [](const FuncDef &expr) {
            cout << "Function: " << expr.m_name << endl;
            for (const auto &para : expr.m_para_list) {
                parse(*para);
            }
            for (const auto &func_expr : expr.m_func_body) {
                parse(*func_expr);
            }
            cout << "Return Type = " << expr.m_return_type << endl;
        },
        [](const Variable &expr) {
            cout << "This is a variable: Type=" << expr.m_var_type << " Name=" << expr.m_var_name
                 << endl;
        },
        [](const auto &) { cout << "Error!" << endl; });
}

int main() {
    // clang-format off
    auto e = Expr{FuncDef{
        "foo",
        {
            // make_unique<Expr>(Variable{
            //     DataTypes::Int,
            //     "x"
            // })
        },
        {},
        DataTypes::Void
    }};
    // // clang-format on
    parse(e);
    // vector<unique_ptr<int>> vv{1};
    // printf("vv: %d", *vv[0]);
}