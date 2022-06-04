#include "ASTSimplify.h"
#include "AST.hpp"
#include "utility.hpp"
#include "variant_magic.hpp"

using namespace std;

inline bool isTerminator(Expr const &expr) {
    return expr.is<Return>() || expr.is<Break>() || expr.is<Continue>();
}

static void simplifyVisitor(Expr &expr) {
    if (expr.is<CompoundExpr>()) {
        auto &comp = expr.as<CompoundExpr>();
        int i = 0;
        for (; i < comp.size(); ++i) {
            if (isTerminator(*comp[i])) {
                comp.resize(i + 1);
                break;
            }
            simplifyVisitor(*comp[i]);
        }
        return;
    }

    if (expr.is<FuncDef>()) {
        simplifyVisitor(*expr.as<FuncDef>().m_body);
    } else if (expr.is<IfElse>()) {
        simplifyVisitor(*expr.as<IfElse>().m_if);
        if (expr.as<IfElse>().m_else) simplifyVisitor(*expr.as<IfElse>().m_else);
    } else if (expr.is<WhileLoop>()) {
        simplifyVisitor(*expr.as<WhileLoop>().m_loop_body);
    } else if (expr.is<ForLoop>()) {
        simplifyVisitor(*expr.as<ForLoop>().m_loop_body);
    }
}

void simplifyAST(std::vector<std::shared_ptr<Expr>> &ASTs) {
    //
    for (const auto &AST : ASTs) {
        if (AST->is<InitExpr>()) continue;

        // funcdef
        simplifyVisitor(*AST);
    }
}
