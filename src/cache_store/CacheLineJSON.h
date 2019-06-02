#ifndef CacheLineJSON_H
#define CacheLineJSON_H

// Experimneting with JSON as a way to mock class instances.

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
using namespace rapidjson;

class CacheLineJSON : public CacheLine
{
public:
  CacheLineSize_t lineSize;
  CacheLineJSON(CacheLineSize_t lineSize, const char* json)
  : CacheLine()					// 8 words by default; i.e., one Line contains 8 data words
  {
    Document d;
    d.Parse(json);
    this->lineSize =    (CacheLineSize_t) d["lineSize"].GetInt();
    this->isValid =     (bool)    d["isValid"].GetBool();
    this->tag =         (uint64_t)d["tag"].GetInt();
    const Value& aData = d["data"]; // Using a reference for consecutive access is handy and faster.
    assert(aData.IsArray());
    this->data =  new uint32_t[this->lineSize](); //  the () inits all to 0
    for (SizeType ii = 0; ii < aData.Size(); ii++) { // rapidjson type 'SizeType'
      this->data[ii] = aData[ii].GetInt();
    }
  }
  // This assignment operator is only here for purposes of mocking/testing.
  CacheLineJSON& operator=(const CacheLineJSON & cl) {
    isValid = cl.isValid;
    tag = cl.tag;
    data = cl.data;
    lineSize = cl.lineSize;
    return *this;
  }
  bool dump() {
    for (int jj = 0; jj < this->lineSize; jj++) { // rapidjson type 'SizeType'
      cout << jj << " == " << (int)this->data[jj] << endl;
    }
    return true;
  }
};

#endif
