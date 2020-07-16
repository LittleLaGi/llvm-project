#include "llvm/Analysis/InlineExplorationAdvisor.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/CommandLine.h"

#include <fstream>
#include <iomanip>

#include "json.hpp"

using namespace llvm;
using json = nlohmann::json;

namespace {

enum class InlineExplorationAdvisorMode { Record, Replay };

cl::opt<InlineExplorationAdvisorMode> ExplorationMode(
    "exploration-mode", cl::init(InlineExplorationAdvisorMode::Record),
    cl::desc("Choose the inlining exploration advisor mode."),
    cl::values(clEnumValN(InlineExplorationAdvisorMode::Record, "record",
                          "Record the available decisions"),
               clEnumValN(InlineExplorationAdvisorMode::Replay, "replay",
                          "Replay the decisions provided in input")));

cl::opt<std::string> InlineExplorationAdvisorInput(
    "exploration-input-file", cl::value_desc("filename"),
    cl::init("/tmp/inline_explore_advisor"),
    cl::desc("JSON file that is used either as input or output depending on "
             "the exploration mode. (default:/tmp/inline_explore_advisor"));

struct StringPair {
  std::string caller;
  std::string callee;
};

void to_json(json &j, const StringPair &p) {
  j = json{{"caller", p.caller}, {"callee", p.callee}};
}

// bool operator==(const StringPair &a, const StringPair &b) {
// return a.caller == b.caller && a.callee == b.callee;
//}

bool operator<(const StringPair &a, const StringPair &b) {
  if (a.caller == b.caller)
    return a.callee < b.callee;
  return a.caller < b.caller;
}

class CallSiteRecord {
public:
  CallSiteRecord() {
    if (ExplorationMode == InlineExplorationAdvisorMode::Replay) {
      std::ifstream in{InlineExplorationAdvisorInput};
      json j;
      in >> j;
      for (const auto &cs : j)
        decisions[StringPair{cs["caller"], cs["callee"]}] = cs["inline"];
    }
  }
  ~CallSiteRecord() {
    if (ExplorationMode == InlineExplorationAdvisorMode::Record) {
      json j{callsites};
      std::ofstream out{InlineExplorationAdvisorInput};
      out << std::setw(4) << j[0] << std::endl;
    }
  }

  void insert(StringPair &&callsite) { callsites.insert(std::move(callsite)); }

  bool decision(StringPair &&callsite) {
    return decisions.at(std::move(callsite));
  }

  void addTransitiveDecisions(const std::string &Caller,
                              const std::string &Callee) {
    std::vector<StringPair> callsitesFromCallee;
    for (const auto &d : decisions)
      if (d.first.caller == Callee)
        callsitesFromCallee.push_back(d.first);
    for (const auto &cs : callsitesFromCallee)
      decisions[StringPair{Caller, cs.callee}] = decisions.at(cs);
  }

  std::set<StringPair> callsites;
  std::map<StringPair, bool> decisions;
};

CallSiteRecord *getCallSiteRecord() {
  static auto record = std::make_unique<CallSiteRecord>();
  return record.get();
}

} // namespace

std::unique_ptr<InlineAdvice>
InlineExplorationAdvisor::getAdvice(CallBase &CB) {
  switch (ExplorationMode) {
  case InlineExplorationAdvisorMode::Record:
    recordCallsite(CB);
    return std::make_unique<InlineExplorationAdvice>(
        this, CB,
        FAM.getResult<OptimizationRemarkEmitterAnalysis>(*CB.getCaller()),
        false);
  case InlineExplorationAdvisorMode::Replay:
    return std::make_unique<InlineExplorationAdvice>(
        this, CB,
        FAM.getResult<OptimizationRemarkEmitterAnalysis>(*CB.getCaller()),
        getCallSiteRecord()->decision(
            StringPair{CB.getCaller()->getName().str(),
                       CB.getCalledFunction()->getName().str()}));
  default:
    std::abort();
  }
}

void InlineExplorationAdvisor::recordCallsite(const CallBase &CB) {
  getCallSiteRecord()->insert(
      StringPair{CB.getCaller()->getName().str(),
                 CB.getCalledFunction()->getName().str()});
}

void InlineExplorationAdvisor::recordInlining(const Function *Caller,
                                              const Function *Callee) {
  getCallSiteRecord()->addTransitiveDecisions(Caller->getName().str(),
                                              Callee->getName().str());
}

void InlineExplorationAdvice::recordInliningImpl() {
  static_cast<InlineExplorationAdvisor *>(Advisor)->recordInlining(Caller,
                                                                   Callee);
}

void InlineExplorationAdvice::recordInliningWithCalleeDeletedImpl() {
  static_cast<InlineExplorationAdvisor *>(Advisor)->recordInlining(Caller,
                                                                   Callee);
}

void InlineExplorationAdvice::recordUnsuccessfulInliningImpl(
    const InlineResult &Result) {
  outs() << "Inline unsuccessful: " << Result.getFailureReason() << "\n";
}
void InlineExplorationAdvice::recordUnattemptedInliningImpl() {
  //outs() << "Inline unattempted\n";
}

namespace llvm {
InlineAdvisor *getExplorationAdvisor(FunctionAnalysisManager &FAM) {
  return new InlineExplorationAdvisor(FAM);
}
} // namespace llvm
