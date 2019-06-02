// This is a simple Cache Storage class supporting cache set/get of datalines.
// HIgh-level API to get/set lines of data
//  - getDataLine(adr,dataline* toBuffer) - reads line of cached data into buffer; if not in cache, returns false
//  - setDataLine(adr,dataline* fromBuffer) - writes dataline into cache from buffer, if not in cache, caches it
// Both return true if it was a cache hit, false if miss.
//  - invalidate(adr) - invalidates cache entry if it exists
//
// Configurable parameters - on constructor; all powers of 2
// 	p_MemorySize;		// Size (B) of the Memory
//	p_CacheSize;		// Size (B) of the Cache
//	p_LineSize;			// Size (B) of a Cache Line and of a cacheable chunk of memory
//	p_NumWays;	    // number of 'ways' per cache block

#ifndef CacheStore_H
#define CacheStore_H

#include <math.h>       // pow
#include <limits.h>
#include <sstream>
#include <vector>
#include <algorithm>    // copy

#include "CacheLine.h"

using namespace std;

struct CacheStore
{
  // Ideas for future API enhancements:
  // abstract interface:
  //  bool isWordCached(wordAddress)
  //  word* accessWord(wordAddress, lengthToEnd*)
  // helpful utilities:
  //  lineAddress getLineAddress(wordAddress)
  //  bool copyToCacheLine(word*, word*, length, lenCopied*)
  //  bool copyFromCacheLine(word*, word*, length, lenCopied*)
  // even-higher-level interface...probably more for a CacheController interface
  //  buffer getData(address, length )

  //-------------------------
  // Configurable parameters
public:
  uint64_t	p_MemorySize;		// Size (B) of the Memory
  uint64_t	p_CacheSize;		// Size (B) of the Cache
  uint64_t	p_LineSize;			// Size (B) of a Cache Line and of a cacheable chunk of memory
  uint64_t	p_NumWays;		    // number of 'ways' per cache block

protected:
  //CacheLine *m_cacheLines;
  vector<CacheLine> m_cacheLines;

  uint64_t m_numMemoryLines;
  uint64_t m_numCacheLines;
  uint64_t m_numCacheBlocks;          // In my mind, a Cache Block is like a cache hash bucket in that multiple Lines of
                                      //  Memory can/will end up mapping to the same cache block.
  uint64_t m_cacheBlockIndexMask;
  uint64_t m_bitsForLine;
  uint64_t m_bitShiftForCacheTag;

public:
  CacheStore(uint64_t memorySize=pow(2,30), uint64_t cacheSize=pow(2,20), uint64_t lineSize=pow(2,3), uint64_t numWays=pow(2,0) )
  : p_MemorySize( memorySize )				// 1 GiB
  , p_CacheSize(  cacheSize )				  // 1 MiB
  , p_LineSize(   lineSize )					// 8 words by default; i.e., one Line contains 8 data words
  , p_NumWays(    numWays  )					// 1 way by default; i.e., one Line per Cache Block
  , m_numMemoryLines(  p_MemorySize/ p_LineSize )   //
  , m_numCacheLines(   p_CacheSize / p_LineSize )   //
  , m_numCacheBlocks(  (p_CacheSize / p_LineSize)/p_NumWays)
  , m_cacheLines( p_CacheSize / p_LineSize )        // This pre-allocates vector of CacheLine and calls  default ctor for each.
                                                    // I like this b/c I immediately have all CacheLine instances existing and indexable.
  {
    // TODO: assert that Memory, Cache, Line sizes and NumWays are all reasonable numbers
    //cout
    //<< " p_MemorySize=" <<p_MemorySize
    //<< " p_CacheSize=" <<p_CacheSize
    //<< " p_LineSize=" <<p_LineSize
    //<< " p_NumWays=" <<p_NumWays
    //<< endl;

    // How many bits needed to address a line? (We will often shift these away and ignore.)
    m_bitsForLine = logbase2( p_LineSize );

    // Q: So how do you calculate which Cache Block a memory line maps to? I.e., what is the hash function?
    // A: you use the lowest N bits of the memory address (actually memory line index).
    // Calculate how many bits are needed to distnguish NUM_BLOCKS blocks.
    uint64_t cacheBlockIndex_NumBits = logbase2( m_numCacheBlocks );
    m_cacheBlockIndexMask = pow(2,cacheBlockIndex_NumBits)-1; // i.e. 10 1's or 1111111111

    // While the lowest bits index inside a Line and the next N bits are needed to determine the cacheBlock,
    // the rest of the bits (highest) are used as a tag to distinguish which memoryLine is actually cached.
    // To get the highest N bits for the cacheTag, we will just shift the memoryAddress by m_bitShiftForCacheTag.
    m_bitShiftForCacheTag = ( cacheBlockIndex_NumBits + m_bitsForLine );

    // not needed
    //uint64_t memoryLineIndex_NumBits = logbase2( m_numMemoryLines );
    //uint64_t memoryAddress_NumBits = 64; // = sizeof(uint64_t) * 8 bits pre byte

  }

  // HIgh-level API to get/set lines of data
  //  - getDataLine(adr,dataline* toBuffer) - reads line of cached data into buffer; if not in cache, returns false
  //  - setDataLine(adr,dataline* fromBuffer) - writes dataline into cache from buffer, if not in cache, caches it
  // Both return true if it was a cache hit, false if miss.
  //  - invalidate(adr) - invalidates cache entry if it exists
  virtual bool getDataLine( uint64_t memoryAddress, uint32_t* toBuffer )
  {
    if( toBuffer == NULL ) return false;    // should throw an exception

    CacheLine* cl = getCacheLine( memoryAddress );
    if( cl == NULL ) return false;          // data is not not in cache

    uint32_t* fromBuffer  = cl->getData(0);
    std::copy(fromBuffer, fromBuffer+(p_LineSize/sizeof(uint32_t)), toBuffer );
    return true;
  }

  virtual bool setDataLine( uint64_t memoryAddress, const uint32_t* fromBuffer )
  {
    if( fromBuffer == NULL ) {}    // should throw an exception
    bool cacheHit = true;

    CacheLine* cl = getCacheLine( memoryAddress );
    if( cl == NULL ) {          // data is not not in cache
      cl = newCacheLine( memoryAddress );
      cacheHit = false;
    }
    cl->load( getCacheTag(memoryAddress),fromBuffer ,(CacheLineSize_t)p_LineSize);
    return cacheHit;
  }
  virtual void invalidate( uint64_t memoryAddress )
  {
    CacheLine*  cl = this->getCacheLine(memoryAddress);
    if ( cl != NULL ) cl->setValid(false);
  }

  // API for dealing with CacheLine objects
  //  - getCacheLine(adr) - returns NULL or CacheLine
  //  - newCacheLine(adr) - creates and returns new CacheLine
  //  - invalidate(adr) - invalidate corresponding CacheLine, if the Cache contains it
  virtual CacheLine*  getCacheLine( uint64_t memoryAddress )
  {
    // Note: the cacheBlock is a chunk from an array of CacheLines. Really just a ptr into an array of CacheLines
    CacheLine* cacheBlock = this->getCacheBlock(getCacheBlockIndex(memoryAddress));

    // Since multiple Memory lines will map to the same cache block (cache hash bucket),
    // we need a key/tag to easily distinguish the multiple bucket entries. We simply use
    // the rest of the bits of the memoryLineIndex:
	  // The upper (m - k) address bits are stored as the tag of each CacheLine.

    //uint64_t cacheTag = memoryLineIndex | m_cacheTagMask; // kq: doesnt seem right...shift?
    // To get the highest N bits for the cacheTag, we just shift the memoryAddress by m_bitShiftForCacheTag
    uint64_t cacheTag = getCacheTag(memoryAddress);
    // check if any of the Ways match this tag
    for( int ii = 0; ii < p_NumWays; ii++) {
      CacheLine* returnCacheLine = &cacheBlock[ii];
      if( returnCacheLine->getTag() == cacheTag ) return returnCacheLine;
    }
    return NULL;
  }
  virtual CacheLine*  newCacheLine( uint64_t memoryAddress )
  {
    // Note: the cacheBlock is a chunk from an array of CacheLines. Really just a ptr into an array of CacheLines
    CacheLine* cacheBlock = this->getCacheBlock(getCacheBlockIndex(memoryAddress));
    // Now we have to evict one of the CacheLines.
    CacheLine* newLine = this->pickOrEvict(cacheBlock);
    // don't bother re-initializing the evictLine, just setEmpty()
    newLine->setEmpty();
    return newLine;
  }
  virtual CacheLine*  pickOrEvict( CacheLine* cacheBlock )
  {
    CacheLine* returnCacheLine = NULL;
    // If one is empty, return it.
    for( int ii = 0; ii < p_NumWays; ii++) {
      returnCacheLine = &cacheBlock[ii];
      if( returnCacheLine->getEmpty()) return returnCacheLine;
    }
    // Return the oldest one
    // TODO : do this right! LRU
    returnCacheLine = &cacheBlock[0];
    return cacheBlock;
  }
  virtual CacheLine* getCacheBlock( uint64_t cacheBlockIndex )
  {
    return &m_cacheLines[cacheBlockIndex * p_NumWays];
  }
  virtual uint64_t    getCacheBlockIndex( uint64_t memoryAddress )
  {
    // which line in the memory does this address select? (imagine the memory is organized in lines)
    uint64_t memoryLineIndex = memoryAddress >> m_bitsForLine;

    // Which cacheBlock does this memoryLine map to?
    // Use the lowest N bits of the memoryLineIndex. We created a mask in the ctor to do this.
    uint64_t cacheBlockIndex = memoryLineIndex & m_cacheBlockIndexMask;
    return cacheBlockIndex;
  }
  virtual uint64_t    getCacheTag( uint64_t memoryAddress )
  {
    // To get the highest bits for the cacheTag, we will just shift the memoryAddress by m_bitShiftForCacheTag.
    return memoryAddress >> m_bitShiftForCacheTag;
  }
  // Return the index of the byte being addressed within the line.
  // This is also the offset from the beginning of the DataLine
  // Just a translation utility.
  virtual uint64_t    getByteIndex( uint64_t memoryAddress )
  {
    // create a bit mask for the number of bits in the address needed to index the word within the line
    // Remember:     m_bitsForLine = logbase2( p_LineSize );
    // So a bit mask of that many 1's can be created simply by subtracting 1 from the LineSize value
    uint64_t bitmaskForWordIndex = p_LineSize - 1;
    return (memoryAddress & bitmaskForWordIndex);
  }
  // Return the index of the word addressed within the line.
  // This is also the offset from the beginning of the DataLine
  // Just a translation utility.
  virtual uint64_t    getWordIndex( uint64_t memoryAddress )
  {
    return (getByteIndex(memoryAddress)/sizeof(uint32_t));
  }
  // For a given word address, returns the address of the cache line that contains it
  // Just a translation utility.
  virtual uint64_t    getLineAddress( uint64_t memoryAddress )
  {
    // zero out the lowest N bits that are needed to index the word within the line
    // Remember:     m_bitsForLine = logbase2( p_LineSize );
    return ((memoryAddress >> m_bitsForLine) << m_bitsForLine);
  }



  // TODO: determine any value in what I did here?
/*
  // For a given word address, returns whether that word is in the cache.
  virtual bool isWordCached( uint64_t adr )
  {
    CacheLine* cacheLine = this->getCacheLine(adr);
    return ( cacheLine != NULL && cacheLine->getValid() );
  }
  // For a given word address, returns a pointer to that word in the cache.
  // This will throw an error if the word is not in the Cache.
  virtual uint32_t* accessWord( uint64_t adr )
  {
    CacheLine* cacheLine = this->getCacheLine(adr);
    uint64_t offsetWithinLine = adr % p_LineSize;
  	return cacheLine->getData(offsetWithinLine);
  }
  // For a given word address, returns the address of the cache line that contains it
  // Just a translation utility.
  // This will throw an error if the word is not in the Cache.
  virtual uint64_t getLineAddress( uint64_t adr )
  {
    // TODO: LINESIZE is useful, but must be a power of 2. So perhaps we just need to store the power
    int howManyBitsToMask = logbase2(p_LineSize);
    //return ( (adr / p_LineSize) * p_LineSize)
    return ( (adr>>howManyBitsToMask) <<howManyBitsToMask);
  }
  // To help copying data from a buffer into cache.
  //  Inputs: ptr into data buffer and length of data to be copied.
  //  Outputs: returns True if operation completed, else
  //           returns False and (1)  next point in buffer of the remaining words to be copied and (2) the # remaining words to be copied.
  // We should only ever copy COMPLETE lines to the cache, so the adr should always be pointing to the start of a cacheline's data.
  virtual bool copyToCacheLine( uint64_t adr, uint32_t* fromBuffer, int len, uint32_t** restOfBuffer, int* lenRemaining )
  {
    if( adr != getLineAddress(adr) ) {
      std::ostringstream oss;
      oss << "Only complete lines can be cached. Thus the address passed must correspond to the beginning of a memory/cache line, but " << adr << "!=" << getLineAddress(adr);
      std::string s = oss.str();
      throw std::runtime_error(s);
    }

    CacheLine* cacheLine = this->getCacheLine(adr);
    if( cacheLine == NULL ) {
      uint64_t memoryLineIndex = memoryAddress / p_LineSize;
      uint64_t cacheTag = memoryLineIndex | m_cacheTagMask;
      // allocate a line of cache
      cl.load( 1111, fromBuffer, this->p_LineSize );
    }
    // TODO !!! refactor this inside of CacheLine ???
    uint32_t* data = cacheLine->getData(0);
    return false;
    memcpy(data,fromBuffer,this->p_LineSize);
    cacheLine->setValid(true);

    // TODO sort out the remaining len and ret true/false

    // TODO NOT IMPLEMENTED
    *restOfBuffer = NULL;
    *lenRemaining = 0;
    return false;
  }
  // To help copying data from cache into a buffer
  //  Inputs: ptr into data buffer and length of data to be copied.
  //  Outputs: returns True if operation completed, else
  //           returns False and (1)  next point in buffer for the remaining words to be copied and (2) the # remaining words to be copied.
  virtual bool copyFromCacheLine( uint32_t* toBuffer, uint32_t* fromCache, int len, uint32_t** restOfBuffer, int* lenRemaining )
  {
    // TODO NOT IMPLEMENTED
    *restOfBuffer = NULL;
    *lenRemaining = 0;
    return false;
  }

protected:
  // This calls picks the cache line to write to: may be there already or not.
  // If not, this may requires the oldest one gets bumped from the cache.
  virtual CacheLine*  getCacheLineForWrite( uint64_t memoryAddress )
  {
    //uint64 memoryWordIndex = memoryAddress / 8;
    uint64_t memoryLineIndex = memoryAddress / p_LineSize;
    // Which cacheBlock does this memoryLine map to?
    // Use the lowest N bits of the memoryLineIndex. We created a mask in the ctor to do this.
    uint64_t cacheBlockIndex = memoryLineIndex | m_cacheBlockIndexMask;
    CacheLine* cacheBlock = this->getCacheBlock(cacheBlockIndex);
    // TODO: implement some sort of LRU here
    // for now, justr returning the address of the CacheBlock which is the first cacheline in the block.
    return cacheBlock;
  }

  virtual bool isDataInCache( uint64_t adr, unsigned int len )
  {
    CacheLine* cacheLine = this->getCacheLine(adr);
    return ( cacheLine != NULL && cacheLine->getValid() );
  }
  */

};



#endif
