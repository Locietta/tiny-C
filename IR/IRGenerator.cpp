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

llvm::Type *IRGenerator::getLLVMType(enum DataTypes type) {
    switch (type) {
    case Int: return llvm::Type::getInt32Ty(*m_context); break;
    case Float: return llvm::Type::getFloatTy(*m_context); break;
    case Char: return llvm::Type::getInt8Ty(*m_context); break;
    default: break;
    }
    return llvm::Type::getVoidTy(*m_context);
}

llvm::AllocaInst *IRGenerator::CreateEntryBlockAlloca(llvm::Function *TheFunction,
                                                      llvm::StringRef VarName, llvm::Type *type) {
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(type, nullptr, VarName);
}

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
            Value *location = m_symbolTable[var_name];
            if (!location) {
                llvm_unreachable("Undeclared Var!");
                return nullptr;
            } else {
                return m_builder->CreateLoad(getLLVMType(m_varTypeTable[var_name]),
                                             location,
                                             var_name.c_str());
            }
        },
        [this](Variable const &var) -> Value * {
            return nullptr;
            // to be done
        },
        [this](FuncCall const &func_call) -> Value * {
            // Look up the name in the global module table.
            Function *CalleeF = m_module->getFunction(func_call.m_func_name);
            if (!CalleeF) {
                llvm_unreachable("Unknown function referenced");
                return nullptr;
            }

            // If argument mismatch error.
            if (CalleeF->arg_size() != func_call.m_para_list.size()) {
                llvm_unreachable("Incorrect # arguments passed");
                return nullptr;
            }

            std::vector<Value *> ArgsV;
            for (unsigned i = 0, e = func_call.m_para_list.size(); i != e; ++i) {
                ArgsV.push_back(visitAST(*(func_call.m_para_list[i])));
                if (!ArgsV.back()) return nullptr;
            }

            return m_builder->CreateCall(CalleeF, ArgsV, "calltmp");
        },
        [this](auto const &) -> Value * { llvm_unreachable("Invalid AST Node!"); });
}
