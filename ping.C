// This test creates two array chares: the Pingers and the Pingees

// Each Pinger performs a number of iterations in which it sends to each Pingee
// the array index of the Pinger and the current iteration.

// Each Pingee starts with an empty
// std::unordered_map<int, std::unordered_multiset<int>>>.
// Each time the Pingee is called, it inserts the received array index of the
// Pinger into the std::unordered_multiset<int> retrieved from the
// std::unordered_map at the received iteration.

// At the end, it is checked that the std::unordered_map has an entry at each
// iteration, and that for each iteration, the std::unordered_multiset contains
// one entry for each Pinger.

#include "ping.decl.h"

#include <chrono>
#include <unordered_map>
#include <unordered_set>

/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ CProxy_Pingers pingersProxy;
/*readonly*/ CProxy_Pingees pingeesProxy;

namespace {
// c++14 string literal for milliseconds
constexpr std::chrono::milliseconds operator "" _ms(unsigned long long ms) {
  return std::chrono::milliseconds(ms);
}
  
static constexpr std::chrono::milliseconds spin_time = 10_ms;
static constexpr int number_of_pingers = 10;
static constexpr int number_of_pingees = 1;
static constexpr int number_of_iterations = 4;

// wait `time_to_spin` milliseconds
void spin(const std::chrono::milliseconds& time_to_spin) {
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < time_to_spin) {
  }
}
}  // namespace

class Main : public CBase_Main {
private:
  int migrations{0};
public:
  Main(CkArgMsg* msg) {
    delete msg;

    pingersProxy = CProxy_Pingers::ckNew(number_of_pingers);
    pingeesProxy = CProxy_Pingees::ckNew(number_of_pingees);
    CkStartQD(CkCallback(CkIndex_Main::execute(), mainProxy));
  }

  void execute() {
    CkPrintf("Main is in phase execute\n");
    pingersProxy.send_pings();
    CkStartQD(CkCallback(CkIndex_Main::check(), mainProxy));
  }

  void check() {
    CkPrintf("Main is in phase check\n");
    pingeesProxy.check(migrations);
    CkStartQD(CkCallback(CkIndex_Main::exit(), mainProxy));
  }

  void exit() {
    CkPrintf("Main is in phase exit\n");
    CkExit();
  }

  void migrated() {
    ++migrations;
    CkPrintf("Migrations done: %i\n", migrations);
  }

  void count_errors(const int errors) {
    if (errors > 0) {
      CkPrintf("Errors: %i\n", errors);
      CkAbort("Test failed!\n");
    }
  }
};

class Pingers : public CBase_Pingers {
 public:
  Pingers() {}

  Pingers(CkMigrateMessage* /*msg*/) {}

  void send_pings() {
    for (int iteration = 1; iteration <= number_of_iterations; ++iteration) {
      spin(spin_time);
      pingeesProxy.receive_ping(iteration, thisIndex);
    }
  }
};
 
class Pingees : public CBase_Pingees {
private:
  std::unordered_map<int, std::unordered_multiset<int>> pings{};
  int migrations{0};
  int initial_proc{-1};
public:
  Pingees() : initial_proc(CkMyPe()) {}

  Pingees(CkMigrateMessage* /*msg*/) {}

  void receive_ping(const int iteration, const int index_of_pinger) {
    pings[iteration].insert(index_of_pinger);
  }

  void check(const int migrations_recorded_by_main) {
    CkAssert(migrations == migrations_recorded_by_main);
    // RotateLB should increase proc at each migration
    CkAssert(CkMyPe() == ((initial_proc + migrations) % CkNumPes()));

    int errors = 0;
    for (int i = 1; i <= number_of_iterations; ++i) {
      CkAssert(pings.count(i) == 1);
      for (int p = 0; p < number_of_pingers; ++p) {
        if (pings.at(i).count(p) != 1) {
          ++errors;
          CkPrintf("Pingee %i unexpected count %zu on iteration %i for pinger"
		   " %i\n", thisIndex, pings.at(i).count(p), i, p);
        }
      }
    }

    contribute(sizeof(int), &errors, CkReduction::sum_int,
               CkCallback(CkReductionTarget(Main, count_errors), mainProxy));
  }

  void pup(PUP::er& p) {
    p | pings;
    p | migrations;
    p | initial_proc;
    if (p.isUnpacking()) {
      ++migrations;
      contribute(CkCallback(CkReductionTarget(Main, migrated), mainProxy));
    }
  }
};

#include "ping.def.h"
