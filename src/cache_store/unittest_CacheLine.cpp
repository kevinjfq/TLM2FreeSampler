// Simple file uses "catch2" as a unittest framework for testing CacheLine.
// First TestCase directly tests the CacheLine class.
// Then, RapidJSON is used in second TestCase to initialize 'mock' instances of CacheLine.
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
#include "CacheLine.h"

using namespace std;

TEST_CASE( "Basic CacheLine initialization", "[CacheLine]" ) {

    CacheLine cl;
    uint32_t data1[8] = {0,1,2,3,4,5,6,7} ;

    REQUIRE( cl.getValid() == false );
    REQUIRE( cl.getEmpty() == true );
    REQUIRE( cl.getTag() == ((uint64_t)0)-1 );

    SECTION( "loading a line makes it valid" ) {
        cl.load( 1111, data1, LineSize4 );
        REQUIRE( cl.getValid() == true );
        REQUIRE( cl.getTag() == 1111 );
    }
    SECTION( "loading and invalidating line makes it invalid" ) {
        cl.load( 1111, data1, LineSize8 );
        cl.invalidate();
        REQUIRE( cl.getValid() == false );
        REQUIRE( cl.getTag() == 1111 );
    }
    SECTION( "loading and getting gets correct value" ) {
        cl.load( 1111, data1, LineSize16 );
        REQUIRE( *(cl.getData(0)) == 0 );
        REQUIRE( *(cl.getData(4)) == 4 );
        REQUIRE( *(cl.getData(7)) == 7 );
    }
  }

// Experimneting with JSON as a way to mock class instances.
#include "CacheLineJSON.h"

  TEST_CASE( "CacheLineJSON initialization", "[CacheLineJSON]" ) {
      const char json1[] = " { \"lineSize\" : 16, \"isValid\" : true , \"tag\" : 222, \"data\":[1, 3, 5, 7] } ";

      CacheLineJSON clj(LineSize8,json1);

      SECTION( "initializaing via JSON" ) {
          REQUIRE( clj.getValid() == true );
          REQUIRE( clj.getTag() == 222 );
          //REQUIRE( clj.dump() == true );
          REQUIRE( *(clj.getData(0)) == 1 );
          REQUIRE( *(clj.getData(1)) == 3 );
          REQUIRE( *(clj.getData(2)) == 5 );
          REQUIRE( *(clj.getData(3)) == 7 );
          REQUIRE( *(clj.getData(4)) == 0 );
          REQUIRE( *(clj.getData(7)) == 0 );
          REQUIRE( *(clj.getData(15)) == 0 );
      }
  }
