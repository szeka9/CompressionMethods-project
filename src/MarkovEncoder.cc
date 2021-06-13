#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include "MarkovEncoder.hh"
#include "BinaryUtils.hh"

#include <map>
#include <unordered_map>

using namespace BinaryUtils;

#define S_WIDTH 3 * 8

///////////////////////////////////////////////////////////////////////////////
// MarkovEncoder
///////////////////////////////////////////////////////////////////////////////

MarkovEncoder::MarkovEncoder(const bitSet& data, size_t symbolSize, double threshold)
  : mUnusedSymbol(bitSet())
  , mSymbolSize(symbolSize)
  , mThreshold(threshold)
{
   setup(data);
}

///////////////////////////////////////////////////////////////////////////////
// MarkovEncoder
///////////////////////////////////////////////////////////////////////////////

MarkovEncoder::MarkovEncoder(size_t symbolSize, double threshold)
  : mUnusedSymbol(bitSet())
  , mSymbolSize(symbolSize)
  , mThreshold(threshold)
{}

///////////////////////////////////////////////////////////////////////////////
// MarkovEncoder - for deserialization
///////////////////////////////////////////////////////////////////////////////

MarkovEncoder::MarkovEncoder(const std::map<bitSet, bitSet>& iSymbolMap,
                             bitSet iUnusedSymbol,
                             size_t iSymbolSize)
  : /*mEncodingMap(iSymbolMap)
  ,*/
  mUnusedSymbol(iUnusedSymbol)
  , mSymbolSize(iSymbolSize)
{
   for (auto e : iSymbolMap)
      mEncodingMap.emplace(e.first, e.second);
}

///////////////////////////////////////////////////////////////////////////////
// Setup source data
///////////////////////////////////////////////////////////////////////////////

void
MarkovEncoder::setup(const bitSet& sourceData)
{
   reset();
   findUnusedSymbol(sourceData, mUnusedSymbol, mSymbolSize);
   mEncodingMap = createEncodingMap(computeMarkovChain(sourceData, mSymbolSize), mThreshold);
}

///////////////////////////////////////////////////////////////////////////////
// Reset encoder
///////////////////////////////////////////////////////////////////////////////

void
MarkovEncoder::reset()
{
   mEncodingMap.clear();
   mUnusedSymbol.clear();
}

///////////////////////////////////////////////////////////////////////////////
// Generate probability map
// Returns  map of symbols with consequtive states and frequencies.
// The outer map contains all symbols.
// The inner map contains the possible state transitions with frequencies.
///////////////////////////////////////////////////////////////////////////////

MarkovEncoder::MarkovChain
MarkovEncoder::computeMarkovChain(const bitSet& data, size_t symbolSize)
{
   MarkovChain result;
   bitSet previousSymbol(symbolSize);
   bitSet currentSymbol(symbolSize);

   previousSymbol = slice(data, 0, symbolSize);

   for (size_t i = 0; i < data.size(); i += symbolSize) {
      currentSymbol = slice(data, i, symbolSize);
      boost::unordered_map<bitSet, float> nextStates;
      result.emplace(previousSymbol, nextStates);
      result.at(previousSymbol).emplace(currentSymbol, 0);
      result.at(previousSymbol).at(currentSymbol) += 1;
      previousSymbol = currentSymbol;
   }

   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Create encoding map based on Markov chain
// Example: key=0001, value=0110 means that the next symbol to 0001 is 0110.
///////////////////////////////////////////////////////////////////////////////

boost::unordered_map<bitSet, bitSet>
MarkovEncoder::createEncodingMap(const MarkovEncoder::MarkovChain& markovChain,
                                 float probabiltyThreshold)
{
   boost::unordered_map<bitSet, bitSet> result;
   for (auto it = markovChain.begin(); it != markovChain.end(); ++it) {
      auto currentSymbol = it->first;

      bitSet candidate;
      float currentFrequency = 0;
      float sum = 0;

      auto nextStates = it->second;

      for (auto it2 = nextStates.begin(); it2 != nextStates.end(); ++it2) {
         if (it2->second > currentFrequency) {
            candidate = it2->first;
            currentFrequency = it2->second;
         }
         sum += it2->second;
      }
      if ((currentFrequency / sum) > probabiltyThreshold) {
         result.emplace(currentSymbol, candidate);
      }
   }

   return result;
}

///////////////////////////////////////////////////////////////////////////////
// getTableSize
///////////////////////////////////////////////////////////////////////////////

size_t
MarkovEncoder::getTableSize() const
{
   return mEncodingMap.size() * 2 * mSymbolSize;
}

///////////////////////////////////////////////////////////////////////////////
// getEncodingMap
///////////////////////////////////////////////////////////////////////////////

std::map<bitSet, bitSet>
MarkovEncoder::getEncodingMap() const
{
   std::map<bitSet, bitSet> result;
   for (auto e : mEncodingMap) {
      result.emplace(e.first, e.second);
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// serialize
///////////////////////////////////////////////////////////////////////////////
bitSet
MarkovEncoder::serialize() const
{
   bitSet result;

   if (!isValid()) {
      return result;
   }

   append(result, convertToBitSet(getEncoderId(), sizeof(uint16_t) * 8));
   append(result, convertToBitSet(mEncodingMap.size(), S_WIDTH));
   append(result, convertToBitSet(mSymbolSize, 8));
   append(result, mUnusedSymbol);
   for (auto p : mEncodingMap) {
      append(result, p.first);
      append(result, p.second);
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// deserialize
///////////////////////////////////////////////////////////////////////////////

MarkovEncoder*
MarkovEncoder::deserializerFactory(const bitSet& data)
{
   std::map<bitSet, bitSet> result;
   bitSet unusedSymbol;
   size_t symbolSize = 0;

   size_t currentIdx = 0;
   size_t symbolCounter = 0;
   size_t numSymbols = 0;

   if (data.size() < sizeof(uint16_t) * 8) {
      return new MarkovEncoder(result, unusedSymbol, symbolSize);
   }

   auto encoderId = slice(data, 0, sizeof(uint16_t) * 8).to_ulong();
   currentIdx += sizeof(uint16_t) * 8;
   if (encoderId != mEncoderId) {
      return new MarkovEncoder(result, unusedSymbol, symbolSize);
   }

   if (data.size() > S_WIDTH + 8) {
      numSymbols = slice(data, currentIdx, S_WIDTH).to_ulong();
      currentIdx += S_WIDTH;
      symbolSize = slice(data, currentIdx, 8).to_ulong();
      currentIdx += 8;
   } else {
      return new MarkovEncoder(result, unusedSymbol, symbolSize);
   }

   if (data.size() > S_WIDTH + 8 + symbolSize) {
      unusedSymbol = slice(data, currentIdx, symbolSize);
      currentIdx += symbolSize;
   } else {
      symbolSize = 0;
      return new MarkovEncoder(result, unusedSymbol, symbolSize);
   }

   while (symbolCounter < numSymbols && currentIdx + symbolSize * 2 <= data.size()) {
      result.emplace(slice(data, currentIdx, symbolSize),
                     slice(data, currentIdx + symbolSize, symbolSize));
      currentIdx += 2 * symbolSize;
      ++symbolCounter;
   }

   if (symbolCounter != numSymbols) {
      result.clear();
      unusedSymbol = bitSet();
      symbolSize = 0;
   }
   return new MarkovEncoder(result, unusedSymbol, symbolSize);
}

///////////////////////////////////////////////////////////////////////////////
// Encode data using the encoding map
///////////////////////////////////////////////////////////////////////////////
bitSet
MarkovEncoder::encode(const bitSet& data)
{
   bitSet result(data.size());

   if (!isValid()) {
      return result;
   }

   bitSet currentSymbol(mSymbolSize);
   bitSet mapped(mSymbolSize);

   if (!mUnusedSymbol.size())
      for (size_t i = 0; i < data.size(); i += mSymbolSize) {
         for (size_t j = 0; j < mSymbolSize && j + i < data.size(); ++j) {
            currentSymbol[j] = data[j + i];
            result[i + j] = data[i + j] ^ mapped[j];
         }

         if (mEncodingMap.find(currentSymbol) != mEncodingMap.end())
            mapped = mEncodingMap.at(currentSymbol);
         else
            mapped = bitSet(mSymbolSize);
      }

   else {
      assign(result, data, 0, mSymbolSize);
      for (size_t i = mSymbolSize; i < data.size(); i += mSymbolSize) {
         currentSymbol = slice(data, i, mSymbolSize);

         if (currentSymbol != mapped)
            assign(result, data, i, mSymbolSize, i);
         else
            assign(result, mUnusedSymbol, i, mSymbolSize);

         if (mEncodingMap.find(currentSymbol) != mEncodingMap.end())
            mapped = mEncodingMap.at(currentSymbol);
         else
            mapped.clear();
      }
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Decode data using the encoding map
///////////////////////////////////////////////////////////////////////////////
bitSet
MarkovEncoder::decode(const bitSet& data)
{
   bitSet result(data.size());

   if (!isValid()) {
      return result;
   }

   bitSet currentSymbol(mSymbolSize);
   bitSet mapped(mSymbolSize);

   if (!mUnusedSymbol.size())
      for (size_t i = 0; i < data.size(); i += mSymbolSize) {
         for (size_t j = 0; j < mSymbolSize && j + i < data.size(); ++j) {
            currentSymbol[j] = data[j + i] ^ mapped[j];
            result[i + j] = currentSymbol[j];
         }

         if (mEncodingMap.find(currentSymbol) != mEncodingMap.end())
            mapped = mEncodingMap.at(currentSymbol);
         else
            mapped = bitSet(mSymbolSize);
      }

   else
      for (size_t i = 0; i < data.size(); i += mSymbolSize) {

         currentSymbol = slice(data, i, mSymbolSize);

         if (i != 0 && currentSymbol == mUnusedSymbol)
            currentSymbol = mapped;

         assign(result, currentSymbol, i, mSymbolSize);

         if (mEncodingMap.find(currentSymbol) != mEncodingMap.end())
            mapped = mEncodingMap.at(currentSymbol);
         else
            mapped = bitSet(mSymbolSize);
      }

   return result;
}

///////////////////////////////////////////////////////////////////////////////
// isValid
///////////////////////////////////////////////////////////////////////////////
bool
MarkovEncoder::isValid() const
{
   return mEncodingMap.size() && mUnusedSymbol.size() && mSymbolSize;
}