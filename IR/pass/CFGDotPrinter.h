#pragma once

namespace llvm {

class CFGDotPrinterPass : public PassInfoMixin<CFGDotPrinterPass> {
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm