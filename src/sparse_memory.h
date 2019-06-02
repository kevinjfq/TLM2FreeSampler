#ifndef MEMORY_SPARSE_H
#define MEMORY_SPARSE_H

// Sparse Memory class
// Behavior is that it doesnt allocate any actual space until a transaction asks to
// access some memory, then this allocates a page containing that and
// implements the transaction.
//  delay += 100 //always
// By default, for testing, memory is initialized with words alternating 0,1,0,1,...
// Allocated mem pages are kept ion a hash map (unordered_map)

// Needed for the simple_target_socket
#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"

#include <unordered_map>

// Target module representing a simple memory

struct SparseMemory: sc_module
{
  // TLM-2 socket, defaults to 32-bits wide, base protocol
  tlm_utils::simple_target_socket<SparseMemory> socket;

  SC_CTOR(SparseMemory)
  : socket("socket")
  {
    // Register callback for incoming b_transport interface method call
    socket.register_b_transport(this, &SparseMemory::b_transport);
    socket.register_get_direct_mem_ptr(this, &SparseMemory::get_direct_mem_ptr);
    socket.register_transport_dbg(this, &SparseMemory::transport_dbg);
  }

  virtual void end_of_simulation	()
  {
    dump();
  }

  virtual void dump()
  {
    cout << "Dumping the set of address blocks allocated in memory. " << endl;
    for( unordered_map<sc_dt::uint64,uint32_t*>::iterator itr = m_writtenAddresses.begin(); itr != m_writtenAddresses.end(); ++itr) {
      cout << hex << itr->first << endl;
    }
  }

  // return a page of Memory
  // if never accessed before, allocate and initialize it
  virtual uint32_t* fetchMemoryPage( sc_dt::uint64 adr )
  {
    uint32_t* page = NULL;
    sc_dt::uint64 pageID = adr / PAGESIZE;
    // From our hashmap, grab the page containing that address. If not there, allocate and init it.
    unordered_map<sc_dt::uint64,uint32_t*>::iterator itr = m_writtenAddresses.find(pageID);
    if( itr != m_writtenAddresses.end() ) {
      page = m_writtenAddresses[pageID];
    } else {
      page = new uint32_t[PAGESIZE];
      memset(page,0, PAGESIZE);
      for( int ii=1; ii<PAGESIZE-2; ii+=2) { page[ii] = 1; } // fill with word pattern of 0,1,0,1,...
      m_writtenAddresses[pageID] = page;
    }
    return page;
  }

  // TLM-2 blocking transport method
  virtual void b_transport( tlm::tlm_generic_payload& trans, sc_time& delay )
  {
    tlm::tlm_command  cmd = trans.get_command();
    sc_dt::uint64     adr = trans.get_address() / sizeof(uint32_t);
    uint32_t*         ptr = reinterpret_cast<uint32_t*>( trans.get_data_ptr() );
    unsigned int     len = trans.get_data_length();
    unsigned char*   byt = trans.get_byte_enable_ptr();
    unsigned int     wid = trans.get_streaming_width();

    // *********************************************
    // Generate the appropriate error response
    // *********************************************
    if (adr >= sc_dt::uint64(MAXSIZEMEM)) {
      trans.set_response_status( tlm::TLM_ADDRESS_ERROR_RESPONSE );
      return;
    }
    if (byt != 0) {
      trans.set_response_status( tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE );
      return;
    }
    if (len > 4 || wid < len) {
      trans.set_response_status( tlm::TLM_BURST_ERROR_RESPONSE );
      return;
    }

    vector<string> CMDS = { "READ", "WRITE"};
    //cout << "b_transport 1:  " << "cmd=" << CMDS[cmd] << " adr=" << hex << adr << endl;
    //if ( cmd == tlm::TLM_WRITE_COMMAND ) {      cout << " data=" << *ptr << endl;    }

    // delay always incrd by 100
    delay += sc_time(100, SC_NS);

    uint32_t* page = fetchMemoryPage(adr);
    sc_dt::uint64 pageID = adr / PAGESIZE;
    sc_dt::uint64 offset = adr % PAGESIZE;
    //cout << "b_transport 4:  " << "pageID hex=" << hex << pageID << " pageID dec=" << dec << pageID << endl;
    //cout << "b_transport 4:  " << "adr hex=" << hex << adr << " adr dec=" << dec << adr << endl;
    //cout << "b_transport 4:  " << "page hex=" << hex << page << " page dec=" << dec << page << endl;
    //cout << "b_transport 4:  " << "offset hex=" << hex << offset << " offset dec=" << dec << offset<< endl;
    //cout << "b_transport 4:  " << "page[offset] before, hex=" << hex << page[offset] << " dec=" << dec << page[offset]<< endl;

    // Obliged to implement read and write commands
    if ( cmd == tlm::TLM_READ_COMMAND ) {
      *ptr = page[offset];
    } else if ( cmd == tlm::TLM_WRITE_COMMAND ) {
      page[offset] = *ptr;
    }
    // no additional behavior for TLM_WRITE_COMMAND
    //cout << "b_transport 5:  " << "page[offset] after, hex=" << hex << page[offset] << " dec=" << dec << page[offset]<< endl;

    // Set DMI hint to indicated that DMI is supported
    trans.set_dmi_allowed(true);

    trans.set_response_status( tlm::TLM_OK_RESPONSE );
  }

  // TLM-2 forward DMI method
  virtual bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans,
                                  tlm::tlm_dmi& dmi_data)
  {
    sc_dt::uint64     adr = trans.get_address() / sizeof(uint32_t);

    // Permit read and write access
    dmi_data.allow_read_write();

    // Set other details of DMI region
    //cout << "get_direct_mem_ptr 0:  " << "adr=" << hex << adr << endl;
    uint32_t* page = fetchMemoryPage(adr);
    sc_dt::uint64 start_adr = (adr / PAGESIZE) * PAGESIZE;
    //cout << "get_direct_mem_ptr 1:  " << "page=" << hex << page << endl;
    dmi_data.set_dmi_ptr( reinterpret_cast<unsigned char*>( page ) );
    //cout << "get_direct_mem_ptr 2:  " << "start_adr=" << hex << start_adr << endl;
    dmi_data.set_start_address( start_adr );
    dmi_data.set_end_address( start_adr + PAGESIZE - 1 );
    dmi_data.set_read_latency( LATENCY_ONE_CLOCK );
    dmi_data.set_write_latency( LATENCY_ONE_CLOCK );

    return true;
  }

  // TLM-2 debug transport method
  virtual unsigned int transport_dbg(tlm::tlm_generic_payload& trans)
  {
    tlm::tlm_command  cmd = trans.get_command();
    sc_dt::uint64     adr = trans.get_address() / 4;
    uint32_t*         ptr = reinterpret_cast<uint32_t*>( trans.get_data_ptr() );
    // TODO: enhance to support len
    //unsigned int     len = trans.get_data_length();
    // Calculate the number of bytes to be actually copied
    //unsigned int num_bytes = (len < (SIZE - adr) * 4) ? len : (SIZE - adr) * 4;

    if ( cmd == tlm::TLM_READ_COMMAND ) {
      // TODO: make this thing really report values that were already "written" to this memory
      // isEven(addr/4) same as (adr%8==0)
      if ( adr%8==0 ) {
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
public:
  const int PAGESIZE = 4096 / sizeof(uint32_t); // each memory page will be 4k or 1k 32 bit ints
  const sc_time LATENCY_ONE_CLOCK = sc_time(10, SC_NS);
  enum { MAXSIZEMEM = (1<<20) }; // 1MiB
protected:
  unordered_map<sc_dt::uint64,uint32_t*> m_writtenAddresses;

} ;
#endif
