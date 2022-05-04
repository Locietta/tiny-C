#pragma once

#include <utility>

#include "AST.h"

class IRGenerator {
public:
    IRGenerator() = delete;
    IRGenerator(std::vector<std::shared_ptr<Expr>> const &trees);
    IRGenerator(std::vector<std::shared_ptr<Expr>> &&trees);

private:
    std::vector<std::shared_ptr<Expr>> m_trees;

    std::unique_ptr<llvm::LLVMContext> m_context;
    std::unique_ptr<llvm::Module> m_module;
    std::unique_ptr<llvm::IRBuilder<>> m_builder;

    llvm::StringMap<llvm::Value *> symbolTable;
};