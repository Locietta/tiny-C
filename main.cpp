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

    if (cli_inputs.input_filename.empty() || !fs::exists(cli_inputs.input_filename.c_str())) {
        fs::path exe_name{argv[0]};
        fmt::print("{}: error: no input files\n", exe_name.filename().native());
        return 1;
    }

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

    const auto &output_dir = cli_inputs.pic_outdir;

    if (cli_inputs.emitAST || cli_inputs.emitCFG) {
        fs::create_directory(output_dir.c_str());
        for (const auto &dir_entry : fs::directory_iterator{output_dir.c_str()}) {
            fs::remove_all(dir_entry);
        }
    }

    if (cli_inputs.emitAST) {
        auto dumpAST = [m_decls = visitor.m_decls, &output_dir, argv]() {
            for (int i = 0; const auto &decl : m_decls) {
                assert(decl->is<FuncDef>() || decl->is<InitExpr>() || decl->is<FuncProto>());
                ASTPrinter decl_printer{decl};
                fs::path pic_path{output_dir.c_str()};
                if (decl->is<FuncDef>()) {
                    pic_path.append(fmt::format("func:{}", decl->as<FuncDef>().getName()));
                } else {
                    pic_path.append(fmt::format("global_decl{}", i++));
                }
                decl_printer.ToPNG(argv[0], pic_path);
            }
        };

        // async: launch AST pic generation in a separate thread
        auto _ = std::async(std::launch::async, dumpAST);
    }

    IRGenerator builder{visitor.m_decls};
    builder.codegen();

    std::string out_name;
    if (cli_inputs.output_filename.empty()) {
        fs::path input{cli_inputs.input_filename.c_str()};
        out_name = input.stem();
    } else {
        fs::path output{cli_inputs.output_filename.c_str()};
        output.replace_extension("");
        out_name = output;
    }

    builder.dumpIR(fmt::format("{}.ll", out_name));

    builder.emitOBJ(fmt::format("{}.o", out_name));

    return 0;
}