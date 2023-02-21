#include "llvm/Transforms/Instrumentation/LiveFunctionExploring.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>

using namespace llvm;

PreservedAnalyses LiveFunctionExploringPass::run(Module &M, ModuleAnalysisManager &AM) {
  const char *LiveFunctionFileName = std::getenv("RECORD_LIVE_FUNCTIONS");
  if (!LiveFunctionFileName)
    return PreservedAnalyses::all();
  
  std::string LiveFunctionString;
  raw_string_ostream LiveFunctionStream{LiveFunctionString};
  for (auto &F : M) {
    //if (!F.isDeclaration() && !(F.hasLocalLinkage() && F.use_empty()))
    //bool Removable = (F.hasLinkOnceODRLinkage() || F.hasWeakODRLinkage()) && F.use_empty();
    
    //bool Removable = F.hasLinkOnceLinkage() && F.use_empty();
    
    bool Removable = F.isDiscardableIfUnused() && F.use_empty();
    if (!F.isDeclaration())
      LiveFunctionStream << F.getName() << '\n';
  }

  std::error_code EC;
  raw_fd_ostream FStream{LiveFunctionFileName, EC};
  FStream << LiveFunctionStream.str();
  
  
  return PreservedAnalyses::all();
}
