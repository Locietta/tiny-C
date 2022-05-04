#pragma once

#include <utility>

#include "AST.h"

namespace fs = std::filesystem;

class IRGenerator {

public:
    IRGenerator() = delete;
    IRGenerator(std::vector<std::shared_ptr<Expr>> const &trees);
    IRGenerator(std::vector<std::shared_ptr<Expr>> &&trees);

    void printIR(fs::path const &asm_path) const;
    void codegen();

private:
    std::vector<std::shared_ptr<Expr>> m_trees;

    std::unique_ptr<llvm::LLVMContext> m_context;
    std::unique_ptr<llvm::Module> m_module;
    std::unique_ptr<llvm::IRBuilder<>> m_builder;

    llvm::StringMap<llvm::Value *> symbolTable;
};