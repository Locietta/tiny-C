#include "IRGenerator.h"

using namespace llvm;

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

llvm::AllocaInst *IRGenerator::CreateEntryBlockAlloca(llvm::Function *TheFunction,
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
            Value *init_value;

            // initial value
            if (var.m_var_init) {
                // has init value
                init_value = visitAST(*var.m_var_init);
            } else {
                // no init value, give it default value
                init_value = ConstantFP::get(*m_context, APFloat(0.0));
            }

            // back up variables with the same name
            if (m_curr_func) {
                // local var
                m_localVars.push_back(var.m_var_name);
                m_addrBackUps.push_back(m_symbolTable[var.m_var_name]);
            }

            // allocate memory
            AllocaInst *alloca =
                CreateEntryBlockAlloca(m_curr_func, var.m_var_name, getLLVMType(var.m_var_type));
            // initialize it
            m_builder->CreateStore(init_value, alloca);
            // register it in symbol table
            m_symbolTable[var.m_var_name] = alloca;

            return nullptr;
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
                ArgsV.push_back(visitAST(*para));
                if (!ArgsV.back()) return nullptr;
            }

            return m_builder->CreateCall(CalleeF, ArgsV, "calltmp");
        },
        [this](IfElse const &if_node) -> Value * {
            llvm::Value *condValue = visitAST(*if_node.m_condi), *thenValue = nullptr,
                        *elseValue = nullptr;
            condValue = m_builder->CreateICmpNE(
                condValue,
                llvm::ConstantInt::get(llvm::Type::getInt1Ty(*m_context), 0, true),
                "ifCond");

            llvm::Function *TheFunction = m_curr_func;
            llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(*m_context, "then", TheFunction);
            llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(*m_context, "else", TheFunction);
            llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(*m_context, "merge", TheFunction);

            // Then
            auto branch = m_builder->CreateCondBr(condValue, thenBB, elseBB);
            m_builder->SetInsertPoint(thenBB);
            thenValue = visitAST(*if_node.m_if);
            m_builder->CreateBr(mergeBB);
            thenBB = m_builder->GetInsertBlock();

            // else
            m_builder->SetInsertPoint(elseBB);
            elseValue = visitAST(*if_node.m_else);
            m_builder->CreateBr(mergeBB);
            elseBB = m_builder->GetInsertBlock();

            m_builder->SetInsertPoint(mergeBB);
            return branch;
        },
        [this](FuncDef const &func_node) -> Value * {
            // analyze param list
            std::vector<llvm::Type *> params;
            for (auto &param_node : func_node.m_para_list) {
                auto param_type = param_node->as<Variable>().m_var_type;
                params.push_back(getLLVMType(param_type));
            }
            // create func prototype
            FunctionType *FT =
                llvm::FunctionType::get(getLLVMType(func_node.m_return_type), params, false);
            // create func
            Function *func = llvm::Function::Create(FT,
                                                    llvm::GlobalValue::ExternalLinkage,
                                                    func_node.m_name,
                                                    m_module.get());

            // create Block
            llvm::BasicBlock *newBlock =
                llvm::BasicBlock::Create(*m_context, func_node.m_name + "_entry", func);
            m_builder->SetInsertPoint(newBlock);

            // set parameter name
            int index = 0;
            for (auto &Arg : func->args()) {
                Arg.setName(func_node.m_para_list[index++]->as<Variable>().m_var_name);
            }

            // build function body recursively
            visitAST(*func_node.m_body);

            // clear local variables in this function
            for (int i = 0; i < m_localVars.size(); i++) {
                m_symbolTable[m_localVars[i]] = m_addrBackUps[i];
            }
            m_localVars.clear();
            m_addrBackUps.clear();

            return func;
        },
        [this](auto const &) -> Value * { llvm_unreachable("Invalid AST Node!"); });
}
