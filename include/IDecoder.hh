#ifndef IDECODER_HH
#define IDECODER_HH

#include <boost/dynamic_bitset.hpp>

typedef boost::dynamic_bitset<> bitSet;

template<typename T>
class IDecoder
{
 public:
   virtual ~IDecoder() {}
   virtual bitSet decode(const bitSet&) = 0;
   static T deserialize(const bitSet& b) { return T::deserialize(b); };
};

#endif // IDECODER_HH