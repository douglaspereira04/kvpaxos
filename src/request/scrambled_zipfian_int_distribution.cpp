#ifndef RFUNC_SCRAMBLED_ZIPFIAN_H
#define RFUNC_SCRAMBLED_ZIPFIAN_H

#include <cmath>
#include <mutex> // std::mutex
#include "random.h"
#include "zipfian_int_distribution.cpp"

template<typename _IntType = int>
class scrambled_zipfian_int_distribution
{

public:

  template<typename _UniformRandomBitGenerator>
  _IntType operator()(_UniformRandomBitGenerator &__urng)
  {
    _IntType ret = gen->operator()(__urng);
    ret = min + fnvhash64(ret) % itemcount;
    lastvalue = ret;
    return ret;
  }
  

  scrambled_zipfian_int_distribution(_IntType min_, _IntType max_, double zipfianconstant_ = zipfian_int_distribution<_IntType>::ZIPFIAN_CONSTANT)
  {
    min = min_;
    max = max_;
    itemcount = max - min + 1;
    if (zipfianconstant_ == USED_ZIPFIAN_CONSTANT)
    {
      gen = new zipfian_int_distribution<_IntType>(0, itemcount, zipfianconstant_, ZETAN);
    }
    else
    {
      gen = new zipfian_int_distribution<_IntType>(0, itemcount, zipfianconstant_, zipfian_int_distribution<_IntType>::zetastatic(max - min + 1, zipfianconstant_));
    }
  }

  scrambled_zipfian_int_distribution(const scrambled_zipfian_int_distribution &t)
  {
    gen = new zipfian_int_distribution<_IntType>(*t.gen);
    min = t.min;
    max = t.max;
    itemcount = t.itemcount;
    lastvalue = t.lastvalue;
  }
  
  ~scrambled_zipfian_int_distribution(){
    delete gen;
  }

private:
  zipfian_int_distribution<_IntType> *gen;
  _IntType min, max, itemcount;
  _IntType lastvalue;

  static _IntType fnvhash64(_IntType val)
  {
    long hashval = FNV_OFFSET_BASIS_64;

    for (int i = 0; i < 8; i++)
    {
      long octet = val & 0x00ff;
      val = val >> 8;

      hashval = hashval ^ octet;
      hashval = hashval * FNV_PRIME_64;
      // hashval = hashval ^ octet;
    }
    return (_IntType)abs(hashval);
  }

private:

  static const long FNV_OFFSET_BASIS_64 = 0xCBF29CE484222325L;
  static const long FNV_PRIME_64 = 1099511628211L;
  const double USED_ZIPFIAN_CONSTANT = 0.99;
  const double ZETAN = 26.46902820178302;
  const long ITEM_COUNT = 10000000000L;
};

#endif