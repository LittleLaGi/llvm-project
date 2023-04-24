#include "llvm/Analysis/GeneralFunctionInfo.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>
#include <string>

using namespace llvm;

PreservedAnalyses GeneralFunctionInfoPass::run(Module &M, ModuleAnalysisManager &AM) {
  const char *RecordFileName = std::getenv("RECORD_GENERAL_FUNCTIONS");
  if (!RecordFileName)
    return PreservedAnalyses::all();

  std::string RecordFile;
  raw_string_ostream RecordStream{RecordFile};
  for (auto &F : M) {
    if (!F.isDeclaration())
      RecordStream << F.getName() << ',' << F.getInstructionCount() << '\n';
  }

  std::error_code EC;
  raw_fd_ostream FStream{RecordFileName, EC};
  FStream << RecordStream.str();

  return PreservedAnalyses::all();
}