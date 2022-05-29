#pragma once

#include "llvm/Support/CommandLine.h"

struct SilentDefaultOpts { // hack to shut up unwanted options in --help
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
        llvm::cl::Positional,
        llvm::cl::desc("<input file>"),
        llvm::cl::init("-"),
    };
};