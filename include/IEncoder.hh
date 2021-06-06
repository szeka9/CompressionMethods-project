#ifndef IENCODER_HH
#define IENCODER_HH

#include <boost/dynamic_bitset.hpp>
#include <map>

typedef boost::dynamic_bitset<> bitSet;

class IEncoder
{
 public:
   virtual ~IEncoder() {}

   virtual size_t getTableSize() const = 0;
   virtual bool isValid() const = 0;
   virtual uint16_t getEncoderId() const = 0;
   virtual void setup(const bitSet&) = 0;
   virtual void reset() = 0;

   // Encoder functions
   virtual bitSet encode(const bitSet&) = 0;
   virtual bitSet decode(const bitSet&) = 0;
   virtual std::map<bitSet, bitSet> getEncodingMap() const = 0;
   virtual bitSet serialize() const = 0;
};

#endif // IENCODER_HH