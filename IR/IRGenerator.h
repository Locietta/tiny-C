#pragma once

#include "AST.h"

namespace fs = std::filesystem;

class IRGenerator {

public:
    IRGenerator() = delete;
    IRGenerator(std::vector<std::shared_ptr<Expr>> const &trees);

    void printIR(fs::path const &asm_path) const;
    void codegen();

private:
    llvm::Value *visitAST(const Expr &expr);
    std::vector<std::shared_ptr<Expr>> const &m_trees; // reference to ASTBuilder

    std::unique_ptr<llvm::LLVMContext> m_context;
    std::unique_ptr<llvm::Module> m_module;
    std::unique_ptr<llvm::IRBuilder<>> m_builder;

    llvm::StringMap<llvm::AllocaInst *> m_symbolTable;
    std::map<std::string, enum DataTypes> m_varTypeTable;

    bool m_is_global = true;

    llvm::Type *getLLVMType(enum DataTypes);
    llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction, llvm::StringRef VarName,
                                             llvm::Type *type);
};