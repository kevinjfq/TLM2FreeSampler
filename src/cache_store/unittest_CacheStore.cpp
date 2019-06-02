// Simple file uses "catch2" as a unittest framework for testing CacheStore.
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
#include "CacheStore.h"

using namespace std;

TEST_CASE( "Basic CacheStore initialization", "[CacheStore]" ) {

    CacheStore cs(pow(2,10),pow(2,7),LineSize8,2);

    SECTION( "Some simple bit masking play" ) {
      REQUIRE( ((0xFFFFFFFFFFFFFFFF >> 3) << 3) == 0xFFFFFFFFFFFFFFF8 );
      REQUIRE( 0 == cs.getWordIndex(0x0) );
      REQUIRE( 1 == cs.getWordIndex(0x4) );
      REQUIRE( 0 == cs.getWordIndex(0x8) );
      REQUIRE( 1 == cs.getWordIndex(0xC) );

      REQUIRE( 1 == cs.getWordIndex(0xFFFF) );
      REQUIRE( 0 == cs.getWordIndex(0xFFFF+1) );

      REQUIRE( 0 == cs.getByteIndex(0xFFF8) );
      REQUIRE( 1 == cs.getByteIndex(0xFFF9) );
      REQUIRE( 2 == cs.getByteIndex(0xFFFA) );
      REQUIRE( 3 == cs.getByteIndex(0xFFFB) );
      REQUIRE( 4 == cs.getByteIndex(0xFFFC) );
      REQUIRE( 5 == cs.getByteIndex(0xFFFD) );
      REQUIRE( 6 == cs.getByteIndex(0xFFFE) );
      REQUIRE( 7 == cs.getByteIndex(0xFFFF) );
    }
    SECTION( "Check default ctor" ) {
      REQUIRE( cs.p_MemorySize == pow(2,10) );
      REQUIRE( cs.p_CacheSize == pow(2,7) );
      REQUIRE( cs.p_LineSize == 8 );
      REQUIRE( cs.p_NumWays == 2 );
    }
    SECTION( "Check getCacheBlockIndex" ) {
      REQUIRE( cs.getCacheBlockIndex(0b00000000) == 0 ); // 0
      REQUIRE( cs.getCacheBlockIndex(0b00001000) == 1 ); // 16
      REQUIRE( cs.getCacheBlockIndex(0b00010000) == 2 ); // 32
      REQUIRE( cs.getCacheBlockIndex(0b00011000) == 3 ); // 48
      REQUIRE( cs.getCacheBlockIndex(0b00100000) == 4 ); // 48
      REQUIRE( cs.getCacheBlockIndex(0b00111000) == 7 );
      REQUIRE( cs.getCacheBlockIndex(0b01000000) == 0 );
      REQUIRE( cs.getCacheBlockIndex(0b01001000) == 1 );
      REQUIRE( cs.getCacheBlockIndex(0b01111000) == 7 );
      REQUIRE( cs.getCacheBlockIndex(0b10000000) == 0 );
    }
    SECTION( "Check getCacheTag" ) {
      // for the sizes above, getCacheTag left shifts by 6
      REQUIRE( cs.getCacheTag(0b00100000) == 0b0 ); // 0
      REQUIRE( cs.getCacheTag(0b01000000) == 0b1 ); // 0
      REQUIRE( cs.getCacheTag(0b10000000) == 0b10 ); // 0
      // same in decimal
      REQUIRE( cs.getCacheTag(1<<5) == 0 );
      REQUIRE( cs.getCacheTag(2<<5) == 1 );
      REQUIRE( cs.getCacheTag(3<<5) == 1 );
      REQUIRE( cs.getCacheTag(4<<5) == 2 );
      REQUIRE( cs.getCacheTag(5<<5) == 2 );
      REQUIRE( cs.getCacheTag(6<<5) == 3 );
      REQUIRE( cs.getCacheTag(7<<5) == 3 );
      REQUIRE( cs.getCacheTag(8<<5) == 4 );
      REQUIRE( cs.getCacheTag(10<<5) == 5 );

      REQUIRE( cs.getCacheTag(1<<6) == 1 );
      REQUIRE( cs.getCacheTag(2<<6) == 2 );
      REQUIRE( cs.getCacheTag(3<<6) == 3 );
      REQUIRE( cs.getCacheTag(777<<6) == 777 );
      REQUIRE( cs.getCacheTag(3<<7) == 6 );
    }
}

// Experimneting with JSON as a way to mock class instances.
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <iostream>
using namespace rapidjson;
#include "CacheLineJSON.h"

class CacheStoreJSON : public CacheStore
{
public:
  static CacheStoreJSON* createCacheStoreJSON(const char* jsonCacheStore)
  {
    Document d;
    d.Parse(jsonCacheStore);
    uint64_t memorySize = (uint64_t)d["memorySize"].GetInt();
    uint64_t cacheSize = (uint64_t)d["cacheSize"].GetInt();
    uint64_t lineSize = (uint64_t)d["lineSize"].GetInt();
    uint64_t numWays = (uint64_t)d["numWays"].GetInt();
    const Value& arrayCacheLines = d["cacheLines"]; // Using a reference for consecutive access is handy and faster.
    assert(arrayCacheLines.IsArray());
    return new CacheStoreJSON(memorySize,cacheSize,lineSize,numWays,arrayCacheLines);
  }
  CacheStoreJSON(
        uint64_t memorySize,
        uint64_t cacheSize,
        uint64_t lineSize,
        uint64_t numWays,
        const Value& arrayCacheLines
      )
  : CacheStore( memorySize,cacheSize,lineSize,numWays)
  {
    for (SizeType ii = 0; ii < arrayCacheLines.Size(); ii++) { // rapidjson type 'SizeType'
      const Value& cacheLine = arrayCacheLines[ii];
      rapidjson::StringBuffer buffer;
      buffer.Clear();
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      cacheLine.Accept(writer);
      string s = buffer.GetString();
      CacheLineJSON clj((CacheLineSize_t)lineSize,s.c_str());
      this->m_cacheLines[ii] = clj; // TODO: what is happening here? default assignment operator???
    }
  }
  bool dump(int numCacheLines) {
    cout << "Dumping CacheStore for " << numCacheLines << " lines." << endl;
    for (int jj = 0; jj < numCacheLines; jj++) { // rapidjson type 'SizeType'
      cout << jj << " : ";
      cout << this->m_cacheLines[jj].getTag() << ", ";
      cout << this->m_cacheLines[jj].getValid() << ", ";
      uint32_t* dataPtr = this->m_cacheLines[jj].getData(0) ;
      cout << "[";
      for (size_t i = 0; i < 4; i++) { cout << dataPtr[i] << ",";  }
      cout << "]" << endl;
    }
    return true;
  }
};

TEST_CASE( "CacheStore getCacheBlock & getCacheLine", "[CacheStore]" ) {
  const char jsonCacheStore[] =
                      "{  \"memorySize\" : 1024, \"cacheSize\" : 128, \"lineSize\" : 16, \"numWays\" : 2, \
                        \"cacheLines\":[ \
                        { \"lineSize\" : 16, \"isValid\" : true , \"tag\" : 68, \"data\":[1, 3, 5, 7] }, \
                        { \"lineSize\" : 16, \"isValid\" : true , \"tag\" : 72, \"data\":[11, 13, 15, 17] }, \
                        { \"lineSize\" : 16, \"isValid\" : false , \"tag\" : 76, \"data\":[21, 23, 25, 27] } \
                        ] \
                      } ";
/*                      Document d;
                      d.Parse(jsonCacheStore);
                      uint64_t memorySize = (uint64_t)d["memorySize"].GetInt();
                      uint64_t cacheSize = (uint64_t)d["cacheSize"].GetInt();
                      uint64_t lineSize = (uint64_t)d["lineSize"].GetInt();
                      uint64_t numWays = (uint64_t)d["numWays"].GetInt();
                      const Value& arrayCacheLines = d["cacheLines"]; // Using a reference for consecutive access is handy and faster.
                      assert(arrayCacheLines.IsArray());
                      for (SizeType ii = 0; ii < arrayCacheLines.Size(); ii++) { // rapidjson type 'SizeType'
                        const Value& cacheLine = arrayCacheLines[ii];
                      }
                      cout <<endl;
*/

  CacheStoreJSON* csj = CacheStoreJSON::createCacheStoreJSON( jsonCacheStore );
  //CacheStore cs2(pow(2,10),pow(2,7),LineSize8,2);

  SECTION( "Check getCacheBlock" ) {
    csj->dump(3);
    // for the sizes above, getCacheTag left shifts by 6
    REQUIRE( csj->getCacheBlock(0)->getValid() == true ); // 0
    REQUIRE( csj->getCacheBlock(0)->getTag() == 68 ); // 0

    REQUIRE( csj->getCacheBlock(1)->getValid() == false ); // 0
    REQUIRE( csj->getCacheBlock(1)->getTag() == 76 ); // 0

  }
/*
  SECTION( "Check getCacheBlockIndex" ) {
    REQUIRE( cs2.getCacheBlockIndex(0x0) == 0 ); // 0
    REQUIRE( csj->getCacheBlockIndex(0x0) == 0 ); // 0
    uint64_t cbIndex = csj->getCacheBlockIndex(0x0);
    REQUIRE( cbIndex == 0 );

  }
*/
  SECTION( "Check getCacheLine 0x00001100 -> cache line #0 (block #0) with tag 68" ) {
    uint64_t adr = 0x00001100;
    //cout << "adr=" << hex << adr << " " << dec << csj->getCacheTag(adr) << " " << csj->getCacheBlockIndex(adr) <<endl;
    uint64_t cbIndex = csj->getCacheBlockIndex(adr); // the first cache line
    REQUIRE( cbIndex == 0 );
    CacheLine* cb = csj->getCacheBlock( cbIndex );
    REQUIRE( cb != NULL );
    CacheLine* cl = csj->getCacheLine(adr);
    REQUIRE( cl != NULL );
    REQUIRE( cl->getTag() == 68 );
    REQUIRE( cl->getData(0) != NULL );
    REQUIRE( cl->getValid() == true );

    //uint64_t adr = 0x00001200;
    //cout << "adr=" << hex << adr << " " << dec << csj->getCacheTag(adr) << " " << csj->getCacheBlockIndex(adr) <<endl;
  }
  SECTION( "Check getCacheLine 0x00001310 ->  cache line #2 (block #1) with tag 76" ) {
    uint64_t adr = 0x00001310;
    //cout << "adr=" << hex << adr << " " << dec << csj->getCacheTag(adr) << " " << csj->getCacheBlockIndex(adr) <<endl;
    uint64_t cbIndex = csj->getCacheBlockIndex(adr); // the first cache line
    REQUIRE( cbIndex == 1 );
    CacheLine* cb = csj->getCacheBlock( cbIndex );
    REQUIRE( cb != NULL );
    CacheLine* cl = csj->getCacheLine(adr);
    REQUIRE( cl != NULL );
    REQUIRE( cl->getTag() == 76 );
    REQUIRE( cl->getData(0) != NULL );
    REQUIRE( cl->getValid() == false );
  }
  SECTION( "Check invalidate 0x00001100 -> cache line #0 (block #0) with tag 68" ) {
    uint64_t adr = 0x00001100;
    CacheLine* cl = csj->getCacheLine(adr);
    REQUIRE( cl->getValid() == true );
    csj->invalidate(adr);
    CacheLine* cl2 = csj->getCacheLine(adr);
    REQUIRE( cl2->getValid() == false );
  }
}
TEST_CASE( "CacheStore newCacheLine", "[CacheStore]" ) {

    CacheStore cs(pow(2,10),pow(2,7),LineSize8,2);

    // These addresses all map to the same cacheBlockIndex 0
    // 0x00000000
    // 0x00010000
    // 0x00020000
    // 0x00030000
    SECTION( "Asking for a sequence of new cachelines" ) {
      uint64_t adr1 = 0x00000000;
      uint64_t adr2 = 0x00010000;
      uint64_t adr3 = 0x00020000;
      uint64_t adr4 = 0x00030000;
      uint32_t dmydata[8] = {0,1,2,3,4,5,6,7} ;

      cout << "adr1=" << hex << adr1 << " tag=" << dec << cs.getCacheTag(adr1) << " block" << cs.getCacheBlockIndex(adr1) <<endl;
      cout << "adr2=" << hex << adr2 << " tag=" << dec << cs.getCacheTag(adr2) << " block" << cs.getCacheBlockIndex(adr2) <<endl;
      cout << "adr3=" << hex << adr3 << " tag=" << dec << cs.getCacheTag(adr3) << " block" << cs.getCacheBlockIndex(adr3) <<endl;
      cout << "adr4=" << hex << adr4 << " tag=" << dec << cs.getCacheTag(adr4) << " block" << cs.getCacheBlockIndex(adr4) <<endl;

      CacheLine* cl_1 = cs.newCacheLine(adr1);
        REQUIRE(cl_1->getEmpty() == true);
        cl_1->load( cs.getCacheTag(adr1), dmydata, LineSize8);
        REQUIRE(cl_1->getEmpty() == false);
      CacheLine* cl_2 = cs.newCacheLine(adr2);
        REQUIRE(cl_2->getEmpty()  == true);
        cl_2->load( cs.getCacheTag(adr2), dmydata, LineSize8);
        REQUIRE(cl_2->getEmpty() == false);

      REQUIRE( cl_1 != cl_2 );
      REQUIRE( cl_1->getTag() == 0 );
      REQUIRE( cl_2->getTag() == 1024 );

      CacheLine* cl_3 = cs.newCacheLine(adr3);
        REQUIRE(cl_3->getEmpty() == true);
        cl_3->load( cs.getCacheTag(adr3), dmydata, LineSize8);
        REQUIRE(cl_3->getEmpty() == false);
        REQUIRE( cl_3 == cl_1 );
        REQUIRE( cl_3->getTag() == 2048 );

      CacheLine* cl_4 = cs.newCacheLine(adr4);
        REQUIRE(cl_4->getEmpty() == true);
        cl_4->load( cs.getCacheTag(adr4), dmydata, LineSize8);
        REQUIRE(cl_4->getEmpty() == false);
        REQUIRE( cl_4->getTag() == 3072 );
        // This will fail until I implement LRU eviction rule for CacheLines within a block
        CHECK_NOFAIL( cl_4 == cl_2 );
    }
}

TEST_CASE( "CacheStore get/setCacheLine", "[CacheStore]" ) {

    CacheStore cs(pow(2,10),pow(2,7),LineSize8,2);

    // These addresses all map to the same cacheBlockIndex 0
    // 0x00000000
    // 0x00010000
    // 0x00020000
    // 0x00030000
    SECTION( "Asking for a sequence of new cachelines" ) {
      uint64_t adr1 = 0x00000000;
      uint64_t adr2 = 0x00010000;
      uint64_t adr3 = 0x00020000;
      uint64_t adr4 = 0x00030000;
      uint32_t dmydata1[2] = {0,1} ;
      uint32_t dmydata2[2] = {10,11} ;
      uint32_t dataout[2] = {0,0} ;

      uint8_t dmybytes1[8] = {0,1,2,3,4,5,6,7} ;
      uint8_t dmybytes2[8] = {10,11,12,13,14,15,16,17} ;
      uint8_t bytesout[8] = {0,0,0,0,0,0,0,0} ;

      bool bHit;
      bHit = cs.setDataLine(adr1, dmydata1);
      REQUIRE(bHit == false);
      bHit = cs.getDataLine(adr1, dataout);
      REQUIRE(bHit == true);
      REQUIRE(dmydata1[0] == dataout[0]);
      REQUIRE(dmydata1[1] == dataout[1]);

      bHit = cs.setDataLine(adr2, dmydata2);
      REQUIRE(bHit == false);
      bHit = cs.getDataLine(adr2, dataout);
      REQUIRE(bHit == true);
      REQUIRE(dmydata2[0] == dataout[0]);
      REQUIRE(dmydata2[1] == dataout[1]);

      dmydata2[1] = 17;
      bHit = cs.setDataLine(adr1, dmydata2);
      REQUIRE(bHit == true);
      bHit = cs.getDataLine(adr1, dataout);
      REQUIRE(bHit == true);
      REQUIRE(dmydata2[0] == dataout[0]);
      REQUIRE(dmydata2[1] == dataout[1]);

      bHit = cs.setDataLine(adr2, reinterpret_cast<uint32_t*>(dmybytes2) );
      REQUIRE(bHit == true);
      bHit = cs.getDataLine(adr2, reinterpret_cast<uint32_t*>(bytesout) );
      REQUIRE(bHit == true);
      REQUIRE(dmybytes2[0] == bytesout[0]);
      REQUIRE(dmybytes2[3] == bytesout[3]);
      REQUIRE(dmybytes2[7] == bytesout[7]);

      bHit = cs.getDataLine(adr3, dataout);
      REQUIRE(bHit == false);


    }
}

/*

virtual CacheLine* getCacheBlock( uint64_t cacheBlockIndex )
{
  return &m_cacheLines[cacheBlockIndex * p_NumWays];
}


      SECTION( "Check default state: nothing cached" ) {
      REQUIRE( cs.isWordCached(0x0) == false );
      REQUIRE( cs.isWordCached(0x1000) == false );
      REQUIRE( cs.getLineAddress(0xFFFFFFFFFFFFFFFF) == 0xFFFFFFFFFFFFFFF8 );
    }
    SECTION( "Check cacheing to an address that isnt the Start of a CacheLine." ) {
      //  virtual bool copyToCacheLine( uint32_t* toCache, uint32_t* fromBuffer, int len, uint32_t** restOfBuffer, int* lenRemaining )
      uint32_t data1[8] = {0,1,2,3,4,5,6,7} ;
      uint32_t* restOfData1 = NULL;
      int lenRemaining = 0;
      REQUIRE_THROWS( cs.copyToCacheLine(0x777,data1,2,&restOfData1,&lenRemaining) == true );
    }
    SECTION( "Check cacheing a line of data that fits" ) {
      //  virtual bool copyToCacheLine( uint32_t* toCache, uint32_t* fromBuffer, int len, uint32_t** restOfBuffer, int* lenRemaining )
      uint32_t data1[8] = {0,1,2,3,4,5,6,7} ;
      uint32_t* restOfData1 = NULL;
      int lenRemaining = 0;
      REQUIRE( cs.copyToCacheLine(0x0,data1,2,&restOfData1,&lenRemaining) == true );
      REQUIRE( restOfData1 == NULL);
      REQUIRE( lenRemaining == 0 );
    }
    SECTION( "Check cacheing a line of data that does NOT fit" ) {
      //  virtual bool copyToCacheLine( uint32_t* toCache, uint32_t* fromBuffer, int len, uint32_t** restOfBuffer, int* lenRemaining )
      uint32_t data1[16] = {0,1,2,3,4,5,6,7, 0,1,2,3,4,5,6,7} ;
      uint32_t* restOfData1 = NULL;
      int lenRemaining = 0;
      REQUIRE( cs.copyToCacheLine(0x0,data1,16,&restOfData1,&lenRemaining) == false );
      REQUIRE( restOfData1 == &data1[8] );
      REQUIRE( lenRemaining == 8 );
    }
    */
