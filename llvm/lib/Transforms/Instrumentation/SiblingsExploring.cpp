#include "llvm/Transforms/Instrumentation/SiblingsExploring.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/CFG.h"
#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace llvm;

PreservedAnalyses SiblingsExploringPass::run(Module &M, ModuleAnalysisManager &AM) {
  const char *SiblingsFileName = nullptr;
  SiblingsFileName = std::getenv("RECORD_SIBLINGS");
  if (!SiblingsFileName)
    return PreservedAnalyses::all();
  
  std::string SiblingsString;
  raw_string_ostream SiblingsStream{SiblingsString};
  if (SiblingsFileName) {
    for (auto &F : M) {
      for (auto &BB : F) {
        for (auto &I : BB) {
          auto *CB = dyn_cast<CallBase>(&I);
          if (!CB)
            continue;
          if (hasCallBaseId(*CB)) {
            size_t ID = getCallBaseId(*CB);
            SiblingsStream  << ID << ' ';
            auto exploreSiblings = [&](const BasicBlock *BB) {
              for (auto &I : *BB) {
                if (auto *CB = dyn_cast<CallBase>(&I)) {
                  if (hasCallBaseId(*CB)) {
                    size_t ID = getCallBaseId(*CB);
                    SiblingsStream  << ID << ' ';
                  }
                }
              }
            };
            const BasicBlock *CurrBB = CB->getParent();
            exploreSiblings(CurrBB);
            for (const BasicBlock *Succ : successors(CurrBB))
              exploreSiblings(Succ);
            for (const BasicBlock *Pre : predecessors(CurrBB))
              exploreSiblings(Pre);
            SiblingsStream << '\n';
          }
        }
      }
    }
  
    std::error_code EC;
    raw_fd_ostream FStream{SiblingsFileName, EC};
    FStream << SiblingsStream.str();
  }
  
  return PreservedAnalyses::all();
}

bool SiblingsExploringPass::hasCallBaseId(const CallBase &CB) {
  auto *N = CB.getMetadata("callbase.id");
  if (not N)
    return false;

  if (not dyn_cast<MDNode>(N))
    return false;

  if (not dyn_cast<MDNode>(N)->getOperand(0))
    return false;

  auto *CM = dyn_cast<ConstantAsMetadata>(dyn_cast<MDNode>(N)->getOperand(0));
  if (not CM)
    return false;
  if (not CM->getValue())
    return false;

  return true;
}

size_t SiblingsExploringPass::getCallBaseId(const CallBase &CB) {
  auto *N = CB.getMetadata("callbase.id");
  assert(N && "CallBase does not carry metadata.\n");
  Constant *C = dyn_cast<ConstantAsMetadata>(dyn_cast<MDNode>(N)->getOperand(0))
                    ->getValue();
  return cast<ConstantInt>(C)->getZExtValue();
}