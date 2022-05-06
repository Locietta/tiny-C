#include "IRGenerator.h"

using namespace llvm;

// ------------ Implementation of `SymbolTable` -------------------

void SymbolTable::push_scope() {
    locals.push_back(StringMap<Value *>{});
}

void SymbolTable::pop_scope() {
    // NOTE: globals should never be poped
    if (!locals.empty()) locals.pop_back();
    llvm_unreachable("Scope mismatch!");
}

void SymbolTable::insert(llvm::StringRef var_name, llvm::Value *val) {
    // NOLINTNEXTLINE
    for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
        auto &map = *it;
        if (auto pair_it = map.find(var_name); pair_it != map.end()) {
            pair_it->getValue() = val;
        }
    }
    if (auto it = globals.find(var_name); it != globals.end()) {
        it->getValue() = val;
    }
    locals.back().insert({var_name, val});
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
    : m_trees(trees), m_context_ptr(std::make_unique<llvm::LLVMContext>()),
      m_builder_ptr(std::make_unique<llvm::IRBuilder<>>(*m_context_ptr)),
      m_module_ptr(std::make_unique<llvm::Module>("tinycc JIT", *m_context_ptr)),
      m_symbolTable_ptr(std::make_unique<SymbolTable>()) {}

llvm::Type *IRGenerator::getLLVMType(enum DataTypes type) {
    switch (type) {
    case Int: return llvm::Type::getInt32Ty(*m_context_ptr); break;
    case Float: return llvm::Type::getFloatTy(*m_context_ptr); break;
    case Char: return llvm::Type::getInt8Ty(*m_context_ptr); break;
    default: break;
    }
    return llvm::Type::getVoidTy(*m_context_ptr);
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
        out << *m_module_ptr;
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
    auto &context = *m_context_ptr;
    auto &module = *m_module_ptr;
    auto &builder = *m_builder_ptr;
    auto &symTable = *m_symbolTable_ptr;

    return match<Value *>(
        expr,
        [&, this](ConstVar const &var) -> Value * {
            if (var.is<double>()) {
                return ConstantFP::get(context, APFloat(var.as<double>()));
            } else if (var.is<int>()) {
                return ConstantInt::get(context, APInt(32, var.as<int>(), true));
            } else if (var.is<char>()) {
                return ConstantInt::get(context, APInt(8, var.as<int>()));
            }
            llvm_unreachable("Unsupported ConstVar type!");
        },
        [&, this](NameRef const &var_name) -> Value * {
            Value *location = symTable[var_name];
            if (!location) {
                llvm_unreachable("Undeclared Var!");
                return nullptr;
            } else {
                return builder.CreateLoad(location->getType(), location, var_name.c_str());
            }
        },
        // [this](InitExpr const &inits) -> Value * {
        //     // std::vector<AllocaInst *>
        // },
        [&, this](Variable const &var) -> Value * {
            auto *A = cast<AllocaInst>(symTable[var.m_var_name]);
            if (!A) {
                fmt::print(stderr, "Unknown variable name");
                return nullptr;
            }

            // Load the value.
            return builder.CreateLoad(A->getAllocatedType(), A, var.m_var_name.c_str());
        },
        [&, this](FuncCall const &func_call) -> Value * {
            // Look up the name in the global module table.
            Function *CalleeF = module.getFunction(func_call.m_func_name);
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

            return builder.CreateCall(CalleeF, ArgsV, "calltmp");
        },
        [](auto const &) -> Value * { llvm_unreachable("Invalid AST Node!"); });
}
