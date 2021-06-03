#ifndef IENCODER_HH
#define IENCODER_HH

#include <boost/dynamic_bitset.hpp>
#include <map>

typedef boost::dynamic_bitset<> bitSet;

class IEncoder
{
 public:
   virtual ~IEncoder() {}
   virtual bitSet encode(const bitSet&) = 0;
   virtual bitSet serialize() = 0;

   virtual size_t getTableSize() const = 0;
   virtual std::map<bitSet, bitSet> getEncodingMap() const = 0;
};

#endif // IENCODER_HH