#pragma once

#include "AST.hpp"

namespace fs = std::filesystem;

template <typename T>
class SymbolTableMixin {
public:
    void push_scope();
    void pop_scope();

    [[nodiscard]] bool inCurrScope(llvm::StringRef sym_name) const;
    void insert(llvm::StringRef sym_name, T val);

    // array-like read
    T operator[](llvm::StringRef sym_name) const;

protected:
    llvm::SmallVector<llvm::StringMap<T>> symbols;
};

class SymbolTable : public SymbolTableMixin<llvm::AllocaInst *> {};

class TypeTable : public SymbolTableMixin<llvm::Type *> {
public:
    TypeTable(llvm::LLVMContext &ctx);
    ~TypeTable();
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
    llvm::Value *boolCast(llvm::Value *val);

    void emitBlock(llvm::BasicBlock *BB, bool IsFinished = false);

    std::vector<std::shared_ptr<Expr>> m_simplifiedAST;

    std::unique_ptr<llvm::LLVMContext> m_context_ptr;
    std::unique_ptr<llvm::Module> m_module_ptr;
    std::unique_ptr<llvm::IRBuilder<>> m_builder_ptr;

    std::unique_ptr<IRAnalysis> m_analysis;
    std::unique_ptr<llvm::ModulePassManager> m_optimizer;

    std::unique_ptr<SymbolTable> m_symbolTable_ptr;
    std::unique_ptr<TypeTable> m_typeTable_ptr;

private:
    struct scope_manager { // RAII scope manager
        IRGenerator &ir_gen;
        scope_manager(IRGenerator &ir_gen) : ir_gen{ir_gen} {
            ir_gen.m_symbolTable_ptr->push_scope();
            ir_gen.m_typeTable_ptr->push_scope();
        }
        ~scope_manager() {
            ir_gen.m_symbolTable_ptr->pop_scope();
            ir_gen.m_typeTable_ptr->pop_scope();
        }
    };
};

// --------------------- Implementation of `SymbolTableMixin<T>` --------------------------

template <typename T>
void SymbolTableMixin<T>::push_scope() {
    symbols.push_back(llvm::StringMap<T>{});
}

template <typename T>
void SymbolTableMixin<T>::pop_scope() {
    assert(!symbols.empty() && "try to pop scope from empty symbol table?");
    symbols.pop_back();
}

template <typename T>
bool SymbolTableMixin<T>::inCurrScope(llvm::StringRef sym_name) const {
    if (symbols.empty()) llvm_unreachable("try to query locals in null scope?");
    const auto &curr_scope = symbols.back();
    return curr_scope.find(sym_name) != curr_scope.end();
}

template <typename T>
void SymbolTableMixin<T>::insert(llvm::StringRef sym_name, T val) {
    assert(!symbols.empty() && "No scope available for variable insertion!");
    symbols.back().insert({sym_name, val});
}

template <typename T>
T SymbolTableMixin<T>::operator[](llvm::StringRef sym_name) const {
    // NOLINTNEXTLINE
    for (auto it = symbols.rbegin(); it != symbols.rend(); ++it) {
        const auto &map = *it;
        if (auto pair_it = map.find(sym_name); pair_it != map.end()) {
            return pair_it->getValue();
        }
    }
    return T{}; // == nullptr for pointer types
}