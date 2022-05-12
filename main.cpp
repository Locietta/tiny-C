#include <future>
#include <iostream>
#include <memory>

#include "ASTBuilder.h"
#include "ASTPrinter.h"
#include "CLexer.h"
#include "CParser.h"
#include "IRGenerator.h"
#include "antlr4-runtime.h"

using namespace antlrcpp;
using namespace antlr4;

namespace fs = std::filesystem;

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

    ASTBuilder visitor;
    visitor.visit(tree);

    std::string output_dir{"output"};
    system(fmt::format("mkdir -p {} && rm -rf {}/*", output_dir, output_dir).c_str());

    // async: launch png generation in a separate thread
    auto png_gen_complete = std::async(std::launch::async, [&visitor, &output_dir, argv]() {
        for (int i = 0; const auto &decl : visitor.m_decls) {
            assert(decl->is<FuncDef>() || decl->is<InitExpr>());
            ASTPrinter decl_printer{decl};
            fs::path pic_path{output_dir};
            if (decl->is<FuncDef>()) {
                pic_path.append(fmt::format("func:{}", decl->as<FuncDef>().m_name));
            } else {
                pic_path.append(fmt::format("global_decl{}", i++));
            }
            decl_printer.ToPNG(argv[0], pic_path);
        }
    });

    IRGenerator builder{visitor.m_decls};
    builder.codegen(); // TODO: IR generation
    builder.printIR("output/a.ll");

    // avoid `visitor` destruction before png generation is done
    png_gen_complete.wait(); //< waiting png-gen
    return 0;
}