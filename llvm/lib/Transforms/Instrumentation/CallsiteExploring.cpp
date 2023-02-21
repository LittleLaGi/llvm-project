#include "llvm/Transforms/Instrumentation/CallsiteExploring.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Metadata.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <map>
#include <utility>

using namespace llvm;

PreservedAnalyses CallsiteExploringPass::run(Module &M, ModuleAnalysisManager &AM) {
  const char *DecisionsFileName = std::getenv("FIXED_INLINE_RECORD");
  if (!DecisionsFileName || !std::getenv("UPDATE_DECISIONS"))
    return PreservedAnalyses::all();

  std::ifstream IStream{DecisionsFileName};
  std::map<size_t, std::string> ID2Decision;
  std::map<size_t, std::pair<std::string, std::string>> ID2CallerCallee;
  for (std::string Line; std::getline(IStream, Line);) {
    SmallVector<StringRef, 4> LineParts;
    StringRef{Line}.split(LineParts, ',');
    assert(LineParts.size() == 4);
    std::istringstream iss(LineParts[2].str());
    size_t ID;
    iss >> ID;
    ID2Decision[ID] = std::string(LineParts[3].str());
  }

  std::string DecisionsString;
  raw_string_ostream DecisionsStream{DecisionsString};

  for (auto &F : M) {
    // if ((F.hasLocalLinkage() || F.hasLinkOnceODRLinkage() || F.hasWeakODRLinkage())
    //     && F.use_empty())
    //   continue;
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *CB = dyn_cast<CallBase>(&I);
        if (!CB)
          continue;
        Function *CalledFunc = CB->getCalledFunction();
        if (CalledFunc && !CalledFunc->isDeclaration()) {
          std::string InlineDecision = "undecided";
          if (hasCallBaseId(*CB)) {
            auto ID = getCallBaseId(*CB);
            // auto &Ctx = M.getContext();
            // auto *N = MDNode::get(Ctx, ConstantAsMetadata::get(ConstantInt::get(
            //                                 Ctx, APInt{64, ++CallBaseID, false})));
            // CB->setMetadata("callbase.id", N);
            if (!ID2Decision.count(ID))
              ID2Decision[ID] = "undecided";
            ID2CallerCallee[ID] = std::make_pair(std::string(F.getName()), std::string(CalledFunc->getName()));
            // DecisionsStream  << F.getName() << ',' << CalledFunc->getName() << ','
            //                   << ID << ',' << InlineDecision << '\n';
          }
          // else {
          //   auto &Ctx = M.getContext();
          //   auto *N = MDNode::get(Ctx, MDString::get(Ctx, NewID));
          //   CB->setMetadata("callbase.id", N);
          // }
        }
      }
    }
  }
  std::error_code EC;
  raw_fd_ostream FStream{DecisionsFileName, EC};
  //FStream << DecisionsStream.str();
  for (auto &pair : ID2CallerCallee) {
    auto &ID = pair.first;
    auto &Caller = pair.second.first;
    auto &Callee = pair.second.second;
    FStream  << Caller << ',' << Callee << ',' << ID << ',' << ID2Decision[ID] << '\n';
  }

  return PreservedAnalyses::all();
}

// bool CallsiteExploringPass::hasCallBaseId(const CallBase &CB) {
//   auto *N = CB.getMetadata("callbase.id");
//   if (not N)
//     return false;

//   if (not dyn_cast<MDNode>(N))
//     return false;

//   if (not dyn_cast<MDNode>(N)->getOperand(0))
//     return false;

//   return true;
// }

// std::string CallsiteExploringPass::getCallBaseId(const CallBase &CB) {
//   auto *N = CB.getMetadata("callbase.id");
//   assert(N && "CallBase does not carry metadata.\n");
//   return std::string(cast<MDString>(N->getOperand(0))->getString());
// }

bool CallsiteExploringPass::hasCallBaseId(const CallBase &CB) {
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

size_t CallsiteExploringPass::getCallBaseId(const CallBase &CB) {
  auto *N = CB.getMetadata("callbase.id");
  assert(N && "CallBase does not carry metadata.\n");
  Constant *C = dyn_cast<ConstantAsMetadata>(dyn_cast<MDNode>(N)->getOperand(0))
                    ->getValue();
  return cast<ConstantInt>(C)->getZExtValue();
}