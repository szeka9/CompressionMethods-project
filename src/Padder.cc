#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include "Padder.hh"
#include "BinaryUtils.hh"
#include "IEncoder.hh"

using namespace BinaryUtils;

///////////////////////////////////////////////////////////////////////////////
// Padder - for deserialization
///////////////////////////////////////////////////////////////////////////////

Padder::Padder(PaddingType padding)
  : mPaddingMode(padding)
{}

///////////////////////////////////////////////////////////////////////////////
// Setup source data
///////////////////////////////////////////////////////////////////////////////

void
Padder::setup(const bitSet& sourceData)
{
   reset();
}

///////////////////////////////////////////////////////////////////////////////
// Reset encoder
///////////////////////////////////////////////////////////////////////////////

void
Padder::reset()
{
   mAddedBits = 0;
}

///////////////////////////////////////////////////////////////////////////////
// getTableSize
///////////////////////////////////////////////////////////////////////////////

size_t
Padder::getTableSize() const
{
   return 0;
}

///////////////////////////////////////////////////////////////////////////////
// serialize
///////////////////////////////////////////////////////////////////////////////
bitSet
Padder::serialize() const
{
   bitSet serialized;
   reverseAppend(serialized, convertToBitSet(getEncoderId(), sizeof(uint16_t) * 8));
   reverseAppend(serialized, convertToBitSet(mPaddingMode, 8));
   reverseAppend(serialized, convertToBitSet(mAddedBits, sizeof(uint32_t) * 8));
   return serialized;
}

///////////////////////////////////////////////////////////////////////////////
// deserialize
///////////////////////////////////////////////////////////////////////////////
#include <iostream>
Padder*
Padder::deserializerFactory(const bitSet& data)
{
   Padder* result = new Padder(PaddingType::None);
   size_t currentIdx = 0;

   if (currentIdx + sizeof(uint16_t) * 8 > data.size())
      return result;

   auto encoderId = reverseSlice(data, currentIdx, sizeof(uint16_t) * 8).to_ulong();
   currentIdx += sizeof(uint16_t) * 8;

   if (encoderId != mEncoderId || currentIdx + 8 > data.size())
      return result;

   auto mode = reverseSlice(data, currentIdx, 8).to_ulong();
   result->mPaddingMode =
     mode <= PaddingType::OddBytes ? static_cast<PaddingType>(mode) : PaddingType::None;

   currentIdx += 8;

   if (currentIdx + sizeof(uint32_t) * 8 > data.size()) {
      result->mPaddingMode = PaddingType::None;
      return result;
   }

   result->mAddedBits = reverseSlice(data, currentIdx, sizeof(uint32_t) * 8).to_ulong();
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Encode data using the encoding chain
///////////////////////////////////////////////////////////////////////////////
bitSet
Padder::encode(const bitSet& data)
{
   bitSet result(data);
   if (!isValid()) {
      result.clear();
      return result;
   }

   bitSet padding;
   switch (mPaddingMode) {
      case WholeBytes:
         padding = result.size() % 8 ? convertToBitSet(0, 8 - result.size() % 8) : bitSet();
         break;
      case EvenBytes:
         padding = result.size() % 16 ? convertToBitSet(0, 16 - result.size() % 16) : bitSet();
         break;
      case OddBytes:
         padding = result.size() % 8 ? convertToBitSet(0, 8 - result.size() % 8) : bitSet();
         padding =
           (result.size() + padding.size()) % 16 ? padding : convertToBitSet(0, padding.size() + 8);
         break;
      default:
         break;
   }
   reverseAppend(result, padding);
   mAddedBits = result.size() - data.size();

   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Decode data using the encoding chain
///////////////////////////////////////////////////////////////////////////////
bitSet
Padder::decode(const bitSet& data)
{
   if (!isValid())
      return bitSet();

   // TODO: fix bit order
   return copyReverseBits(reverseSlice(data, 0, data.size() - mAddedBits));
}

///////////////////////////////////////////////////////////////////////////////
// Decode data using the encoding map
//////////////////////////////////////////////////////////////////////////////
bool
Padder::isValid() const
{
   return mPaddingMode != PaddingType::None;
};
