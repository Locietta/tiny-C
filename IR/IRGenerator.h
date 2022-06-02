#pragma once

#include "AST.hpp"

namespace fs = std::filesystem;

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

struct IRAnalysis {
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;
};

class IRGenerator {
public:
    IRGenerator() = delete;
    IRGenerator(std::vector<std::shared_ptr<Expr>> const &trees, int opt_level);

    void codegen();

    /// should be called only after codegen is done
    void dumpIR(fs::path const &asm_path) const;
    [[nodiscard]] std::string dumpIRString() const;
    void emitOBJ(fs::path const &asm_path);

private:
    llvm::Value *visitASTNode(const Expr &expr);
    llvm::Type *getLLVMType(enum DataTypes);
    llvm::Value *boolCast(llvm::Value *val);

    void emitBlock(llvm::BasicBlock *BB, bool IsFinished = false);

    std::vector<std::shared_ptr<Expr>> m_simplifiedAST;

    std::unique_ptr<llvm::LLVMContext> m_context_ptr;
    std::unique_ptr<llvm::Module> m_module_ptr;
    std::unique_ptr<llvm::IRBuilder<>> m_builder_ptr;

    std::unique_ptr<IRAnalysis> m_analysis;
    std::unique_ptr<llvm::ModulePassManager> m_optimizer;

    std::unique_ptr<SymbolTable> m_symbolTable_ptr;
};