#ifndef TopSparseMemory_H
#define TopSparseMemory_H

// Top of a SystemC hierarchy that assembles an initiator  and SimplestMemory.

#include "testable_module.h"
#include "initiator_test_sparse_memory.h"
#include "sparse_memory.h"

struct TopSparseMemory : TestableModule {
  InitiatorTestSparseMemory *initiatorTestSparseMemory;
  SparseMemory    *sparseMemory;

  TopSparseMemory(const sc_module_name& name)
  : TestableModule(name)
  {
    initiatorTestSparseMemory = new InitiatorTestSparseMemory("InitiatorTestSparseMemory");
    sparseMemory    = new SparseMemory   ("sparseMemory");
    initiatorTestSparseMemory->socket.bind( sparseMemory->socket );
  }
  void runTests() {
    initiatorTestSparseMemory->test_1();
  }
};

#endif
