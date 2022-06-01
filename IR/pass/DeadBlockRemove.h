#pragma once

struct DeadBlockRemove : public llvm::FunctionPass {
    static char ID;
    DeadBlockRemove() : FunctionPass(ID) {}

    bool runOnFunction(llvm::Function &F) override;
};

// LLVM会利用pass的地址来为这个id赋值，所以初始值并不重要
inline char DeadBlockRemove::ID = 0;

// 注册pass，这个pass可能会改变CFG，所以将第三个参数设为true
inline static llvm::RegisterPass<DeadBlockRemove> X("deadblock", "Dead blocks elimination pass",
                                                    true, false);