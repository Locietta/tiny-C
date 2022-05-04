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

    for (int i = 0; const auto &func : visitor.m_func_roots) {
        ASTPrinter print_func{func};
        std::string pic_path;
        fmt::format_to(std::back_inserter(pic_path), "output/func{}", i++);
        print_func.ToPNG(argv[0], move(pic_path));
    }
    for (int i = 0; const auto &var : visitor.m_global_vars) {
        ASTPrinter print_var{var};
        std::string pic_path;
        fmt::format_to(std::back_inserter(pic_path), "output/var{}", i++);
        print_var.ToPNG(argv[0], move(pic_path));
    }
    return 0;
}