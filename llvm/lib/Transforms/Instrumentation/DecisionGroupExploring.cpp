#include "llvm/Transforms/Instrumentation/DecisionGroupExploring.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/CFG.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>

using namespace llvm;

PreservedAnalyses DecisionGroupExploringPass::run(Module &M, ModuleAnalysisManager &AM) {
  const char *DecisionGroupFileName = std::getenv("RECORD_DECISION_GROUP");
  if (!DecisionGroupFileName)
    return PreservedAnalyses::all();
  
  std::string DecisionGroupString;
  raw_string_ostream DecisionGroupStream{DecisionGroupString};
  for (auto &F : M) {
    for (auto &BB : F) {
      std::map<std::string, std::vector<size_t>> CBs;
      for (auto &I : BB) {
        auto *CB = dyn_cast<CallBase>(&I);
        if (!CB)
          continue;
        if (hasCallBaseId(*CB)) {      
          Function *Callee = CB->getCalledFunction();
          if (Callee && !Callee->isDeclaration()) {
            size_t ID = getCallBaseId(*CB);
            if (F.getName() == Callee->getName())
              continue;
            std::string CalleeName = std::string(Callee->getName());
            if (!CBs.count(CalleeName))
              CBs[CalleeName] = std::vector<size_t>();
            CBs[CalleeName].push_back(ID);
          }
        }
      }
      for (auto &pair : CBs) {
        auto &vec = pair.second;
        if (vec.size() <= 1)
          continue;
        for (auto &ID : vec)
          DecisionGroupStream << ID << ' ';
        DecisionGroupStream << '\n';
      }
    }
  }

  std::error_code EC;
  raw_fd_ostream FStream{DecisionGroupFileName, EC};
  FStream << DecisionGroupStream.str();
  
  return PreservedAnalyses::all();
}

bool DecisionGroupExploringPass::hasCallBaseId(const CallBase &CB) {
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

size_t DecisionGroupExploringPass::getCallBaseId(const CallBase &CB) {
  auto *N = CB.getMetadata("callbase.id");
  assert(N && "CallBase does not carry metadata.\n");
  Constant *C = dyn_cast<ConstantAsMetadata>(dyn_cast<MDNode>(N)->getOperand(0))
                    ->getValue();
  return cast<ConstantInt>(C)->getZExtValue();
}