#ifndef TopFakeCache_H
#define TopFakeCache_H

// Top of a SystemC hierarchy that assembles an initiator, FakeCache, and SimplestMemory.

#include "testable_module.h"
#include "initiator_test_simplest_memory.h"
#include "simplest_memory.h"
#include "fake_cache.h"

struct TopFakeCache : TestableModule {
  InitiatorTestSimplestMemory *initiatorTestSimplestMemory;
  SimplestMemory              *simplestMemory;
  FakeCache                   *fakeCache;

  TopFakeCache(const sc_module_name& name)
  : TestableModule(name)
  {
    initiatorTestSimplestMemory = new InitiatorTestSimplestMemory("InitiatorTestSimplestMemory");
    simplestMemory    = new SimplestMemory   ("SimplestMemory");
    fakeCache    = new FakeCache   ("FakeCache",simplestMemory);

    initiatorTestSimplestMemory->socket.bind( fakeCache->target_socket );
    fakeCache->initiator_socket.bind( simplestMemory->socket );
  }
  void runTests() {
    initiatorTestSimplestMemory->test_1();
  }
};

#endif
