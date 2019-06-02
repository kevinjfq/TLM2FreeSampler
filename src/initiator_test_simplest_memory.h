#ifndef InitiatorTestSimplestMemory_h
#define InitiatorTestSimplestMemory_h

// Test Initiator class
// This is just for SystemC "unit" testing (though since its systemc, it's really a kind of integration test).
// This works in conjunction with SimplestMemory and with RealCache.
// It issues a short sequence of read and write transactions and checks the results.

#include <string>
#include <sstream>
#include "systemc"
//#include <sc_report.h>
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"

struct InitiatorTestSimplestMemory: sc_module
{
  // TLM-2 socket, defaults to 32-bits wide, base protocol
  tlm_utils::simple_initiator_socket<InitiatorTestSimplestMemory> socket;

  sc_time m_missDelay;
  sc_time m_hitDelay;

  InitiatorTestSimplestMemory(sc_module_name name, uint64_t missDelayNS=100, uint64_t hitDelayNS=100 )
  : socket("socket")
  {
    // If this thread is registered, the test method kicks off at time 0
    //SC_THREAD(thread_process);
    m_missDelay = sc_time(missDelayNS, SC_NS);
    m_hitDelay  = sc_time(hitDelayNS,  SC_NS);
  }

  void check_trans_read_good(const char* unittestName, tlm::tlm_generic_payload* trans, uint32_t expdata, sc_time delay, sc_time expdelay)
  {
    if ( trans->is_response_error() ) {
      std::ostringstream oss;
      oss << "Response error from b_transport, " + sc_time_stamp().to_string() ;
      std::string s = oss.str();
      SC_REPORT_ERROR(unittestName, s.c_str() );
    }
    if ( m_data != expdata ) {
      std::ostringstream oss;
      oss << "Wrong data returned, " << m_data << ", " << sc_time_stamp().to_string() ;
      std::string s = oss.str();
      SC_REPORT_ERROR(unittestName, s.c_str() );
    }
    if ( delay != expdelay ) {
      std::ostringstream oss;
      oss << "Wrong delay returned, " << delay.to_string() << ", " << sc_time_stamp().to_string() ;
      std::string s = oss.str();
      SC_REPORT_ERROR(unittestName, s.c_str() );
    }
  }

  void check_trans_write_good(const char* unittestName, tlm::tlm_generic_payload* trans, sc_time delay, sc_time expdelay)
  {
    if ( trans->is_response_error() ) {
      std::ostringstream oss;
      oss << "Response error from b_transport, " + sc_time_stamp().to_string() ;
      std::string s = oss.str();
      SC_REPORT_ERROR(unittestName, s.c_str() );
    }
    if ( delay != expdelay ) {
      std::ostringstream oss;
      oss << "Wrong delay returned, " << delay.to_string() << ", " << sc_time_stamp().to_string() ;
      std::string s = oss.str();
      SC_REPORT_ERROR(unittestName, s.c_str() );
    }
    // confirm that what was written can be read, using the same trans
    trans->set_command( tlm::TLM_READ_COMMAND );
    trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
    delay = sc_time(0, SC_NS);
    int expdata = m_data;
    m_data += 1;
    socket->b_transport( *trans, delay );  // Blocking transport call
    check_trans_read_good(unittestName,trans,expdata,delay,expdelay);
  }

void test_1()
  {
    sc_time testStartTime = sc_time_stamp();
    sc_time delay = sc_time(0, SC_NS);  // var where transaction delays will be accumulated
    //  test_1():
    //    uncached read   (note: adr%8==0 means data returned is 0, and if there is a cache, it wil be a miss)
    //    cached read     (note: adr%8==0 means data returned is 1, and if there is a cache, it wil be a hit)
    //    uncached write
    //    cached write

    tlm::tlm_generic_payload* trans = new tlm::tlm_generic_payload;
    // we're not using these fields/functionalities in this test
    trans->set_data_length( 4 );
    trans->set_streaming_width( 4 ); // = data_length to indicate no streaming
    trans->set_byte_enable_ptr( 0 ); // 0 indicates unused
    trans->set_dmi_allowed( false ); // Mandatory initial value
    // always same data buffer. a member var
    trans->set_data_ptr( reinterpret_cast<unsigned char*>(&m_data) );
    // run our tests

    //-----------------
    const char* unittestName = "test_1.1 0x0 read word 0" ;
    cout << endl << unittestName << endl;
    trans->set_command( tlm::TLM_READ_COMMAND );
    trans->set_address( 0x0 );
    trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
    delay = sc_time(0, SC_NS);
    socket->b_transport( *trans, delay );  // Blocking transport call
    cout << " read address=" << hex << 0x0 << " data=" << m_data << endl;
    check_trans_read_good(unittestName,trans,0,delay,m_missDelay);

    //-----------------
    unittestName = "test_1.1 0x4 read word 1";
    cout << endl << unittestName << endl;
    trans->set_command( tlm::TLM_READ_COMMAND );
    trans->set_address( 0x4 );
    trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
    delay = sc_time(0, SC_NS);
    socket->b_transport( *trans, delay );  // Blocking transport call
    cout << " read address=" << hex << 0x4 << " data=" << m_data << endl;
    check_trans_read_good(unittestName,trans,1,delay,m_hitDelay);

    //-----------------
    unittestName = "test_1.1 0xC read word 3";
    cout << endl  << unittestName << endl;
    trans->set_command( tlm::TLM_READ_COMMAND );
    trans->set_address( 0xC );
    trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
    delay = sc_time(0, SC_NS);
    socket->b_transport( *trans, delay );  // Blocking transport call
    cout << " read address=" << hex << 0xC << " data=" << m_data << endl;
    check_trans_read_good(unittestName,trans,1,delay,m_missDelay);

    //-----------------
    unittestName = "test_1.1 0x10 read word 4";
    cout << endl  << unittestName << endl;
    trans->set_command( tlm::TLM_READ_COMMAND );
    trans->set_address( 0x10 );
    trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
    delay = sc_time(0, SC_NS);
    socket->b_transport( *trans, delay );  // Blocking transport call
    cout << " read address=" << hex << 0x10 << " data=" << m_data << endl;
    check_trans_read_good(unittestName,trans,0,delay,m_missDelay);

    //-----------------
    unittestName = "test_1.3 write  0x0 2";
    cout << endl  << unittestName << endl;
    trans->set_command( tlm::TLM_WRITE_COMMAND );
    trans->set_address( 0x0 );
    trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
    delay = sc_time(0, SC_NS);
    m_data = 2;
    socket->b_transport( *trans, delay );  // Blocking transport call
    cout << " write address=" << hex << 0x0 << " data=" << m_data << endl;
    check_trans_write_good(unittestName,trans,delay,m_hitDelay);

    //-----------------
    unittestName = "test_1.4 write  0x4 3";
    cout << endl  << unittestName << endl;
    trans->set_command( tlm::TLM_WRITE_COMMAND );
    trans->set_address( 0x4 );
    trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
    delay = sc_time(0, SC_NS);
    m_data = 3;
    socket->b_transport( *trans, delay );  // Blocking transport call
    cout << " write address=" << hex << 0x4 << " data=" << m_data << endl;
    check_trans_write_good(unittestName,trans,delay,m_hitDelay);

  }

  void thread_process()
  {
    sc_report_handler::set_actions(SC_ERROR,SC_DISPLAY);
    test_1();
  }

  // Internal data buffer used by initiator with generic payload
  int m_data;
  // support for DMI
  bool dmi_ptr_valid;
  tlm::tlm_dmi dmi_data;

  const int PAGESIZE = 4096 / sizeof(uint32_t); // each memory page will be 4k or 1k 32 bit ints
  SC_HAS_PROCESS(InitiatorTestSimplestMemory);
};

#endif
