#pragma once

struct Expr; // generic node

void simplifyAST(std::vector<std::shared_ptr<Expr>> &AST);