#ifndef TopMockMemory_H
#define TopMockMemory_H

// Top of a SystemC hierarchy that assembles an initiator and a mock memory.

#include "testable_module.h"
#include "initiator_test_mock_memory.h"
#include "mock_memory1.h"

struct TopMockMemory : public TestableModule {
  InitiatorTestMockMemory *initiator;
  MockMemory1    *memory;
  TopMockMemory(const sc_module_name& name)
  : TestableModule(name)
  {
    // Instantiate components
    // (1) Instantiate and test the MockMemory with no intervening bus
    initiator = new InitiatorTestMockMemory("initiator");
    memory    = new MockMemory1   ("memory");
    // Bind initiator socket to target socket
    initiator->socket.bind( memory->socket );
  }
  void runTests() {
    initiator->test_1();
  }
};

#endif
