#pragma once

#include "llvm/Support/CommandLine.h"

// hack to shut up unwanted options in --help
struct SilentDefaultOpts {
    SilentDefaultOpts() {
        llvm::StringMap<llvm::cl::Option *> &optMap = llvm::cl::getRegisteredOptions();
        for (auto &opt : optMap) opt.getValue()->setHiddenFlag(llvm::cl::Hidden);
    }
};

// you should never alloc this huge object on stack...
struct OptHandler : SilentDefaultOpts {
    llvm::cl::opt<int> opt_level{
        "O",
        llvm::cl::desc("Choose optimization level"),
        llvm::cl::init(0),
    };

    llvm::cl::opt<std::string> input_filename{
        llvm::cl::Positional, llvm::cl::desc("<input file>"),
        // llvm::cl::init("-"),
    };

    llvm::cl::opt<std::string> output_filename{
        "o",
        llvm::cl::desc("Specify output filename"),
        llvm::cl::value_desc("filename"),
    };

    llvm::cl::opt<bool> emitAST{
        "emit-ast",
        llvm::cl::desc("Emit tree graph for all ASTs"),
    };

    llvm::cl::alias emitAST_short{
        "A",
        llvm::cl::desc("Alias for --emit-ast"),
        llvm::cl::aliasopt(emitAST),
        llvm::cl::NotHidden,
    };

    llvm::cl::opt<std::string> pic_outdir{
        "pic-dir",
        llvm::cl::desc("Specify output directory of pics, default to `output`"),
        llvm::cl::value_desc("dirname"),
        llvm::cl::init("output"),
    };

    llvm::cl::opt<bool> emitCFG{
        "emit-cfg",
        llvm::cl::desc("Emit Control Flow Graphs for all functions"),
    };

    llvm::cl::alias emitCFG_short{
        "C",
        llvm::cl::desc("Alias for --emit-cfg"),
        llvm::cl::aliasopt(emitCFG),
        llvm::cl::NotHidden,
    };

    llvm::cl::opt<bool> debugSExpr{
        "debug-sexpr",
        llvm::cl::desc("Output S-expression of generated AST to stdout"),
    };

    llvm::cl::opt<std::string> gcc_lib_version{
        "gcc-lib-version",
        llvm::cl::desc(
            "Specify the version gcc, used for linker to link the gcc lib. Default to 12.1.0"),
        llvm::cl::value_desc("version"),
        llvm::cl::init("12.1.0"),
    };
};
