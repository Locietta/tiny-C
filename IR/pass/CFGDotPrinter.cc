#include "CFGDotPrinter.h"
#include "OptHandler.h"
#include "utility.hpp"

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Support/FileSystem.h"

using namespace llvm;

extern OptHandler cli_inputs;

static void writeCFGToDotFile(Function &F, BlockFrequencyInfo *BFI, BranchProbabilityInfo *BPI,
                              uint64_t MaxFreq, bool CFGOnly = false) {
    std::string Basename = fmt::format("{}/cfg_{}", cli_inputs.pic_outdir, F.getName());
    dbg_print("[DEBUG] write CFG for '{}'\n", F.getName());

    std::error_code EC;
    raw_fd_ostream File(Basename + ".dot", EC, sys::fs::OF_Text);

    DOTFuncInfo CFGInfo(&F, BFI, BPI, MaxFreq);
    CFGInfo.setHeatColors(true);
    CFGInfo.setEdgeWeights(false);
    CFGInfo.setRawEdgeWeights(false);

    if (!EC)
        WriteGraph(File, &CFGInfo, CFGOnly).flush();
    else
        errs() << "  error opening file for writing!\n";

    auto temp = fmt::format("dot {}.dot -Tpng:cairo -o {}.png", Basename, Basename);
    system(temp.c_str());
    std::filesystem::remove(Basename + ".dot");
}

PreservedAnalyses CFGDotPrinterPass::run(Function &F, FunctionAnalysisManager &AM) {
    auto *BFI = &AM.getResult<BlockFrequencyAnalysis>(F);
    auto *BPI = &AM.getResult<BranchProbabilityAnalysis>(F);
    writeCFGToDotFile(F, BFI, BPI, getMaxFreq(F, BFI));
    return PreservedAnalyses::all();
}
