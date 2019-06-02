#ifndef TOP_H
#define TOP_H

// This is not really a Top of a particular SystemC module hierarchy./
// Instead it holds a vector of actual such Top modules, and invokes "runTests()" on each.
// Thus , this serves as an easy way to instantiate mulltiple test systems all in the
// same place and test them all at once.
// The ctor instantiates all the Top modules, and the thread process calls runTests on each.

#include <vector>
#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "testable_module.h"
#include "top_mock_memory.h"
#include "top_simplest_memory.h"
#include "top_fake_cache.h"
#include "top_real_cache.h"
#include "top_sparse_memory.h"

SC_MODULE(Top)
{
  // Here we instantiate several testable modules
  vector<TestableModule*> m_testable_modules;

  SC_CTOR(Top)
  {
    m_testable_modules.push_back(new TopMockMemory("TopMockMemory")  );
    m_testable_modules.push_back(new TopSimplestMemory("TopSimplestMemory")  );
    m_testable_modules.push_back(new TopFakeCache("TopFakeCache")  );
    m_testable_modules.push_back(new TopRealCache("TopRealCache")  );
    m_testable_modules.push_back(new TopSparseMemory("TopSparseMemory")  );
    SC_THREAD(thread_process);
  }

  void thread_process()
  {
    sc_report_handler::set_actions(SC_ERROR,SC_DISPLAY);
    vector<TestableModule*>::iterator itr;
    for( itr=m_testable_modules.begin(); itr!=m_testable_modules.end(); ++itr) {
      //(*itr)->runTests();
      TestableModule* m = *itr;
      cout << "============================================" << endl;
      cout << "Running tests for " << m->name() << endl;
      m->runTests();
    }
  }


};

#endif
