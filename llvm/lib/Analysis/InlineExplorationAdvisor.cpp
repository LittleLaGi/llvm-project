#include "llvm/Analysis/InlineExplorationAdvisor.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/CommandLine.h"

#include <fstream>
#include <iomanip>
#include <set>

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

struct CallBaseEntry {
  std::string Caller;
  std::string Callee;
  size_t Id;
};

void to_json(json &J, const CallBaseEntry &P) {
  J = json{{"caller", P.Caller}, {"callee", P.Callee}, {"id", P.Id}};
}

bool operator<(const CallBaseEntry &A, const CallBaseEntry &B) {
  return std::tie(A.Id, A.Caller, B.Callee) <
         std::tie(B.Id, B.Caller, B.Callee);
}

class CallSiteRecord {
public:
  CallSiteRecord() {
    if (ExplorationMode == InlineExplorationAdvisorMode::Replay) {
      std::ifstream In{InlineExplorationAdvisorInput};
      json J;
      In >> J;
      for (const auto &Cs : J) {
        Decisions[CallBaseEntry{Cs["caller"], Cs["callee"], Cs["id"]}] =
            Cs["inline"];
      }
    }
  }
  ~CallSiteRecord() {
    if (ExplorationMode == InlineExplorationAdvisorMode::Record) {
      json J{Callsites};
      std::ofstream Out{InlineExplorationAdvisorInput};
      Out << std::setw(4) << J[0] << std::endl;
    }
  }

  void insert(CallBaseEntry &&Callsite) {
    Callsites.insert(std::move(Callsite));
  }

  bool decision(CallBaseEntry &&Callsite) {
    return Decisions.at(std::move(Callsite));
  }

  void addTransitiveDecisions(const std::string &Caller,
                              const std::string &Callee) {
    std::vector<CallBaseEntry> CallsitesFromCallee;
    for (const auto &D : Decisions)
      if (D.first.Caller == Callee)
        CallsitesFromCallee.push_back(D.first);
    for (const auto &Cs : CallsitesFromCallee)
      Decisions[CallBaseEntry{Caller, Cs.Callee, Cs.Id}] = Decisions.at(Cs);
  }

  std::set<CallBaseEntry> Callsites;
  std::map<CallBaseEntry, bool> Decisions;
};

CallSiteRecord *getCallSiteRecord() {
  static auto Record = std::make_unique<CallSiteRecord>();
  return Record.get();
}

size_t getCallBaseId(const CallBase &CB) {
  auto *N = CB.getMetadata("callbase.id");
  Constant *C = dyn_cast<ConstantAsMetadata>(dyn_cast<MDNode>(N)->getOperand(0))
                    ->getValue();
  return cast<ConstantInt>(C)->getZExtValue();
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
  case InlineExplorationAdvisorMode::Replay: {
    auto &Decisions = getCallSiteRecord()->Decisions;
    auto Key = CallBaseEntry{CB.getCaller()->getName().str(),
                             CB.getCalledFunction()->getName().str(),
                             getCallBaseId(CB)};

    // outs() << "<Query>\n";
    // outs() << Key.caller << ' ' << Key.callee << '\n';
    // outs() << "</Query>\n";
    if (Decisions.count(Key) == 0) {
      outs() << "[InlineExplorationAdvisor]; unknown callsite: " << CB.getName()
             << '\n';
      return std::make_unique<InlineExplorationAdvice>(
          this, CB,
          FAM.getResult<OptimizationRemarkEmitterAnalysis>(*CB.getCaller()),
          false);
    }

    return std::make_unique<InlineExplorationAdvice>(
        this, CB,
        FAM.getResult<OptimizationRemarkEmitterAnalysis>(*CB.getCaller()),
        getCallSiteRecord()->decision(std::move(Key)));
  }
  default:
    std::abort();
  }
}

void InlineExplorationAdvisor::recordCallsite(const CallBase &CB) {
  getCallSiteRecord()->insert(CallBaseEntry{
      CB.getCaller()->getName().str(), CB.getCalledFunction()->getName().str(),
      getCallBaseId(CB)});
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
  // outs() << "Inline unattempted\n";
}

namespace llvm {
InlineAdvisor *getExplorationAdvisor(FunctionAnalysisManager &FAM) {
  return new InlineExplorationAdvisor(FAM);
}
} // namespace llvm
