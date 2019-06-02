// Created this to insert between the initiator and memory of the Doulos TLM Training example #1.
// Does not currently implement any data sotorage itself, but parasitically uses the same
// data space as the memory instance (which had to be enhanced to provide a ptr to that space.)
// This model simply uses rand() to model when a cache miss might have happened and then
// adds an additional cache miss penalty to the default delay.

#ifndef CacheLine_H
#define CacheLine_H

#include <math.h>       // pow
#include <limits.h>

unsigned int logbase2 (unsigned int num) {
  unsigned int retVal = 0;
  //if (ii == 0) return UINT_MAX; // should throw runtime exception
  if (num == 0) throw std::runtime_error("logbase2: Cannot take log of 0.");
  if (num == 1) return 0;
  while (num > 1) {
      num >>= 1;
      retVal++;
  }
  return retVal;
}

// Size (B) of a Cache Line and of a cacheable chunk of memory. The LineSize has to be a power of 2.
enum CacheLineSize_t {LineSize2=2, LineSize4=4 , LineSize8=8 , LineSize16=16 , LineSize32=32, LineSize64=64, LineSize128=128, LineSize256=256, LineSize512=512, LineSize1024=1024 };

class CacheLine
{
  static const uint64_t EMPTY = 0-1;
protected:
  bool                isValid;
  uint64_t		        tag;
  uint32_t*           data;
public:
  CacheLine()
  : isValid(false)
  , tag(CacheLine::EMPTY)  // a maxint that can never be the value of a real cache tag
  , data(0)
  {}

  void load(uint64_t _tag, const uint32_t* line, CacheLineSize_t lineSize) {
    if( data == NULL ) {
      data = new uint32_t[lineSize];
    }
    for( int ii=0; ii < lineSize; ii++) { data[ii]=line[ii];}
    tag = _tag;
    isValid = true;
  }

  void setValid(bool _valid) {
    isValid = _valid;
  }

  bool getValid() {
    return isValid;
  }

  void setEmpty() {
    tag = CacheLine::EMPTY;
  }

  bool getEmpty() {
    return (tag == CacheLine::EMPTY);
  }

  uint64_t getTag() {
    return tag;
  }

  uint32_t* getData(unsigned int index) {
    // assert(isValid)
    // assert( index < p_LineSize )
    return &data[index];
  }

  void invalidate() {
    isValid = false;
  }
};
#endif
