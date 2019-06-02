#ifndef MOCK_MEMORY1_H
#define MOCK_MEMORY1_H

// Simple Mock Memory class
// This is just here for unit testing. No actual storage is modeled.
// Reads always return 0 or 1. Writes dont do anything. Delay is always 100.

// Behavior is:
// read():
//  delay += 100 //always
//  if isEven(addr/4)
//    data = 0 // only one address will be writen, len is ignored
//  else
//    data = 1
// write():
//  delay += 100 //always
//  // take no action on actual data

// Needed for the simple_target_socket
#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"

struct MockMemory1: sc_module
{
  // TLM-2 socket, defaults to 32-bits wide, base protocol
  tlm_utils::simple_target_socket<MockMemory1> socket;

  SC_CTOR(MockMemory1)
  : socket("socket")
  {
    // Register callback for incoming b_transport interface method call
    socket.register_b_transport(this, &MockMemory1::b_transport);
    socket.register_transport_dbg(this, &MockMemory1::transport_dbg);
  }

  // TLM-2 blocking transport method
  virtual void b_transport( tlm::tlm_generic_payload& trans, sc_time& delay )
  {
    tlm::tlm_command  cmd = trans.get_command();
    sc_dt::uint64     adr = trans.get_address() / sizeof(uint32_t); // ie address / 4
    uint32_t*         ptr = reinterpret_cast<uint32_t*>( trans.get_data_ptr() );

    // delay always incrd by 100
    delay += sc_time(100, SC_NS);

    // Obliged to implement read and write commands
    if ( cmd == tlm::TLM_READ_COMMAND ) {
      if( adr % 2 == 0 ) {
        *ptr = 0;
      } else {
        *ptr = 1;
      }
    } else if ( cmd == tlm::TLM_WRITE_COMMAND ) {
      // no op
    }
    trans.set_response_status( tlm::TLM_OK_RESPONSE );
  }
  // *********************************************
  // TLM-2 debug transport method
  // *********************************************

  virtual unsigned int transport_dbg(tlm::tlm_generic_payload& trans)
  {
    tlm::tlm_command  cmd = trans.get_command();
    sc_dt::uint64     adr = trans.get_address() / sizeof(uint32_t); // ie address / 4
    uint32_t*         ptr = reinterpret_cast<uint32_t*>( trans.get_data_ptr() );
    if ( cmd == tlm::TLM_READ_COMMAND ) {
      if ( adr%2==0 ) {
        *ptr = 0;
      } else {
        *ptr = 1;
      }
    } else if ( cmd == tlm::TLM_WRITE_COMMAND ) {
      // Todo : support WRITE
    }
    trans.set_response_status( tlm::TLM_OK_RESPONSE );
    // TODO: enhance to support len
    return 4;
  }
protected:

} ;
#endif
