#include "IRGenerator.h"
#include "utility.hpp"

using namespace llvm;

struct scope_manager { // RAII scope manager
    SymbolTable &sym;
    scope_manager(SymbolTable &sym) : sym{sym} { sym.push_scope(); }
    ~scope_manager() { sym.pop_scope(); }
};

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

bool SymbolTable::inCurrScope(llvm::StringRef var_name) const {
    if (locals.empty()) llvm_unreachable("try to query locals in null scope?");
    const auto &curr_scope = locals.back();
    return curr_scope.find(var_name) != curr_scope.end();
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
                        throw_err("Unmatched initializer type for global var:{}\n", var.m_var_name);
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

static bool isFloat(Value *val) {
    Type *valT = val->getType();
    auto &context = valT->getContext();
    Type *floatT = llvm::Type::getFloatTy(context);
    Type *doubleT = llvm::Type::getDoubleTy(context);
    return valT == floatT || valT == doubleT;
}

Value *IRGenerator::visitASTNode(const Expr &expr) {
    auto &context = *m_context_ptr;
    auto &module = *m_module_ptr;
    auto &builder = *m_builder_ptr;
    auto &symTable = *m_symbolTable_ptr;

    return match<Value *>(
        expr,
        [&, this](ConstVar const &var) -> Value * {
            if (var.is<double>()) { // FIXME: double or float?
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
                    if (para.m_var_type != Void) { // skip Void param
                        funcArgsTypes.push_back(getLLVMType(para.m_var_type));
                    }
                }
                Type *retType = getLLVMType(func_node.m_return_type);
                FunctionType *func_type = FunctionType::get(retType, funcArgsTypes, false);
                p_func = Function::Create(func_type,
                                          Function::ExternalLinkage,
                                          func_node.m_name,
                                          module);
            }

            if (!p_func) { // somehow fail to generate func proto
                throw_err("Failed to generate function prototype for `{}`\n", func_node.m_name);
            }

            // Create new basic block
            BasicBlock *entryBlock = BasicBlock::Create(context, "func_entry", p_func);
            builder.SetInsertPoint(entryBlock);
            // TODO: ↓ func args should be in the same scope as func body
            scope_manager scope_mgr(symTable);

            for (auto &arg : p_func->args()) {
                // AllocaInst *alloc = CreateEntryBlockAlloca(p_func, arg.getName(), arg.getType());
                size_t argNo = arg.getArgNo();
                Type *argType = p_func->getFunctionType()->getParamType(argNo);
                StringRef argName = func_node.m_para_list[argNo]->as<Variable>().m_var_name;
                AllocaInst *alloc = builder.CreateAlloca(argType, nullptr, argName);
                symTable.insert(argName, alloc);
            }

            // codegen for func body
            visitASTNode(*func_node.m_body);

            // verification
            verifyFunction(*p_func);

            // TODO: mem2reg
            return p_func;
        },
        [&, this](CompoundExpr const &comp) -> Value * {
            scope_manager scope_mgr(symTable);
            for (const auto &p_expr : comp) {
                visitASTNode(*p_expr);
            }
            return nullptr;
        },
        // NameRef returns `LoadInst *`
        [&, this](NameRef const &var_name) -> Value * {
            auto *var_ref = symTable[var_name];
            if (!var_ref) {
                // check if it's global
                if (auto *global = module.getNamedGlobal(var_name)) {
                    // NOTE: global->getType() is a pointer type
                    return builder.CreateLoad(global->getValueType(), global, var_name.c_str());
                }
                // report error: ref to var that doesn't exist
                throw_err("Try to use undeclared var:{}\n", var_name);
            } else {
                return builder.CreateLoad(var_ref->getAllocatedType(), var_ref, var_name.c_str());
            }
        },
        [this](InitExpr const &var_decls) -> Value * {
            for (const auto &p_var_decl : var_decls) {
                visitASTNode(*p_var_decl);
            }
            return nullptr;
        },
        [&, this](Variable const &var) -> Value * {
            if (symTable.inCurrScope(var.m_var_name)) {
                throw_err("Duplicate declaration of `{}`\n", var.m_var_name);
            }

            Type *var_type = getLLVMType(var.m_var_type);
            AllocaInst *p_new_var = builder.CreateAlloca(var_type, nullptr, var.m_var_name);
            if (!p_new_var) [[unlikely]] {
                throw_err<std::runtime_error>("Interal compiler error: failed to allocate {}\n",
                                              var.m_var_name);
            }

            // add it to symbol table
            symTable.insert(var.m_var_name, p_new_var);

            // init var if needed
            if (var.m_var_init) {
                Value *init_val = visitASTNode(*var.m_var_init);
                builder.CreateStore(init_val, p_new_var);
            }

            return p_new_var;
        },
        [&, this](Return const &retExpr) -> Value * {
            Function *parent_func = builder.GetInsertBlock()->getParent();
            // if (!parent_func) throw_err("Return statement outside func?");
            ReturnInst *ret;
            if (retExpr.m_expr) {
                ret = builder.CreateRet(visitASTNode(*retExpr.m_expr));
            } else {
                ret = builder.CreateRetVoid();
            }
            // mark dead code after `return`
            builder.CreateUnreachable();
            return ret;
        },
        [&, this](FuncCall const &func_call) -> Value * { // FIXME
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
        [&, this](Binary const &exp) -> Value * {
            Value *lhs = visitASTNode(*exp.m_operand1);
            Value *rhs = visitASTNode(*exp.m_operand2);

            // if either is float, convert them to floats
            bool is_f_left = isFloat(lhs), is_f_right = isFloat(rhs);
            bool is_f = is_f_left || is_f_right;
            if (is_f && !is_f_left) builder.CreateSIToFP(lhs, getLLVMType(Float));
            if (is_f && !is_f_right) builder.CreateSIToFP(rhs, getLLVMType(Float));

            switch (exp.m_operator) {
            case Plus: return builder.CreateAdd(lhs, rhs, "add");
            case Minus: return builder.CreateSub(lhs, rhs, "sub");
            case Mul: return builder.CreateMul(lhs, rhs, "mul");
            case Div: {
                if (is_f) {
                    return builder.CreateFDiv(lhs, rhs, "fdiv");
                } else { // neither is float, so they're (signed) ints
                    return builder.CreateSDiv(lhs, rhs, "sidiv");
                }
            }
            case Equal: {
                if (is_f) {
                    return builder.CreateCmp(CmpInst::FCMP_OEQ, lhs, rhs, "feq");
                } else {
                    return builder.CreateCmp(CmpInst::ICMP_EQ, lhs, rhs, "ieq");
                }
            }
            default: llvm_unreachable("Unimplemented op?");
            }
        },
        [&, this](IfElse const &exp) -> Value * {
            /**
             *      br <cond>, then, else
             * then:
             *      <True Branch>
             *      br if_end
             * else:
             *      <False Branch>
             *      br if_end
             * if_end:
             *      ...
             */

            Value *cond_val = visitASTNode(*exp.m_condi);
            if (!cond_val) throw_err("Null condition expr for if-else statement!");
            Function *parent_func = builder.GetInsertBlock()->getParent();

            // create new basic block for branches
            auto *thenBB = BasicBlock::Create(context, "then", parent_func);
            auto *elseBB = BasicBlock::Create(context, "else");
            auto *mergeBB = BasicBlock::Create(context, "if_end");

            builder.CreateCondBr(cond_val, thenBB, elseBB);

            // then branch
            builder.SetInsertPoint(thenBB);
            visitASTNode(*exp.m_if);

            // recursive if-else can change blocks where we're emitting code
            thenBB = builder.GetInsertBlock();
            builder.CreateBr(mergeBB);

            // else branch
            parent_func->getBasicBlockList().push_back(elseBB);
            builder.SetInsertPoint(elseBB);
            if (exp.m_else) visitASTNode(*exp.m_else);
            builder.CreateBr(mergeBB);
            elseBB = builder.GetInsertBlock();

            // exit if
            parent_func->getBasicBlockList().push_back(mergeBB);
            builder.SetInsertPoint(mergeBB);

            return nullptr;
        },
        [&, this](WhileLoop const &while_loop) -> Value * {
            /**
             *      br <cond>, loop, loop_end
             * loop:
             *      <loop body...>
             *      br <cond>, loop, loop_end
             * loop_end:
             *      ...
             */

            Value *cond_val = visitASTNode(*while_loop.m_condi);
            if (!cond_val) throw_err("Null condition expr for if-else statement!");
            Function *parent_func = builder.GetInsertBlock()->getParent();

            // create while loop blocks
            auto *loopBB = BasicBlock::Create(context, "loop");
            auto *loopEndBB = BasicBlock::Create(context, "loop_end");

            builder.CreateCondBr(cond_val, loopBB, loopEndBB);

            // loop body
            parent_func->getBasicBlockList().push_back(loopBB);
            builder.SetInsertPoint(loopBB);
            visitASTNode(*while_loop.m_loop_body);

            // re-check condition
            cond_val = visitASTNode(*while_loop.m_condi);
            if (!cond_val) throw_err("Null condition expr for if-else statement!");
            // reset BB for recursive loops
            loopBB = builder.GetInsertBlock();
            builder.CreateCondBr(cond_val, loopBB, loopEndBB);

            // exit loop
            parent_func->getBasicBlockList().push_back(loopEndBB);
            builder.SetInsertPoint(loopEndBB);

            return nullptr;
        },
        [](auto const &) -> Value * { llvm_unreachable("Invalid AST Node!"); } //
    );
}
