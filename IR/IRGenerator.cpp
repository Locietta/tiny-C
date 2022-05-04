#include "IRGenerator.h"

IRGenerator::IRGenerator(std::vector<std::shared_ptr<Expr>> const &trees)
    : m_trees(trees), m_context(std::make_unique<llvm::LLVMContext>()),
      m_builder(std::make_unique<llvm::IRBuilder<>>(*m_context)),
      m_module(std::make_unique<llvm::Module>("tinycc JIT", *m_context)) {}

IRGenerator::IRGenerator(std::vector<std::shared_ptr<Expr>> &&trees)
    : m_trees(std::move(trees)), m_context(std::make_unique<llvm::LLVMContext>()),
      m_builder(std::make_unique<llvm::IRBuilder<>>(*m_context)),
      m_module(std::make_unique<llvm::Module>("tinycc JIT", *m_context)) {}