#include <iostream>
#include <memory>

#include "CLexer.h"
#include "CParser.h"
#include "antlr4-runtime.h"
#include "my_visitor.h"

using namespace antlrcpp;
using namespace antlr4;

int main(int argc, const char *argv[]) {
    std::shared_ptr<std::ifstream> file_holder;
    std::istream *is = nullptr;
    if (argc > 1) {
        file_holder = make_shared<std::ifstream>(argv[1], std::ios_base::in);
        if (!*file_holder) {
            fmt::print("Failed to open file {}! Aborting...\n", argv[1]);
            return 1;
        }
        is = file_holder.get();
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

    if (visitor.m_func_roots.empty() && visitor.m_global_vars.empty()) {
        fmt::print("No AST is generated...\n");
        return 0;
    }
    ASTPrinter print{visitor.m_global_vars[0]};
    print.ToPNG(argv[0], "hahaha");
    return 0;
}