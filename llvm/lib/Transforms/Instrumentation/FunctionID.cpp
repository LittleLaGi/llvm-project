#include "llvm/Transforms/Instrumentation/FunctionID.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/PassManager.h"

using namespace llvm;

PreservedAnalyses FunctionIDPass::run(Module &M, ModuleAnalysisManager &AM) {
  size_t CallBaseID = 0;
  for (auto &F : M) {
    for (auto &I : instructions(F)) {
      auto *CB = dyn_cast<CallBase>(&I);
      if (not CB)
        continue;
      auto &Ctx = M.getContext();
      auto *N = MDNode::get(Ctx, ConstantAsMetadata::get(ConstantInt::get(
                                     Ctx, APInt{64, CallBaseID++, false})));
      CB->setMetadata("callbase.id", N);
    }
  }
  return PreservedAnalyses::all();
}
