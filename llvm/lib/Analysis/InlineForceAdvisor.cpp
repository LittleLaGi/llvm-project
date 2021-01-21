#include "llvm/Analysis/InlineForceAdvisor.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

namespace {

enum class InlineForceAdvisorMode { Inline, Noinline };

cl::opt<InlineForceAdvisorMode>
    ExplorationMode("force-mode", cl::init(InlineForceAdvisorMode::Noinline),
                    cl::desc("Choose the inlining force advisor mode."),
                    cl::values(clEnumValN(InlineForceAdvisorMode::Inline,
                                          "inline", "Inline everything"),
                               clEnumValN(InlineForceAdvisorMode::Noinline,
                                          "noinline", "Inline nothing")));
} // namespace

std::unique_ptr<InlineAdvice> InlineForceAdvisor::getAdvice(CallBase &CB) {
  switch (ExplorationMode) {
  case InlineForceAdvisorMode::Inline:
    return std::make_unique<InlineAdvice>(
        this, CB,
        FAM.getResult<OptimizationRemarkEmitterAnalysis>(*CB.getCaller()),
        true);
  case InlineForceAdvisorMode::Noinline:
    return std::make_unique<InlineAdvice>(
        this, CB,
        FAM.getResult<OptimizationRemarkEmitterAnalysis>(*CB.getCaller()),
        false);
  default:
    std::abort();
  }
}

namespace llvm {
InlineAdvisor *getForceAdvisor(FunctionAnalysisManager &FAM) {
  return new InlineForceAdvisor(FAM);
}
} // namespace llvm
