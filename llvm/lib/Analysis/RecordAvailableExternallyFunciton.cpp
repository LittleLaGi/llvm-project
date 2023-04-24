#include "llvm/Analysis/RecordAvailableExternallyFunciton.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>

using namespace llvm;

PreservedAnalyses RecordAvailableExternallyFuncitonPass::run(Module &M, ModuleAnalysisManager &AM) {
  const char *FunctionFileName = std::getenv("RECORD_AVAILABLE_EXTERNALLY_FUNCTIONS");
  if (!FunctionFileName)
    return PreservedAnalyses::all();

  std::string FunctionString;
  raw_string_ostream FunctionStream{FunctionString};
  for (auto &F : M) {
    if (F.hasAvailableExternallyLinkage())
      FunctionStream << F.getName() << '\n';
  }

  std::error_code EC;
  raw_fd_ostream FStream{FunctionFileName, EC};
  FStream << FunctionStream.str();

  return PreservedAnalyses::all();
}