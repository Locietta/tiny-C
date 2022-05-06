#include "IRGenerator.h"

using namespace llvm;

// ------------ Implementation of `SymbolTable` -------------------

void SymbolTable::push() {
    locals.push_back(StringMap<Value *>{});
}

void SymbolTable::pop() {
    // NOTE: globals should never be poped
    if (!locals.empty()) locals.pop_back();
    llvm_unreachable("Scope mismatch!");
}

// array-like write
llvm::Value *&SymbolTable::operator[](llvm::StringRef var_name) {
    // NOLINTNEXTLINE
    for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
        auto &map = *it;
        if (auto pair_it = map.find(var_name); pair_it != map.end()) {
            return pair_it->getValue();
        }
    }
    if (auto it = globals.find(var_name); it != globals.end()) {
        return it->getValue();
    }
    return locals.back()[var_name];
}

// array-like read
llvm::Value *const &SymbolTable::operator[](llvm::StringRef var_name) const {
    // NOLINTNEXTLINE
    for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
        const auto &map = *it;
        if (auto pair_it = map.find(var_name); pair_it != map.end()) {
            return pair_it->getValue();
        }
    }
    if (auto it = globals.find(var_name); it != globals.end()) {
        return it->getValue();
    }

    // invalid access
    std::string err_msg;
    fmt::format_to(std::back_inserter(err_msg), "Access to undefined variable `{}`!", var_name);
    throw std::logic_error(err_msg);
}

bool SymbolTable::contains(llvm::StringRef var_name) const {
    // NOLINTNEXTLINE
    for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
        const auto &map = *it;
        if (auto pair_it = map.find(var_name); pair_it != map.end()) {
            return true;
        }
    }
    if (auto it = globals.find(var_name); it != globals.end()) {
        return true;
    }
    return false;
}

// ------------ Implementation of `IRGenerator` -------------------

IRGenerator::IRGenerator(std::vector<std::shared_ptr<Expr>> const &trees)
    : m_trees(trees), m_context(std::make_unique<llvm::LLVMContext>()),
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

static llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction,
                                                llvm::StringRef VarName, llvm::Type *type) {
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(type, nullptr, VarName);
}

void IRGenerator::printIR(fs::path const &asm_path) const {
    std::error_code ec;
    raw_fd_ostream out(asm_path.native(), ec);
    if (!ec) {
        out << *m_module;
        return;
    }

    // error when opening file
    fmt::print(stderr,
               "Error Category: {}, Code: {}, Message: {}",
               ec.category().name(),
               ec.value(),
               ec.message());
}

void IRGenerator::codegen() {
    for (const auto &tree : m_trees) {
        visitASTNode(*tree);
    }
}

Value *IRGenerator::visitASTNode(const Expr &expr) {
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
                return m_builder->CreateLoad(location->getType(), location, var_name.c_str());
            }
        },
        // [this](InitExpr const &inits) -> Value * {
        //     // std::vector<AllocaInst *>
        // },
        [this](Variable const &var) -> Value * {
            auto *A = cast<AllocaInst>(m_symbolTable[var.m_var_name]);
            if (!A) {
                fmt::print(stderr, "Unknown variable name");
                return nullptr;
            }

            // Load the value.
            return m_builder->CreateLoad(A->getAllocatedType(), A, var.m_var_name.c_str());
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
            for (const auto &para : func_call.m_para_list) {
                ArgsV.push_back(visitASTNode(*para));
                if (!ArgsV.back()) return nullptr;
            }

            return m_builder->CreateCall(CalleeF, ArgsV, "calltmp");
        },
        [this](auto const &) -> Value * { llvm_unreachable("Invalid AST Node!"); });
}
