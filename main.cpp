/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

//
//  main.cpp
//  antlr4-cpp-demo
//
//  Created by Mike Lischke on 13.03.16.
//
#include <iostream>
#include <memory>

#include "CLexer.h"
#include "CParser.h"
#include "antlr4-runtime.h"
#include "my_visitor.h"

using namespace antlrcpp;
using namespace antlr4;

int main(int argc, const char *argv[]) {
    std::istream *is = nullptr;
    if (argc > 1) {
        is = new std::ifstream(argv[1], std::ios_base::in);
    } else {
        is = &std::cin;
    }

    ANTLRInputStream input(*is);
    CLexer lexer(&input);
    CommonTokenStream tokens(&lexer);

    tokens.fill();
    // for (auto token : tokens.getTokens()) {
    //     std::cout << token->toString() << std::endl;
    // }

    CParser parser(&tokens);
    tree::ParseTree *tree = parser.prog();

    // std::cout << tree->toStringTree(&parser) << std::endl << std::endl;

    my_visitor visitor;
    std::any test = visitor.visit(tree);

    ASTPrinter print{visitor.m_global_vars[0]};
    print.ToPNG(argv[0], "hahaha");

    if (argc > 1) delete is;

    return 0;
}