#ifndef SimplestMemory_H
#define SimplestMemory_H

// Simple memory
//  - memory is modeled as a single contiguous block of 32bit values
//  - supports DMI by returning the pointer to that mem

// Needed for the simple_target_socket
#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"

// This Memory is implemented with a fixed buffer to represent actual memeory
struct SimplestMemory: sc_module
{
  tlm_utils::simple_target_socket<SimplestMemory> socket;

  // configurable parameters
  int p_PAGESIZE; // each memory page will be 4k bytes or 1k words (4byte or 32bit)
  int p_LINESIZE;  // how many words can be copied at once; must be a power of 2

  SimplestMemory(sc_module_name name, int PAGESIZE=(4096 / sizeof(uint32_t)), int LINESIZE=8)
  : socket("socket")
  , p_PAGESIZE(PAGESIZE)
  , p_LINESIZE(LINESIZE)
  {
    // Register callback for incoming b_transport interface method call
    socket.register_b_transport(this, &SimplestMemory::b_transport);

    // init mem with word pattern of 0,1,0,1,...
    m_mem = new uint32_t[p_PAGESIZE];
    memset(m_mem,0, p_PAGESIZE);
    for( int ii=1; ii<p_PAGESIZE-2; ii+=2) { m_mem[ii] = 1; }
  }

  // TLM-2 blocking transport method
  virtual void b_transport( tlm::tlm_generic_payload& trans, sc_time& delay )
  {
    tlm::tlm_command cmd = trans.get_command();
    sc_dt::uint64    adr = trans.get_address() / 4;
    unsigned char*   ptr = trans.get_data_ptr();
    unsigned int     len = trans.get_data_length(); // in bytes
    unsigned char*   byt = trans.get_byte_enable_ptr();
    unsigned int     wid = trans.get_streaming_width();

    if (adr >= sc_dt::uint64(p_PAGESIZE))
      SC_REPORT_ERROR("TLM-2", "Target does not support address of the given transaction");
    if ( byt != 0)
      SC_REPORT_ERROR("TLM-2", "Target does not support byte enable of the given transaction");
    if ( len > 4 * p_LINESIZE)
      SC_REPORT_ERROR("TLM-2", "Target does not support len of the given transaction");
    if ( wid < len)
      SC_REPORT_ERROR("TLM-2", "Target does not support width  of the given transaction");
    // delay always incrd by 100
    delay += sc_time(100, SC_NS);

    // Obliged to implement read and write commands
    if ( cmd == tlm::TLM_READ_COMMAND )
      memcpy(ptr, &m_mem[adr], len);
    else if ( cmd == tlm::TLM_WRITE_COMMAND )
      memcpy(&m_mem[adr], ptr, len);

    // Obliged to set response status to indicate successful completion
    trans.set_response_status( tlm::TLM_OK_RESPONSE );
  }
  // This is to support an implementation cheat by the FakeCache module class.
  virtual uint32_t* getDirectMemoryPointer()
  {
    return m_mem;
  }
  SC_HAS_PROCESS(SimplestMemory);

protected:
  uint32_t *m_mem;
};

#endif
