#pragma once

#include "llvm/IR/PassManager.h"
#include "llvm/IR/InstrTypes.h"

namespace llvm {

class VerifyInlineDecisionsPass : public PassInfoMixin<VerifyInlineDecisionsPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
  bool hasCallBaseId(const CallBase &CB);
  size_t getCallBaseId(const CallBase &CB);
};

} // namespace llvm
