#pragma once

#include "llvm/Analysis/InlineAdvisor.h"
#include <set>

namespace llvm {

class InlineForceAdvisor : public InlineAdvisor {
public:
  friend class InlineForceAdvice;
  InlineForceAdvisor(FunctionAnalysisManager &FAM) : InlineAdvisor(FAM) {}
  std::unique_ptr<InlineAdvice> getAdvice(CallBase &CB) override;
};

InlineAdvisor *getForceAdvisor(FunctionAnalysisManager &FAM);

} // namespace llvm
