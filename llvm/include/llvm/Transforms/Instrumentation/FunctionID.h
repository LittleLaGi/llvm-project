#pragma once

#include "llvm/IR/PassManager.h"

namespace llvm {

class FunctionIDPass : public PassInfoMixin<FunctionIDPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm
