#include "IRGenerator.h"

using namespace llvm;

IRGenerator::IRGenerator(std::vector<std::shared_ptr<Expr>> const &trees)
    : m_trees(trees), m_context(std::make_unique<llvm::LLVMContext>()),
      m_builder(std::make_unique<llvm::IRBuilder<>>(*m_context)),
      m_module(std::make_unique<llvm::Module>("tinycc JIT", *m_context)) {}

IRGenerator::IRGenerator(std::vector<std::shared_ptr<Expr>> &&trees)
    : m_trees(std::move(trees)), m_context(std::make_unique<llvm::LLVMContext>()),
      m_builder(std::make_unique<llvm::IRBuilder<>>(*m_context)),
      m_module(std::make_unique<llvm::Module>("tinycc JIT", *m_context)) {}

void IRGenerator::codegen() {
    for (const auto &tree : m_trees) {
        visitAST(*tree);
    }
}

Value *IRGenerator::visitAST(const Expr &expr) {
    return match<Value *>(
        expr,
        [this](ConstVar const &var) -> Value * {
            if (var.is<double>()) {
                return ConstantFP::get(*m_context, APFloat(var.as<double>()));
            } else if (var.is<int>()) {
                return ConstantInt::get(*m_context, APInt(32, var.as<int>(), true));
            } else if (var.is<char>()) {
                return ConstantInt::get(*m_context, APInt(8, var.as<int>()));
            }
            llvm_unreachable("Unsupported ConstVar type!");
        },
        [this](NameRef const &var_name) -> Value * {
            Value *ret = m_symbolTable[var_name];
            if (!ret) {
                llvm_unreachable("Undeclared Var!");
                return nullptr;
            } else {
                return ret;
            }
        },
        [this](auto const &) -> Value * { llvm_unreachable("Invalid AST Node!"); });
}