// A real model of a Cache.
// Implementation uses a separate C class CacheStore from subdirectory.
//
// Initial implementation only support singel byte read/write by Initiator.
// But reads and caches entire cache lines at once.

#ifndef RealCache_H
#define RealCache_H

#define FMT_HEADER_ONLY
#include "fmt/format.h"

// Needed for the simple_target_socket
#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

#include "cache_store/CacheStore.h"

struct RealCache: sc_module
{
  // TLM-2 socket, defaults to 32-bits wide, base protocol
  tlm_utils::simple_initiator_socket<RealCache>	initiator_socket;
  // TLM-2 socket, defaults to 32-bits wide, base protocol
  tlm_utils::simple_target_socket<RealCache> 		target_socket;

  // The CacheStore object that implements the cache storage
  CacheStore m_cacheStore;

  // TODO: should be a configurable parameter
  //  HACK FOR TESTING!
  //  Setting the cache delay to 100ns which matches the memory access delay.
  //  Doing this only for re-use of testing harness
  const sc_time cacheDelay = sc_time(100, SC_NS);

  RealCache(sc_module_name name, uint64_t memorySize=pow(2,30), uint64_t cacheSize=pow(2,20), uint64_t lineSize=pow(2,3), uint64_t numWays=pow(2,0) )
  : sc_module(name)
  , initiator_socket("initiator_socket")  // Construct and name initiator_socket
  , target_socket("target_socket")  // Construct and name target_socket
  , m_cacheStore( memorySize,cacheSize,lineSize,numWays)  // Construct and configure the CacheStore
  , m_cachetrans()
  {
    // Register callback for incoming b_transport interface method call
    target_socket.register_b_transport(this, &RealCache::b_transport);
  }

  //  Delegate the access call to the Memory
  //  Used for both read  & write.
    virtual bool accessDataFromMemory( tlm::tlm_generic_payload& trans, sc_time& delay )
  {
	   initiator_socket->b_transport( trans, delay );  // Blocking transport call
     if ( trans.is_response_error() ) {
       std::ostringstream oss;
       oss << "Response error from b_transport call to Memory, " + sc_time_stamp().to_string() ;
       std::string s = oss.str();
       SC_REPORT_ERROR("RealCache", s.c_str() );
       return false;
     }
     return  true;
  }

  // Used when the cache needs to read an entire line form memory -- ie to cache it.
  virtual void readLineFromMemory(sc_dt::uint64 adr, uint32_t* dataout, sc_time& delay )
  {
    unsigned char* d = reinterpret_cast<unsigned char*>(dataout);
    m_cachetrans.set_command(tlm::TLM_READ_COMMAND);
    m_cachetrans.set_address(m_cacheStore.getWordIndex(adr)); // TODO: Review! getLineAddress ?! shouldnt this be line address?
    m_cachetrans.set_data_ptr( d );
    m_cachetrans.set_data_length(m_cacheStore.p_LineSize);
    //m_cachetrans.set_data_length(sizeof(uint32_t));
    m_cachetrans.set_streaming_width( m_cacheStore.p_LineSize); // = data_length to indicate no streaming
    m_cachetrans.set_byte_enable_ptr( 0 ); // 0 indicates unused
    m_cachetrans.set_dmi_allowed( false ); // Mandatory initial value

    accessDataFromMemory(m_cachetrans,delay);
    if ( m_cachetrans.is_response_error() ) {
      //SC_REPORT_ERROR("TLM-2", "ERROR in transaction ro read dataline from memory!");
      cout << "ERROR RealCache: in transaction to read dataline from memory!" << endl;
    }
  }

  // TLM-2 blocking transport method
  //  Check if data is in the cache, return it with short delay.
  //  Else forward to memory (which has longer delay).
  virtual void b_transport( tlm::tlm_generic_payload& trans, sc_time& delay )
  {
    dump_trans("RealCache::b_transport ",trans);
    tlm::tlm_command cmd = trans.get_command();
    sc_dt::uint64    adr = trans.get_address();
    unsigned char*   ptr = trans.get_data_ptr();
    unsigned int     len = trans.get_data_length();
    unsigned char*   byt = trans.get_byte_enable_ptr();
    unsigned int     wid = trans.get_streaming_width();

    // Only supports single byte read and write.
    assert(len==sizeof(uint32_t));
    //assert(len==sizeof(uint32_t) && "RealCache only supports transactions of len==4");
    bool bHit;
    if ( cmd == tlm::TLM_READ_COMMAND ) {
      uint8_t* dataout = new uint8_t[m_cacheStore.p_LineSize](); // allocs array for dataline and inits all elems to 0
      bHit = m_cacheStore.getDataLine(adr, reinterpret_cast<uint32_t*>(dataout) ); //
      if ( bHit == true ) {
        // Hit!
        dump_line("dataout from cache: ", dataout);
        uint8_t dByte = dataout[m_cacheStore.getByteIndex(adr)];
        *ptr= dByte;
        cout << "Cache read hit 2.  delay is " << delay << endl;
      } else {
        // Miss!  Read the entire dataline from memory, cache it and return just the word requested
        // 1. Read the entire data line
        uint8_t* dataout = new uint8_t[m_cacheStore.p_LineSize](); // allocs array for dataline and inits all elems to 0
        readLineFromMemory(adr,reinterpret_cast<uint32_t*>(dataout),delay);
        dump_line("dataout from mem: ", dataout);
        // 2. cache the entire line from memory
        m_cacheStore.setDataLine(m_cacheStore.getLineAddress(adr), reinterpret_cast<uint32_t*>(dataout) );
        // 3. return just the word requested
        *ptr=dataout[m_cacheStore.getByteIndex(adr)];

        cout << "Cache miss.  delay is " << delay << endl;
      }

    } else if ( cmd == tlm::TLM_WRITE_COMMAND ) {
      // Because we only access entire datalines from CacheStore.
      uint8_t* dataout = new uint8_t[m_cacheStore.p_LineSize](); // allocs array for dataline and inits all elems to 0
      bHit = m_cacheStore.getDataLine(adr, reinterpret_cast<uint32_t*>(dataout));
      if ( bHit == true ) {
        dump_line("dataout from cache: ", dataout);
        dataout[m_cacheStore.getByteIndex(adr)] = *ptr;
        m_cacheStore.setDataLine(adr, reinterpret_cast<uint32_t*>(dataout));
        cout << "Cache write hit. " << endl;
        // TODO: schedule a write to memory
      } else {
        // write the data to memory, then read the line from memory and cache it.
        // 1. write the data to memory
        accessDataFromMemory(trans,delay);
        cout << "Cache write miss.  delay is " << delay << endl;
        if ( trans.is_response_error() ) {
          //SC_REPORT_ERROR("TLM-2", "Cache: Received response error from memory!");
          cout << "ERROR RealCache: Writing to memory upon a write miss" << endl;
        } else {
          // 2. read the entire line from memory with the newly written data
          //TODO: create a new transaction for this!!! why corrupt the existing one?
          readLineFromMemory(adr,reinterpret_cast<uint32_t*>(dataout),delay);
          dump_line("dataout from mem: ", dataout);
          // 3. put line from memory into the cache
          m_cacheStore.setDataLine(m_cacheStore.getLineAddress(adr), reinterpret_cast<uint32_t*>(dataout) );
        }
      }
    }
    trans.set_response_status( tlm::TLM_OK_RESPONSE );
  }

  virtual void dump_trans( const char* msg, tlm::tlm_generic_payload& trans )
  {
    return;
    fmt::print(
      "{}: cmd={} adr={} ptr={} len={} wid={}\n"
      , msg
      , trans.get_command()
      , trans.get_address()
      , trans.get_data_ptr()
      , trans.get_data_length()
      , trans.get_streaming_width()
    );

    /*
    // This style of named args works with my C++ 11 but I don't consider it an improvement on the positional approach above.
    fmt::print(
          "{msg}: cmd={cmd} adr={adr} ptr={ptr} len={len} wid={wid}\n"
          , fmt::arg("msg", msg)
          , fmt::arg("cmd", trans.get_command())
          , fmt::arg("adr", trans.get_address())
          , fmt::arg("ptr", trans.get_data_ptr())
          , fmt::arg("len", trans.get_data_length())
          , fmt::arg("wid", trans.get_streaming_width())
        );
    // This style of named args doesnt work with my C++ 11 ?!?
    fmt::print(
          "{msg}: cmd={cmd} adr={adr} ptr={ptr} len={len} wid={wid}\n"
          , "msg"_a=msg
          , "cmd"_a=trans.get_command()
          , "adr"_a=trans.get_address()
          , "ptr"_a=trans.get_data_ptr()
          , "len"_a=trans.get_data_length()
          , "wid"_a=trans.get_streaming_width()
        );
        // , "byt"_a=trans.get_byte_enable_ptr()
    */
  }
  /*
  // Here is a cout+checvrons version of dump_trans which is not as nice as the fmt version.
  virtual void dump_trans( const char* msg, tlm::tlm_generic_payload& trans )
  {
    return;
    cout << msg << " : " ;
    tlm::tlm_command cmd = trans.get_command();
    sc_dt::uint64    adr = trans.get_address();
    unsigned char*   ptr = trans.get_data_ptr();
    unsigned int     len = trans.get_data_length();
    unsigned char*   byt = trans.get_byte_enable_ptr();
    unsigned int     wid = trans.get_streaming_width();
    cout
    << " cmd = " << cmd
    << " adr=" << hex << adr
    << " ptr=" << ptr
    << " len=" << dec << len
    //<< " byt=" << byt
    << " wid=" << wid
    << endl;
  }
  */

  /*
  // Here is an fmt version of dump_line.  I don't like it.  using 'cout' seems easier.
  virtual void dump_line( const char* msg, uint8_t* dataline )
  {
    return;
    fmt::memory_buffer out;
    format_to(out, "{} :", msg);
    for( int jj=0; jj<m_cacheStore.p_LineSize; jj++) {
      format_to(out, " [{}]={}", jj, (int)dataline[jj] );
    }
    fmt::print("{}\n",fmt::to_string(out) );
  }
  */
  // Here is the typical approach using cout and chevrons.
  virtual void dump_line( const char* msg, uint8_t* dataline )
  {
    return;
    cout << msg << " : " ;
    for( int jj=0; jj<m_cacheStore.p_LineSize; jj++)
      cout << " [" << jj << "]=" << (int)dataline[jj];
    cout<<endl;
  }

  SC_HAS_PROCESS(RealCache);

protected:
  tlm::tlm_generic_payload m_cachetrans;
};

//const sc_time Cache::cacheDelay = sc_time(10, SC_NS);

#endif
