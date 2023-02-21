#pragma once

#include "llvm/IR/PassManager.h"

namespace llvm {

class LiveFunctionExploringPass : public PassInfoMixin<LiveFunctionExploringPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm
