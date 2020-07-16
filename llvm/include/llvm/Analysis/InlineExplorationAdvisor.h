#pragma once

#include "llvm/Analysis/InlineAdvisor.h"
#include <set>

namespace llvm {

class InlineExplorationAdvisor : public InlineAdvisor {
public:
  friend class InlineExplorationAdvice;
  InlineExplorationAdvisor(FunctionAnalysisManager &FAM) : InlineAdvisor(FAM) {}
  std::unique_ptr<InlineAdvice> getAdvice(CallBase &CB) override;

private:
  void recordCallsite(const CallBase &CB);
  void recordInlining(const Function *Caller, const Function *Callee);
};

class InlineExplorationAdvice : public InlineAdvice {
public:
  InlineExplorationAdvice(InlineAdvisor *Advisor, CallBase &CB,
                          OptimizationRemarkEmitter &ORE,
                          bool IsInliningRecommended)
      : InlineAdvice(Advisor, CB, ORE, IsInliningRecommended) {}

protected:
  void recordInliningImpl() override;
  void recordInliningWithCalleeDeletedImpl() override;
  void recordUnsuccessfulInliningImpl(const InlineResult &Result) override;
  void recordUnattemptedInliningImpl() override;
};

InlineAdvisor *getExplorationAdvisor(FunctionAnalysisManager &FAM);

} // namespace llvm
