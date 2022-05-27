#pragma once

#include "AST.hpp"

namespace fs = std::filesystem;

#define ASYNC

class SymbolTable {
public:
    void push_scope();
    void pop_scope();

    [[nodiscard]] bool inCurrScope(llvm::StringRef var_name) const;
    void insert(llvm::StringRef var_name, llvm::AllocaInst *val);

    llvm::AllocaInst *operator[](llvm::StringRef var_name) const;

private:
    llvm::SmallVector<llvm::StringMap<llvm::AllocaInst *>> locals; // llvm::AllocaInst *
};

class IRGenerator {
public:
    IRGenerator() = delete;
    IRGenerator(std::vector<std::shared_ptr<Expr>> const &trees);

    void codegen();

    /// should be called only after codegen is done
    [[nodiscard]] std::future<void> dumpIR(fs::path const &asm_path) const ASYNC;
    [[nodiscard]] std::string dumpIRString() const;
    std::future<void> emitOBJ(fs::path const &asm_path) ASYNC;

private:
    llvm::Value *visitASTNode(const Expr &expr);
    llvm::Type *getLLVMType(enum DataTypes);
    llvm::Value *boolCast(llvm::Value *val);

    std::vector<std::shared_ptr<Expr>> const &m_trees; // reference to ASTBuilder

    std::unique_ptr<llvm::LLVMContext> m_context_ptr;
    std::unique_ptr<llvm::Module> m_module_ptr;
    std::unique_ptr<llvm::IRBuilder<>> m_builder_ptr;

    std::unique_ptr<llvm::legacy::FunctionPassManager> m_func_opt;

    std::unique_ptr<SymbolTable> m_symbolTable_ptr;
};