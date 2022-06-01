#include <future>
#include <iostream>
#include <memory>

#include "ASTBuilder.h"
#include "ASTPrinter.h"
#include "CLexer.h"
#include "CParser.h"
#include "IRGenerator.h"
#include "OptHandler.h"
#include "antlr4-runtime.h"

using namespace antlrcpp;
using namespace antlr4;

namespace fs = std::filesystem;

OptHandler cli_inputs;

int main(int argc, const char *argv[]) {
    llvm::cl::ParseCommandLineOptions(argc, argv, "Simple compiler for C", nullptr, nullptr, false);

    std::unique_ptr<std::ifstream> is{
        make_unique<std::ifstream>(cli_inputs.input_filename, std::ios_base::in),
    };

    ANTLRInputStream input(*is);
    CLexer lexer(&input);
    CommonTokenStream tokens(&lexer);

    CParser parser(&tokens);
    tree::ParseTree *tree = parser.prog();

    ASTBuilder visitor;
    visitor.visit(tree);

    std::string output_dir{"output"};
    system(fmt::format("mkdir -p {} && rm -rf {}/*", output_dir, output_dir).c_str());

    // async: launch png generation in a separate thread
    auto png_gen_complete =
        std::async(std::launch::async, [m_decls = visitor.m_decls, &output_dir, argv]() {
            for (int i = 0; const auto &decl : m_decls) {
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

    IRGenerator builder{visitor.m_decls, cli_inputs.opt_level};
    builder.codegen();

    // TODO: output filename should correspond to input filename
    auto ir_print_complete = builder.dumpIR("output/a.ll");

    builder.emitOBJ("output/a.o").wait();

    // wait for async threads, this avoids early destruction of resources
    ir_print_complete.wait();
    return 0;
}