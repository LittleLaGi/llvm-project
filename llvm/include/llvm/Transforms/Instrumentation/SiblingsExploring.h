#pragma once

#include "llvm/IR/PassManager.h"
#include "llvm/IR/InstrTypes.h"

namespace llvm {

class SiblingsExploringPass : public PassInfoMixin<SiblingsExploringPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
  bool hasCallBaseId(const CallBase &CB);
  size_t getCallBaseId(const CallBase &CB);
};

} // namespace llvm
