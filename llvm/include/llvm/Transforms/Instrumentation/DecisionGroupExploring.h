#pragma once

#include "llvm/IR/PassManager.h"
#include "llvm/IR/InstrTypes.h"

namespace llvm {

class DecisionGroupExploringPass : public PassInfoMixin<DecisionGroupExploringPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
  bool hasCallBaseId(const CallBase &CB);
  size_t getCallBaseId(const CallBase &CB);
};

} // namespace llvm
