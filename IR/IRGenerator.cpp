#include "IRGenerator.h"
#include "AST.hpp"
#include "ASTSimplify.h"
#include "DeadBlockRemove.h"
#include "utility.hpp"

using namespace llvm;

struct scope_manager { // RAII scope manager
    SymbolTable &sym;
    scope_manager(SymbolTable &sym) : sym{sym} { sym.push_scope(); }
    ~scope_manager() { sym.pop_scope(); }
};

struct loop_BB_manager { // RAII loop basic block manager
    inline static BasicBlock *continueBB = nullptr, *breakBB = nullptr;
    BasicBlock *tmp_head, *tmp_tail;
    loop_BB_manager(BasicBlock *conti, BasicBlock *brk) : tmp_head(conti), tmp_tail(brk) {
        std::swap(continueBB, tmp_head);
        std::swap(breakBB, tmp_tail);
    }
    ~loop_BB_manager() {
        std::swap(continueBB, tmp_head);
        std::swap(breakBB, tmp_tail);
    }
};

// ------------ Implementation of `SymbolTable` -------------------

void SymbolTable::push_scope() {
    locals.push_back(StringMap<AllocaInst *>{});
}

void SymbolTable::pop_scope() {
    // NOTE: globals should never be poped
    assert(!locals.empty() && "try to pop scope from empty symbol table?");
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

static PassBuilder::OptimizationLevel int2OptLevel(int opt_level) {
    switch (opt_level) {
    case 0: return PassBuilder::OptimizationLevel::O0;
    case 1: return PassBuilder::OptimizationLevel::O1;
    case 2: return PassBuilder::OptimizationLevel::O2;
    default: return PassBuilder::OptimizationLevel::O3;
    }
}

// ------------ Implementation of `IRGenerator` -------------------

IRGenerator::IRGenerator(std::vector<std::shared_ptr<Expr>> const &trees, int opt_level)
    : m_simplifiedAST(trees), m_context_ptr(std::make_unique<llvm::LLVMContext>()),
      m_builder_ptr(std::make_unique<llvm::IRBuilder<>>(*m_context_ptr)),
      m_module_ptr(std::make_unique<llvm::Module>("tinycc JIT", *m_context_ptr)),
      m_symbolTable_ptr(std::make_unique<SymbolTable>()),
      m_analysis(std::make_unique<IRAnalysis>()) {

    /// trivial heuristic transform on AST
    simplifyAST(m_simplifiedAST);

    /// LLVM Pass (New PM)
    PassBuilder PB;
    PB.registerModuleAnalyses(m_analysis->MAM);
    PB.registerCGSCCAnalyses(m_analysis->CGAM);
    PB.registerFunctionAnalyses(m_analysis->FAM);
    PB.registerLoopAnalyses(m_analysis->LAM);
    PB.crossRegisterProxies(m_analysis->LAM, m_analysis->FAM, m_analysis->CGAM, m_analysis->MAM);

    if (opt_level) {
        m_optimizer = std::make_unique<ModulePassManager>(
            PB.buildPerModuleDefaultPipeline(int2OptLevel(opt_level)));
    } else { // disable opt: O0 isn't allowed by pipeline builder
        m_optimizer = std::make_unique<ModulePassManager>();
    }
}

void IRGenerator::emitOBJ(fs::path const &asm_path) {
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    // construct target machine
    auto targetTriple = llvm::sys::getDefaultTargetTriple();

    std::string error;
    auto target = TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        throw_err<std::runtime_error>("Failed to initialize target: {}", error);
    }

    auto CPU = "x86-64-v3";
    auto features = "";
    TargetOptions opt;
    auto RM = Optional<Reloc::Model>();
    auto targetMachine = target->createTargetMachine(targetTriple, CPU, features, opt, RM);

    // set module target
    m_module_ptr->setTargetTriple(targetTriple);
    m_module_ptr->setDataLayout(targetMachine->createDataLayout());

    // open file to emit
    std::error_code ec;
    raw_fd_ostream out(asm_path.native(), ec);

    if (ec) {
        fmt::print(stderr,
                   "Error Category: {}, Code: {}, Message: {}\n",
                   ec.category().name(),
                   ec.value(),
                   ec.message());
    }

    legacy::PassManager pass;
    auto fileType = CGFT_ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, out, nullptr, fileType)) {
        throw_err<std::runtime_error>("target machine can't emit a file of this type");
    }

    pass.run(*m_module_ptr);
    out.flush();

    dbg_print("[DEBUG] obj file is written to {}\n", asm_path.native());
}

llvm::Type *IRGenerator::getLLVMType(enum DataTypes type) {
    switch (type) {
    case Int: return llvm::Type::getInt32Ty(*m_context_ptr); break;
    case Float: return llvm::Type::getFloatTy(*m_context_ptr); break;
    case Char: return llvm::Type::getInt8Ty(*m_context_ptr); break;
    default: break;
    }
    return llvm::Type::getVoidTy(*m_context_ptr);
}

void IRGenerator::dumpIR(fs::path const &asm_path) const {
    std::error_code ec;
    raw_fd_ostream out(asm_path.native(), ec);
    if (!ec) {
        out << *m_module_ptr;
        dbg_print("[DEBUG] LLVM IR is written to {}\n", asm_path.native());
        return;
    }

    // error when opening file
    fmt::print(stderr,
               "Error Category: {}, Code: {}, Message: {}\n",
               ec.category().name(),
               ec.value(),
               ec.message());
}

std::string IRGenerator::dumpIRString() const {
    std::string buf;
    raw_string_ostream out(buf);
    out << *m_module_ptr;
    return out.str();
}

void IRGenerator::emitBlock(BasicBlock *BB, bool IsFinished) {
    auto &Builder = *m_builder_ptr;

    BasicBlock *CurBB = Builder.GetInsertBlock();

    // Fall out of the current block (if necessary).
    if (!CurBB || CurBB->getTerminator()) {
        // If there is no insert point or the previous block is already
        // terminated, don't touch it.
    } else {
        // Otherwise, create a fall-through branch.
        Builder.CreateBr(BB);
    }

    Builder.ClearInsertionPoint();

    if (IsFinished && BB->use_empty()) {
        delete BB;
        return;
    }

    Function *CurFn = CurBB->getParent();

    // Place the block after the current block, if possible, or else at
    // the end of the function.
    if (CurBB && CurBB->getParent())
        CurFn->getBasicBlockList().insertAfter(CurBB->getIterator(), BB);
    else
        CurFn->getBasicBlockList().push_back(BB);
    Builder.SetInsertPoint(BB);
}

void IRGenerator::codegen() {
    for (const auto &tree : m_simplifiedAST) {
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

    m_optimizer->run(*m_module_ptr, m_analysis->MAM);
}

static bool isFloat(Value *val) {
    Type *valT = val->getType();
    auto &context = valT->getContext();
    Type *floatT = llvm::Type::getFloatTy(context);
    Type *doubleT = llvm::Type::getDoubleTy(context);
    return valT == floatT || valT == doubleT;
}

Value *IRGenerator::boolCast(Value *val) { // NOTE: ad-hoc bool cast
    LLVMContext &context = val->getContext();
    auto &builder = *m_builder_ptr;
    Type *type = val->getType();
    if (type == getLLVMType(Int) || type == getLLVMType(Char)) {
        return builder.CreateICmpNE(val, ConstantInt::get(type, 0), "bool_cast");
    } else if (type == getLLVMType(Float)) {
        // QUESTION: comparison relaxation for float?
        return builder.CreateFCmpONE(val, ConstantFP::get(type, 0), "bool_cast");
    }
    return val;
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
            } else if (var.is<bool>()) {
                return ConstantInt::get(context, APInt(1, var.as<bool>()));
            }
            llvm_unreachable("Unsupported ConstVar type!");
        },
        [&, this](FuncProto const &func_proto) -> Value * {
            Function *p_func = module.getFunction(func_proto.m_name);

            if (!p_func) { // generate func proto if not exist
                SmallVector<Type *> funcArgsTypes;
                for (const auto &p_para : func_proto.m_para_list) {
                    const auto &para = p_para->as<Variable>();
                    if (para.m_var_type != Void) { // skip Void param
                        funcArgsTypes.push_back(getLLVMType(para.m_var_type));
                    }
                }
                Type *retType = getLLVMType(func_proto.m_return_type);
                FunctionType *func_type = FunctionType::get(retType, funcArgsTypes, false);
                p_func = Function::Create(func_type,
                                          Function::ExternalLinkage,
                                          func_proto.m_name,
                                          module);

                for (size_t i = 0; auto &arg : p_func->args()) {
                    arg.setName(func_proto.m_para_list[i++]->as<Variable>().m_var_name);
                }
            }

            if (!p_func) { // somehow fail to generate func proto
                throw_err("Failed to generate function prototype for `{}`\n", func_proto.m_name);
            }

            return p_func;
        },
        [&, this](FuncDef const &func_node) -> Value * {
            auto *p_func = cast<Function>(visitASTNode(*func_node.m_proto));

            // Create new basic block
            BasicBlock *entryBlock = BasicBlock::Create(context, "func_entry", p_func);
            builder.SetInsertPoint(entryBlock);
            scope_manager scope_mgr(symTable);

            for (auto &arg : p_func->args()) {
                Type *argType = arg.getType();
                StringRef argName = arg.getName();
                AllocaInst *alloc = builder.CreateAlloca(argType, nullptr, argName);
                symTable.insert(argName, alloc);
                builder.CreateStore(&arg, alloc);
            }

            // codegen for func body
            visitASTNode(*func_node.m_body);

            // verification
            verifyFunction(*p_func);

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
            // builder.CreateUnreachable();
            return ret;
        },
        [&, this](FuncCall const &func_call) -> Value * {
            // Look up the name in the global module table.
            Function *CalleeF = module.getFunction(func_call.m_func_name);
            if (!CalleeF) {
                throw_err("Unknown function `{}` referenced", func_call.m_func_name);
            }

            // If argument mismatch error.
            if (CalleeF->arg_size() != func_call.m_para_list.size()) {
                throw_err("Function `{}` expects {} arguments, but provided {}",
                          func_call.m_func_name,
                          CalleeF->arg_size(),
                          func_call.m_para_list.size());
            }

            std::vector<Value *> ArgsV;
            for (const auto &para : func_call.m_para_list) {
                auto argVal = visitASTNode(*para);
                if (!argVal) {
                    throw_err("Null argument when calling function {}", func_call.m_func_name);
                }
                ArgsV.push_back(argVal);
            }

            return builder.CreateCall(CalleeF, ArgsV, "calltmp");
        },
        [&, this](Binary const &exp) -> Value * {
            // refactor? builder.CreateBinOp(llvm::BinaryOperator::Add);
            Value *lhs = visitASTNode(*exp.m_operand1);
            Value *rhs = visitASTNode(*exp.m_operand2);
            if (!lhs) throw_err("null left oprand for {} operation?", op_to_str[exp.m_operator]);
            if (!rhs) throw_err("null right oprand for {} operation?", op_to_str[exp.m_operator]);

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
            case Mod: {
                if (is_f) {
                    return builder.CreateFRem(lhs, rhs, "frem");
                } else {
                    return builder.CreateSRem(lhs, rhs, "srem");
                }
            }
            case Equal: {
                if (is_f) {
                    return builder.CreateCmp(CmpInst::FCMP_OEQ, lhs, rhs, "feq");
                } else {
                    return builder.CreateCmp(CmpInst::ICMP_EQ, lhs, rhs, "ieq");
                }
            }
            case NotEqual: {
                if (is_f) {
                    return builder.CreateCmp(CmpInst::FCMP_ONE, lhs, rhs, "fne");
                } else {
                    return builder.CreateCmp(CmpInst::ICMP_NE, lhs, rhs, "ine");
                }
            }
            case Greater: {
                if (is_f) {
                    return builder.CreateCmp(CmpInst::FCMP_OGT, lhs, rhs, "fgt");
                } else {
                    return builder.CreateCmp(CmpInst::ICMP_SGT, lhs, rhs, "sigt");
                }
            }
            case GreaterEqual: {
                if (is_f) {
                    return builder.CreateCmp(CmpInst::FCMP_OGE, lhs, rhs, "fge");
                } else {
                    return builder.CreateCmp(CmpInst::ICMP_SGE, lhs, rhs, "sige");
                }
            }
            case Less: {
                if (is_f) {
                    return builder.CreateCmp(CmpInst::FCMP_OLT, lhs, rhs, "flt");
                } else {
                    return builder.CreateCmp(CmpInst::ICMP_SLT, lhs, rhs, "silt");
                }
            }
            case LessEqual: {
                if (is_f) {
                    return builder.CreateCmp(CmpInst::FCMP_OLE, lhs, rhs, "fle");
                } else {
                    return builder.CreateCmp(CmpInst::ICMP_SLE, lhs, rhs, "sile");
                }
            }
            case OrOr: return builder.CreateOr(lhs, rhs, "or");
            case AndAnd: return builder.CreateAnd(lhs, rhs, "and");
            case Assign: {
                // FIXME: ad hoc, unable to handle *ptr
                if (!exp.m_operand1->is<NameRef>()) {
                    throw_err("LHS of Assign op is expected to be lvalue!");
                }

                // extract var ref
                StringRef var_name = exp.m_operand1->as<NameRef>();
                Value *var_ref = symTable[var_name];
                if (!var_ref) var_ref = module.getNamedGlobal(var_name);
                if (!var_ref) throw_err("Try to use undeclared var:{}\n", var_name);

                return builder.CreateStore(rhs, var_ref);
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

            cond_val = boolCast(cond_val);

            // create new basic block for branches
            auto *thenBB = BasicBlock::Create(context, "then");
            auto *elseBB = BasicBlock::Create(context, "else");
            auto *mergeBB = BasicBlock::Create(context, "if_end");

            builder.CreateCondBr(cond_val, thenBB, elseBB);

            // then branch
            emitBlock(thenBB);
            visitASTNode(*exp.m_if);

            // else branch
            emitBlock(elseBB);
            if (exp.m_else) visitASTNode(*exp.m_else);

            // exit if, don't emit if unreachable
            emitBlock(mergeBB, true);

            return nullptr;
        },
        [&, this](Break const &) -> Value * { return builder.CreateBr(loop_BB_manager::breakBB); },
        [&, this](Continue const &) -> Value * {
            return builder.CreateBr(loop_BB_manager::continueBB);
        },
        [&, this](WhileLoop const &while_loop) -> Value * {
            /**
             *      ...
             *      <cond> = cmp ...
             *      br <cond>, loop, loop_end
             * loop:
             *      [loop body...]
             *      br latch
             * latch:
             *      <cond> = cmp ...
             *      br <cond>, loop, loop_end
             * loop_end:
             *      ...
             */

            Value *cond_val = visitASTNode(*while_loop.m_condi);
            if (!cond_val) throw_err("Null condition expr for loop statement!");
            cond_val = boolCast(cond_val);

            // create while loop blocks
            auto *loopBB = BasicBlock::Create(context, "loop");
            auto *latchBB = BasicBlock::Create(context, "latch");
            auto *loopEndBB = BasicBlock::Create(context, "loop_end");
            loop_BB_manager BB_mgr(latchBB, loopEndBB);

            builder.CreateCondBr(cond_val, loopBB, loopEndBB);

            // loop header (body)
            emitBlock(loopBB);
            visitASTNode(*while_loop.m_loop_body);

            // latch
            emitBlock(latchBB);
            cond_val = visitASTNode(*while_loop.m_condi);
            if (!cond_val) throw_err("Null condition expr for loop statement!");
            cond_val = boolCast(cond_val);
            builder.CreateCondBr(cond_val, loopBB, loopEndBB);

            // exit loop, don't emit if unreachable
            emitBlock(loopEndBB, true);

            return nullptr;
        },
        [&, this](ForLoop const &for_loop) -> Value * {
            /**
             *      <init>
             *      <cond> = cmp ...
             *      br <cond>, loop, loop_end
             * loop:
             *      [loop body...]
             *      br latch
             * latch:
             *      [iter]
             *      <cond> = cmp ...
             *      br <cond>, loop, loop_end
             * loop_end:
             *      ...
             */

            // codegen for init expr
            scope_manager scope_mgr(symTable); // the var defined here shouldn't leak out of loop
            if (for_loop.m_init) visitASTNode(*for_loop.m_init);

            // create for loop blocks
            auto *loopBB = BasicBlock::Create(context, "loop");
            auto *latchBB = BasicBlock::Create(context, "latch");
            auto *loopEndBB = BasicBlock::Create(context, "loop_end");
            loop_BB_manager BB_mgr(latchBB, loopEndBB);

            // loop entry
            if (for_loop.m_condi) {
                Value *cond_val = visitASTNode(*for_loop.m_condi);
                if (!cond_val) throw_err("Null condition expr for loop statement!");
                cond_val = boolCast(cond_val);
                builder.CreateCondBr(cond_val, loopBB, loopEndBB);
            } else {
                builder.CreateBr(loopBB);
            }

            // loop header (body)
            emitBlock(loopBB);
            visitASTNode(*for_loop.m_loop_body);

            // latch
            emitBlock(latchBB);
            if (for_loop.m_iter) {
                visitASTNode(*for_loop.m_iter);
            }
            if (for_loop.m_condi) {
                Value *cond_val = visitASTNode(*for_loop.m_condi);
                if (!cond_val) throw_err("Null condition expr for loop statement!");
                cond_val = boolCast(cond_val);
                builder.CreateCondBr(cond_val, loopBB, loopEndBB);
            } else {
                builder.CreateBr(loopBB);
            }

            // exit loop, don't emit if unreachable
            emitBlock(loopEndBB, true);

            return nullptr;
        },
        [](Null const &) -> Value * { return nullptr; },
        [](auto const &) -> Value * { llvm_unreachable("Invalid AST Node!"); } //
    );
}
