#pragma once

#include "AST.h"

namespace fs = std::filesystem;

class SymbolTable {
public:
    void push();
    void pop();

    [[nodiscard]] bool contains(llvm::StringRef var_name) const;

    llvm::Value *&operator[](llvm::StringRef var_name);
    llvm::Value *const &operator[](llvm::StringRef var_name) const;

private:
    llvm::StringMap<llvm::Value *> globals;                      // GlobalVariable *
    llvm::SmallVector<llvm::StringMap<llvm::Value *>, 8> locals; // llvm::AllocaInst *
};

class IRGenerator {
public:
    IRGenerator() = delete;
    IRGenerator(std::vector<std::shared_ptr<Expr>> const &trees);

    void printIR(fs::path const &asm_path) const;
    void codegen();

private:
    llvm::Value *visitASTNode(const Expr &expr);
    std::vector<std::shared_ptr<Expr>> const &m_trees; // reference to ASTBuilder

    std::unique_ptr<llvm::LLVMContext> m_context;
    std::unique_ptr<llvm::Module> m_module;
    std::unique_ptr<llvm::IRBuilder<>> m_builder;

    SymbolTable m_symbolTable;

    llvm::Type *getLLVMType(enum DataTypes);
};