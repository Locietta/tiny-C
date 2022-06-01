#include "DeadBlockRemove.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/IR/CFG.h"

using namespace llvm;

bool DeadBlockRemove::runOnFunction(llvm::Function &F) {
    df_iterator_default_set<BasicBlock *> visitedSet;
    df_iterator_default_set<BasicBlock *> unreachableSet;

    // 从EntryBlock开始深度优先遍历整个函数内可以访问的BasicBlock
    // 将已被访问过的BaseBlock存放在visitedSet中

    for (auto i = df_ext_begin(&F.getEntryBlock(), visitedSet),
              e = df_ext_end(&F.getEntryBlock(), visitedSet);
         i != e;
         ++i)
        ;

    auto name = F.getName();
    auto count = F.size();

    // 遍历函数内所有BaseBlock，将不在vistitedSet中的BaseBlock添加到unreachableSet中
    for (BasicBlock &BB : F) {
        if (visitedSet.find(&BB) == visitedSet.end()) {
            unreachableSet.insert(&BB);
        }
    }

    // 标记目标函数是否会被修改
    bool changed = !unreachableSet.empty();

    // 遍历unreachableSet，通知其successor移除多余的phi node
    for (BasicBlock *BB : unreachableSet) {
        for (auto i = succ_begin(BB); i != succ_end(BB); i++) {
            i->removePredecessor(BB);
        }
        BB->eraseFromParent();
    }

    return changed;
}