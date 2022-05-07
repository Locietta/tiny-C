#include "IRGenerator.h"

using namespace llvm;

// ------------ Implementation of `SymbolTable` -------------------

void SymbolTable::push_scope() {
    locals.push_back(StringMap<AllocaInst *>{});
}

void SymbolTable::pop_scope() {
    // NOTE: globals should never be poped
    locals.pop_back();
}

void SymbolTable::insert(llvm::StringRef var_name, llvm::AllocaInst *val) {
    assert(!locals.empty() && "No scope available for variable insertion!");
    locals.back().insert({var_name, val});
}

// array-like read
llvm::AllocaInst *SymbolTable::operator[](llvm::StringRef var_name) const {
    // NOLINTNEXTLINE
    for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
        const auto &map = *it;
        if (auto pair_it = map.find(var_name); pair_it != map.end()) {
            return pair_it->getValue();
        }
    }
    return nullptr;
}

bool SymbolTable::contains(llvm::StringRef var_name) const {
    // NOLINTNEXTLINE
    for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
        const auto &map = *it;
        if (auto pair_it = map.find(var_name); pair_it != map.end()) {
            return true;
        }
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
               "Error Category: {}, Code: {}, Message: {}\n",
               ec.category().name(),
               ec.value(),
               ec.message());
}

void IRGenerator::codegen() {
    for (const auto &tree : m_trees) {
        if (tree->is<InitExpr>()) {
            // global vars
            for (const auto &p_node : tree->as<InitExpr>()) {
                const auto &var = p_node->as<Variable>();
                auto var_type = getLLVMType(var.m_var_type);
                auto *globalVar = cast<GlobalVariable>(
                    m_module_ptr->getOrInsertGlobal(var.m_var_name, var_type) //
                );
                if (var.m_var_init) {
                    auto init = cast<Constant>(visitASTNode(*var.m_var_init));
                    if (init->getType() != var_type) {
                        std::string err_msg;
                        fmt::format_to(std::back_inserter(err_msg),
                                       "Unmatched initializer type for global var:{}\n",
                                       var.m_var_name);
                        throw std::logic_error(err_msg);
                    }
                    globalVar->setInitializer(init);
                }
            }
        } else {
            // a func
            visitASTNode(*tree);
        }
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
        [&, this](FuncDef const &func_node) -> Value * {
            Function *p_func = module.getFunction(func_node.m_name);

            if (!p_func) { // generate func proto if not exist
                SmallVector<Type *> funcArgsTypes;
                for (const auto &p_para : func_node.m_para_list) {
                    const auto &para = p_para->as<Variable>();
                    funcArgsTypes.push_back(getLLVMType(para.m_var_type));
                }
                Type *retType = getLLVMType(func_node.m_return_type);
                FunctionType *func_type = FunctionType::get(retType, funcArgsTypes, false);
                p_func = Function::Create(func_type,
                                          Function::ExternalLinkage,
                                          func_node.m_name,
                                          module);

                for (size_t i = 0; auto &arg : p_func->args()) {
                    const auto &para = func_node.m_para_list[i++]->as<Variable>();
                    arg.setName(para.m_var_name);
                }
            }

            if (!p_func) { // somehow fail to generate func proto
                std::string err_msg;
                fmt::format_to(std::back_inserter(err_msg),
                               "Failed to generate function prototype for `{}`\n",
                               func_node.m_name);
                throw std::logic_error(err_msg);
            }

            // Create new basic block
            BasicBlock *BB = BasicBlock::Create(context, "func_entry", p_func);
            builder.SetInsertPoint(BB);
            symTable.push_scope();

            for (auto &arg : p_func->args()) {
                AllocaInst *alloc = CreateEntryBlockAlloca(p_func, arg.getName(), arg.getType());
                builder.CreateStore(&arg, alloc);
                symTable.insert(arg.getName(), alloc);
            }

            // TODO: func body
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
