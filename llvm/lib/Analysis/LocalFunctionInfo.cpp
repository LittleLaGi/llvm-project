#include "llvm/Analysis/LocalFunctionInfo.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>
#include <string>

using namespace llvm;

PreservedAnalyses LocalFunctionInfoPass::run(Module &M, ModuleAnalysisManager &AM) {
  std::string RecordFile;
  if (const char *RecordFileName = std::getenv("RECORD_LOCAL_FUNCTIONS"))
    RecordFile = RecordFileName;
  if (RecordFile.empty())
    return PreservedAnalyses::all();

  std::error_code EC;
  raw_fd_ostream FStream{RecordFile, EC};
  for (auto &F : M) {
    if (F.hasLocalLinkage() && !F.hasAddressTaken())
        FStream << F.getName() << ',' << F.getInstructionCount() << '\n';
  }

  return PreservedAnalyses::all();
}