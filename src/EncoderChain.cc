#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include "EncoderChain.hh"
#include "BinaryUtils.hh"
#include "HuffmanTransducer.hh"
#include "IEncoder.hh"
#include "MarkovEncoder.hh"
#include "Padder.hh"

#include <numeric>

using namespace BinaryUtils;

///////////////////////////////////////////////////////////////////////////////
// EncoderChain - for deserialization
///////////////////////////////////////////////////////////////////////////////

EncoderChain::EncoderChain() {}

///////////////////////////////////////////////////////////////////////////////
// Setup source data
///////////////////////////////////////////////////////////////////////////////

void
EncoderChain::setup(const bitSet& sourceData)
{
   // Not implemented
}

///////////////////////////////////////////////////////////////////////////////
// Reset encoder
///////////////////////////////////////////////////////////////////////////////

void
EncoderChain::reset()
{
   // Not implemented
}

///////////////////////////////////////////////////////////////////////////////
// addEncoder
///////////////////////////////////////////////////////////////////////////////

void
EncoderChain::addEncoder(std::unique_ptr<IEncoder>&& encoder)
{
   mEncoderChain.push_back(std::move(encoder));
}

///////////////////////////////////////////////////////////////////////////////
// addEncoder
///////////////////////////////////////////////////////////////////////////////

uint16_t
EncoderChain::readEncoderId(const bitSet& b)
{
   if (b.size() > sizeof(uint16_t) * 8) {
      return reverseSlice(b, 0, sizeof(uint16_t) * 8).to_ulong();
   }
   return 0x0000;
}

///////////////////////////////////////////////////////////////////////////////
// getTableSize
///////////////////////////////////////////////////////////////////////////////

size_t
EncoderChain::getTableSize() const
{
   return std::accumulate(
     mEncoderChain.begin(),
     mEncoderChain.end(),
     0,
     [&](int value, const std::unique_ptr<IEncoder>& e) { return value + e->getTableSize(); });
}

///////////////////////////////////////////////////////////////////////////////
// getEncodingMap
///////////////////////////////////////////////////////////////////////////////

/*std::map<bitSet, bitSet>
EncoderChain::getEncodingMap() const
{
   std::map<bitSet, bitSet> result();
   return result;
}*/

///////////////////////////////////////////////////////////////////////////////
// serialize
///////////////////////////////////////////////////////////////////////////////
bitSet
EncoderChain::serialize() const
{
   std::vector<bitSet> serialized;
   for (const std::unique_ptr<IEncoder>& e : mEncoderChain) {
      if (e->isValid()) {
         auto v = e->serialize();
         serialized.insert(serialized.begin(), v); // insert in reverse order
      } else {
         throw std::runtime_error("Serialization of invalid encoder. (Encoder ID: " +
                                  std::to_string(e->getEncoderId()) + ")");
      }
   }
   return BinaryUtils::serialize(serialized, 4);
}

///////////////////////////////////////////////////////////////////////////////
// deserialize
///////////////////////////////////////////////////////////////////////////////
#include <iostream>
EncoderChain*
EncoderChain::deserializerFactory(const bitSet& data)
{
   EncoderChain* result = new EncoderChain();
   auto deserializable = BinaryUtils::deserialize(data, 4);

   for (auto b : deserializable) {
      if (readEncoderId(b) == 0x0001) {
         auto h = std::unique_ptr<HuffmanTransducer>(HuffmanTransducer::deserializerFactory(b));
         if (h && h->isValid())
            result->mEncoderChain.push_back(std::move(h));
      }
      if (readEncoderId(b) == 0x0002) {
         auto m = std::unique_ptr<MarkovEncoder>(MarkovEncoder::deserializerFactory(b));
         if (m && m->isValid())
            result->mEncoderChain.push_back(std::move(m));
      }
      if (readEncoderId(b) == 0x0003) {
         auto m = std::unique_ptr<Padder>(Padder::deserializerFactory(b));
         if (m && m->isValid())
            result->mEncoderChain.push_back(std::move(m));
      }
   }
   return result;
}
#include <iostream>
///////////////////////////////////////////////////////////////////////////////
// Encode data using the encoding chain
///////////////////////////////////////////////////////////////////////////////
bitSet
EncoderChain::encode(const bitSet& data)
{
   bitSet result(data);

   for (const std::unique_ptr<IEncoder>& e : mEncoderChain) {
      if (e->isValid()) {
         result = e->encode(result);
      } else {
         // Retry with setup
         e->setup(result);
         if (e->isValid()) {
            result = e->encode(result);
         } else {
            throw std::runtime_error("Use of invalid encoder during encoding. (Encoder ID: " +
                                     std::to_string(e->getEncoderId()) + ")");
         }
      }
   }

   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Decode data using the encoding chain
///////////////////////////////////////////////////////////////////////////////
bitSet
EncoderChain::decode(const bitSet& data)
{
   bitSet result(data);

   for (const std::unique_ptr<IEncoder>& e : mEncoderChain) {
      if (e->isValid()) {
         result = e->decode(result);
      } else {
         throw std::runtime_error("Use of invalid encoder during decoding. (Encoder ID: " +
                                  std::to_string(e->getEncoderId()) + ")");
      }
   }

   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Decode data using the encoding map
//////////////////////////////////////////////////////////////////////////////
bool
EncoderChain::isValid() const
{
   for (const std::unique_ptr<IEncoder>& e : mEncoderChain) {
      if (!e->isValid())
         return false;
   }
   return true;
};
