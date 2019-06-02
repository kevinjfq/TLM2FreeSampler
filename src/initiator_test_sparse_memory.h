#ifndef InitiatorTestMemorySparse_h
#define InitiatorTestMemorySparse_h

// Test Initiator class
// This is just for SystemC "unit" testing (though since its systemc, it's really a kind of integration test).
// This works in conjunction with SparseMemory .
// Tests:
// - short sequence of read and write transactions and checks the results.
// - some transactions to test the loading of >1 pages of memory into SparseMemory
// - simple test of DMI
// - simple test of transport_dbg


#include <string>
#include <sstream>
#include "systemc"
//#include <sc_report.h>
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"

// InitiatorTestSparseMemory module generating generic payload transactions

struct InitiatorTestSparseMemory: sc_module
{
  // TLM-2 socket, defaults to 32-bits wide, base protocol
  tlm_utils::simple_initiator_socket<InitiatorTestSparseMemory> socket;

  SC_CTOR(InitiatorTestSparseMemory)
  : socket("socket")  // Construct and name socket
  {
    // If this thread is registered, the test method kicks off at time 0
    //SC_THREAD(thread_process);
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
    check_trans_read_good(unittestName,trans,expdata,delay,sc_time(100, SC_NS));
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
    check_trans_read_good(unittestName,trans,0,delay,sc_time(100, SC_NS));

    //-----------------
    unittestName = "test_1.1 0x4 read word 1";
    cout << endl << unittestName << endl;
    trans->set_command( tlm::TLM_READ_COMMAND );
    trans->set_address( 0x4 );
    trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
    delay = sc_time(0, SC_NS);
    socket->b_transport( *trans, delay );  // Blocking transport call
    cout << " read address=" << hex << 0x4 << " data=" << m_data << endl;
    check_trans_read_good(unittestName,trans,1,delay,sc_time(100, SC_NS));

    //-----------------
    unittestName = "test_1.1 0xC read word 3";
    cout << endl  << unittestName << endl;
    trans->set_command( tlm::TLM_READ_COMMAND );
    trans->set_address( 0xC );
    trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
    delay = sc_time(0, SC_NS);
    socket->b_transport( *trans, delay );  // Blocking transport call
    cout << " read address=" << hex << 0xC << " data=" << m_data << endl;
    check_trans_read_good(unittestName,trans,1,delay,sc_time(100, SC_NS));

    //-----------------
    unittestName = "test_1.1 0x10 read word 4";
    cout << endl  << unittestName << endl;
    trans->set_command( tlm::TLM_READ_COMMAND );
    trans->set_address( 0x10 );
    trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
    delay = sc_time(0, SC_NS);
    socket->b_transport( *trans, delay );  // Blocking transport call
    cout << " read address=" << hex << 0x10 << " data=" << m_data << endl;
    check_trans_read_good(unittestName,trans,0,delay,sc_time(100, SC_NS));

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
    check_trans_write_good(unittestName,trans,delay,sc_time(100, SC_NS));

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
    check_trans_write_good(unittestName,trans,delay,sc_time(100, SC_NS));

    //-----------------
    unittestName = "test_1.5 write/access a different page";
    cout << endl  << unittestName << endl;
    trans->set_command( tlm::TLM_WRITE_COMMAND );
    //cout << dec << (PAGESIZE * 2) + sizeof(uint32_t) << " vs " << 0x2004 << endl;
    // There are 1024 words per page. We want to ask for a word From
    // an unloaded page #2 but at an odd offset, ie 1.
    // So the wordIndex we want is ((PAGESIZE * 2) + 1)
    // And so the address to that word is wordIndex * 4 (the word size).
    sc_dt::uint64 adr = ((PAGESIZE * 2) + 1) * sizeof(uint32_t);
    trans->set_address( adr );
    //cout << "qqq adr dec" << dec << adr  << "  adr hex" << hex << adr << endl;
    trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
    delay = sc_time(0, SC_NS);
    m_data = 'Z';
    socket->b_transport( *trans, delay );  // Blocking transport call
    cout << " write address=" << hex << adr << " data=" << m_data << endl;
    check_trans_write_good(unittestName,trans,delay,sc_time(100, SC_NS));

    //-----------------
    unittestName = "test_1.6 check DMI hint";
    cout << endl  << unittestName << endl;
    // Check DMI hint
    if ( !trans->is_dmi_allowed() ) {
      SC_REPORT_ERROR(unittestName, "Expected DMI to be allowed." );
    }
    // Re-use transaction object for DMI
    dmi_data.init();
    dmi_ptr_valid = socket->get_direct_mem_ptr( *trans, dmi_data );
    uint32_t* page = reinterpret_cast<uint32_t*>( dmi_data.get_dmi_ptr() );
    uint32_t offset = sizeof(uint32_t) / 4;
    // check the last value we wrote is in the DMI page returned
    check_trans_read_good(unittestName,trans,page[offset],delay,sc_time(100, SC_NS));
    if ( !dmi_data.is_read_allowed() ) {
      SC_REPORT_ERROR(unittestName, "Expected DMI is_read_allowed()." );
    }
    if ( !dmi_data.is_write_allowed() ) {
      SC_REPORT_ERROR(unittestName, "Expected DMI is_write_allowed()." );
    }
    if ( dmi_data.get_read_latency() != sc_time(10, SC_NS) ) {
      SC_REPORT_ERROR(unittestName, "Expected DMI get_read_latency() == 10." );
    }
    if ( dmi_data.get_write_latency() != sc_time(10, SC_NS) ) {
      SC_REPORT_ERROR(unittestName, "Expected DMI get_write_latency() == 10." );
    }

    //-----------------
    unittestName = "test_1.7 transport_dbg";
    cout << endl  << unittestName << endl;
    trans->set_command( tlm::TLM_WRITE_COMMAND );
    trans->set_address( 0x4 );
    trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value
    delay = sc_time(0, SC_NS);
    sc_time unittestStartTime = sc_time_stamp();
    socket->transport_dbg( *trans );  // Blocking transport call
    sc_time unittestEndTime = sc_time_stamp();
    // Initiator obliged to check response status and delay
    if ( trans->is_response_error() ) {
      std::ostringstream oss;
      oss << "Response error from transport_dbg, " + sc_time_stamp().to_string() ;
      std::string s = oss.str();
      SC_REPORT_ERROR(unittestName, s.c_str() );
    }
    if ( unittestStartTime != unittestEndTime ) {
      std::ostringstream oss;
      oss << "Failure: sc_time should not have advanced during transport_dbg. " ;
      std::string s = oss.str();
      SC_REPORT_ERROR(unittestName, s.c_str() );
    }

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
};

#endif
