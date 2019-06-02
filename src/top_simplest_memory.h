#ifndef TopSimplestMemory_H
#define TopSimplestMemory_H

// Top of a SystemC hierarchy that assembles an initiator  and SimplestMemory.

#include "testable_module.h"
#include "initiator_test_simplest_memory.h"
#include "simplest_memory.h"

struct TopSimplestMemory : TestableModule {
  InitiatorTestSimplestMemory *initiatorTestSimplestMemory;
  SimplestMemory    *simplestMemory;

  TopSimplestMemory(const sc_module_name& name)
  : TestableModule(name)
  {
    initiatorTestSimplestMemory = new InitiatorTestSimplestMemory("InitiatorTestSimplestMemory");
    simplestMemory    = new SimplestMemory   ("SimplestMemory");
    initiatorTestSimplestMemory->socket.bind( simplestMemory->socket );
  }
  void runTests() {
    initiatorTestSimplestMemory->test_1();
  }
};

#endif
