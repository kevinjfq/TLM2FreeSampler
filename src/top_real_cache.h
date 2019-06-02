#ifndef TopRealCache_H
#define TopRealCache_H

// Top of a SystemC hierarchy that assembles an initiator, RealCache, and SimplestMemory.

#include "testable_module.h"
#include "initiator_test_simplest_memory.h"
#include "simplest_memory.h"
#include "real_cache.h"

struct TopRealCache : TestableModule {
  InitiatorTestSimplestMemory *initiatorTestSimplestMemory;
  SimplestMemory              *simplestMemory;
  RealCache                   *realCache;

  TopRealCache(const sc_module_name& name)
  : TestableModule(name)
  {
    initiatorTestSimplestMemory = new InitiatorTestSimplestMemory("InitiatorTestSimplestMemory",100,0);
    simplestMemory    = new SimplestMemory   ("SimplestMemory");
    realCache    = new RealCache   ("RealCache",pow(2,10),pow(2,7),LineSize8,2);

    initiatorTestSimplestMemory->socket.bind( realCache->target_socket );
    realCache->initiator_socket.bind( simplestMemory->socket );
  }
  void runTests() {
    initiatorTestSimplestMemory->test_1();
  }
};

#endif
