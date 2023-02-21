#include "llvm/Transforms/Instrumentation/CallsiteIDRewriting.h"
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
#include <set>

using namespace llvm;

PreservedAnalyses CallsiteIDRewritingPass::run(Module &M, ModuleAnalysisManager &AM) {
  const char *DecisionsFileName = std::getenv("FIXED_INLINE_RECORD");
  const char *MaxCallsiteIdStr = std::getenv("MAX_CALLSITE_ID");
  
  if (!DecisionsFileName || !MaxCallsiteIdStr)
    return PreservedAnalyses::all();

  std::istringstream MaxCallsiteIdStream(MaxCallsiteIdStr);
  size_t MaxCallsiteId;
  MaxCallsiteIdStream >> MaxCallsiteId;

  std::ifstream IStream{DecisionsFileName};
  std::map<size_t, std::string> ID2Decision;
  std::map<size_t, std::string> ID2Caller;
  for (std::string Line; std::getline(IStream, Line);) {
    SmallVector<StringRef, 4> LineParts;
    StringRef{Line}.split(LineParts, ',');
    assert(LineParts.size() == 4);
    //std::string ID = std::string(LineParts[2].str());
    std::istringstream iss(LineParts[2].str());
    size_t ID;
    iss >> ID;
    ID2Decision[ID] = std::string(LineParts[3].str());
    ID2Caller[ID] = std::string(LineParts[0].str());
  }

  std::set<size_t> VisitedIds;
  for (auto &F : M) {
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
            // if (ID2Decision.count(ID))
            //   InlineDecision = ID2Decision[ID];
            // if (std::string(F.getName()) != ID2Caller[ID]) {
            //     std::string NewID = std::to_string(++MaxCallsiteId);
            //     auto &Ctx = M.getContext();
            //     auto *N = MDNode::get(Ctx, MDString::get(Ctx, NewID));
            //     CB->setMetadata("callbase.id", N);
            // }

            // VisitedIds.count(ID)
            // ID2Caller[ID] != std::string(F.getName())
            if (!VisitedIds.count(ID)) {
              std::string NewID = std::to_string(++MaxCallsiteId);
              auto &Ctx = M.getContext();
              auto *N = MDNode::get(Ctx, MDString::get(Ctx, NewID));
              CB->setMetadata("callbase.id", N);
            }
            else
              VisitedIds.insert(ID);
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
  
  return PreservedAnalyses::all();
}

// bool CallsiteIDRewritingPass::hasCallBaseId(const CallBase &CB) {
//   auto *N = CB.getMetadata("callbase.id");
//   if (not N)
//     return false;

//   if (not dyn_cast<MDNode>(N))
//     return false;

//   if (not dyn_cast<MDNode>(N)->getOperand(0))
//     return false;

//   return true;
// }

// std::string CallsiteIDRewritingPass::getCallBaseId(const CallBase &CB) {
//   auto *N = CB.getMetadata("callbase.id");
//   assert(N && "CallBase does not carry metadata.\n");
//   return std::string(cast<MDString>(N->getOperand(0))->getString());
// }

bool CallsiteIDRewritingPass::hasCallBaseId(const CallBase &CB) {
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

size_t CallsiteIDRewritingPass::getCallBaseId(const CallBase &CB) {
  auto *N = CB.getMetadata("callbase.id");
  assert(N && "CallBase does not carry metadata.\n");
  Constant *C = dyn_cast<ConstantAsMetadata>(dyn_cast<MDNode>(N)->getOperand(0))
                    ->getValue();
  return cast<ConstantInt>(C)->getZExtValue();
}