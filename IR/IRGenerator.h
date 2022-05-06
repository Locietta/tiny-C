#pragma once

#include "AST.h"

namespace fs = std::filesystem;

class SymbolTable {
public:
    void push_scope();
    void pop_scope();

    [[nodiscard]] bool contains(llvm::StringRef var_name) const;
    void insert(llvm::StringRef var_name, llvm::Value *val);

    llvm::Value *const &operator[](llvm::StringRef var_name) const;

private:
    llvm::StringMap<llvm::Value *> globals;                   // GlobalVariable *
    llvm::SmallVector<llvm::StringMap<llvm::Value *>> locals; // llvm::AllocaInst *
};

class IRGenerator {
public:
    IRGenerator() = delete;
    IRGenerator(std::vector<std::shared_ptr<Expr>> const &trees);

    void printIR(fs::path const &asm_path) const;
    void codegen();

private:
    llvm::Value *visitASTNode(const Expr &expr);
    llvm::Type *getLLVMType(enum DataTypes);

    std::vector<std::shared_ptr<Expr>> const &m_trees; // reference to ASTBuilder

    std::unique_ptr<llvm::LLVMContext> m_context_ptr;
    std::unique_ptr<llvm::Module> m_module_ptr;
    std::unique_ptr<llvm::IRBuilder<>> m_builder_ptr;

    std::unique_ptr<SymbolTable> m_symbolTable_ptr;
};