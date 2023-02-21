#include "llvm/Transforms/Instrumentation/VerifyInlineDecisions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Metadata.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <map>

using namespace llvm;

PreservedAnalyses VerifyInlineDecisionsPass::run(Module &M, ModuleAnalysisManager &AM) {
  const char *DebugInfoFileName = std::getenv("DEBUG_INFO_FILE");
  const char *DecisionsFileName = std::getenv("RECORD_DECISIONS");
  if (!DebugInfoFileName || !DecisionsFileName)
    return PreservedAnalyses::all();
  std::ifstream IStream{DecisionsFileName};
  std::string DebugInfoString;
  raw_string_ostream DebugInfoStream{DebugInfoString};
  DebugInfoStream << "----- start verification -----" << '\n';

  std::map<size_t, std::string> ID2Decision;
  for (std::string Line; std::getline(IStream, Line);) {
    SmallVector<StringRef, 4> LineParts;
    StringRef{Line}.split(LineParts, ',');
    if (LineParts.size() != 4) {
      DebugInfoStream << "[Error] Wrong decision format: " << Line << '\n';
      continue;
    }
    std::istringstream iss(LineParts[2].str());
    size_t ID;
    iss >> ID;
    ID2Decision[ID] = std::string(LineParts[3].str());
    if (ID2Decision[ID] == "undecided")
      DebugInfoStream << "[Error] Undecided decision: " << Line << '\n';
  }

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *CB = dyn_cast<CallBase>(&I);
        if (!CB)
          continue;
        Function *CalledFunc = CB->getCalledFunction();
        if (CalledFunc && !CalledFunc->isDeclaration()) {
          if (hasCallBaseId(*CB)) {
            auto ID = getCallBaseId(*CB);
            if (!ID2Decision.count(ID))
              DebugInfoStream << "[Error] CB (" << ID << ") not in decisions: " << F.getName() << "," << CalledFunc->getName() << '\n';
            else if (ID2Decision[ID] != "not_inlined")
              DebugInfoStream << "[Error] Should be not_inlined: " << F.getName() << "," << CalledFunc->getName() << "," << ID << "," << ID2Decision[ID] << '\n';
          }
          else
            DebugInfoStream << "[Error] Missing ID " << F.getName() << "," << CalledFunc->getName() << '\n';
        }
      }
    }
  }

  if (DebugInfoFileName) {
    std::error_code EC;
    raw_fd_ostream FStream{DebugInfoFileName, EC};
    FStream << DebugInfoStream.str();
  }
  return PreservedAnalyses::all();
}

bool VerifyInlineDecisionsPass::hasCallBaseId(const CallBase &CB) {
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

size_t VerifyInlineDecisionsPass::getCallBaseId(const CallBase &CB) {
  auto *N = CB.getMetadata("callbase.id");
  assert(N && "CallBase does not carry metadata.\n");
  Constant *C = dyn_cast<ConstantAsMetadata>(dyn_cast<MDNode>(N)->getOperand(0))
                    ->getValue();
  return cast<ConstantInt>(C)->getZExtValue();
}