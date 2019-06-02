// This is just a demo, not a real cache model, since it does not implement its own
// data storage, but instead parasitically uses the same data space as the instance of
// 'SimplestMemory' that it fronts for (the memory provides a ptr to that space.)
// So it doesnt really have caceh hits and missed but simply uses rand() to model
// when a cache miss might have happened and then adds an additional cache miss
// penalty to the default delay.

#ifndef FakeCache_H
#define FakeCache_H

// Needed for the simple_target_socket
#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

#include "simplest_memory.h" // because FakeCache has a parasitic dependency on SimplestMemory

struct FakeCache: sc_module
{
  // TLM-2 socket, defaults to 32-bits wide, base protocol
  tlm_utils::simple_initiator_socket<FakeCache>	initiator_socket;
  // TLM-2 socket, defaults to 32-bits wide, base protocol
  tlm_utils::simple_target_socket<FakeCache> 		target_socket;

  // TODO: should be a configurable parameter
  //  HACK FOR TESTING!
  //  Setting the cache delay to 100ns which matches the memory access delay.
  //  Doing this only for re-use of testing harness
  const sc_time cacheDelay = sc_time(100, SC_NS);

  FakeCache(sc_module_name name, SimplestMemory* memory)
  : sc_module(name)
  , initiator_socket("initiator_socket")  // Construct and name initiator_socket
  , target_socket("target_socket")  // Construct and name target_socket
  , _impl_memory(NULL)
  {
    // Register callback for incoming b_transport interface method call
    target_socket.register_b_transport(this, &FakeCache::b_transport);

    // Implementation cheat: here is where we grab a direct handle to the data from the Memory\
    //  This is ok bc we are only interested in performance of the delays associated with the cache.
    _impl_memory = memory->getDirectMemoryPointer();

    // Initialize cache memory with zeroes
    //for (int i = 0; i < LINESIZE; i++) { cachelines[i] = 0x0 }
  }

  virtual bool isDataInCache( tlm::tlm_generic_payload& trans )
  {
    sc_dt::uint64    adr = trans.get_address() / 4;
    unsigned int     len = trans.get_data_length();

    // Assumption even addresses (really word indices) are in the cache
    return ( adr%2 == 0);
  }

  //  Delegate the call to the Memory
  //  Else return False
  virtual bool accessDataFromMemory( tlm::tlm_generic_payload& trans, sc_time& delay )
  {
	   initiator_socket->b_transport( trans, delay );  // Blocking transport call
     if ( trans.is_response_error() ) {
       std::ostringstream oss;
       oss << "Response error from b_transport call to Memory, " + sc_time_stamp().to_string() ;
       std::string s = oss.str();
       SC_REPORT_ERROR("FakeCache", s.c_str() );
       return false;
     }
     return  true;
  }

  //  Check if data is in the cache, fetch it with short delay.
  //  Else return False
  virtual void accessDataFromCache( tlm::tlm_generic_payload& trans, sc_time& delay )
  {
    tlm::tlm_command cmd = trans.get_command();
    sc_dt::uint64    adr = trans.get_address() / 4;
    unsigned char*   ptr = trans.get_data_ptr();
    unsigned int     len = trans.get_data_length();
    unsigned char*   byt = trans.get_byte_enable_ptr();
    unsigned int     wid = trans.get_streaming_width();

    // Obliged to implement read and write commands
    if ( cmd == tlm::TLM_READ_COMMAND )
      memcpy(ptr, &_impl_memory[adr], len);
    else if ( cmd == tlm::TLM_WRITE_COMMAND ) {
      memcpy(&_impl_memory[adr], ptr, len);
      // TODO: the actual HW needs to copy the data to the actual memory
    }
    delay += cacheDelay;
  }

  // TLM-2 blocking transport method
  //  Check if data is in the cache, return it with short delay.
  //  Else forward to memory (which has longer delay).
  virtual void b_transport( tlm::tlm_generic_payload& trans, sc_time& delay )
  {
    if( isDataInCache(trans) ) {
      accessDataFromCache(trans,delay);
      cout << "Cache hit.  delay is " << delay << endl;
    } else {
      accessDataFromMemory(trans,delay);
      cout << "Cache miss.  delay is " << delay << endl;
      // obliged to check response status and delay
      if ( trans.is_response_error() ) {
        //SC_REPORT_ERROR("TLM-2", "Cache: Received response error from memory!");
        cout << "ERROR in transaction" << endl;
      }
    }
    trans.set_response_status( tlm::TLM_OK_RESPONSE );
  }
  SC_HAS_PROCESS(FakeCache);

protected:
  // ptr to implementation memory : i.e. where the functionally correct data is actually stored/accessed
  uint32_t* _impl_memory;
};

//const sc_time Cache::cacheDelay = sc_time(10, SC_NS);

#endif
