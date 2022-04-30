#include "AST.h"
#include <bits/stdc++.h>

using namespace std;

void parse(Expr &e) {
    match(
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
        [](const auto &) { cout << "Error!" << endl; } // wildcard
    );
}

struct B {};
struct A : public B {};
int main() {
    // A a;
    // vector<unique_ptr<B>> vv{make_unique<B>(a)}; // trouble maker: 50 lines error
    // vv.push_back(make_unique<B>(a));             // safe and sound

    // clang-format off
    auto e = Expr{FuncDef{
        "foo",
        {
            // make_unique<Expr>(Variable{
            //     DataTypes::Int,
            //     "x"
            // }),
        },
        {},
        DataTypes::Void
    }};
    // clang-format on

    get<FuncDef>(e).m_para_list.emplace_back(make_unique<Expr>(Variable{DataTypes::Int, "x"}));
    get<FuncDef>(e).m_func_body.emplace_back(
        make_unique<Expr>(Return{make_unique<Expr>(ConstVar{2})}));
    parse(e);
    // vector<unique_ptr<int>> vv{1};
    // printf("vv: %d", *vv[0]);
}