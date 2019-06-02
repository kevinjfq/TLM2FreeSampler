#ifndef TestableModule_H
#define TestableModule_H

// base class interface for testable Initiator modules, intended to be base for Initiators
struct TestableModule: public sc_module {
  TestableModule(const sc_module_name& name) : sc_module(name) {}
  virtual void runTests() = 0;
};

#endif
