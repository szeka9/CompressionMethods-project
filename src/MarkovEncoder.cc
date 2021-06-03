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

MarkovEncoder::MarkovEncoder(const bitSet& data,
                             size_t symbolSize,
                             double threshold,
                             bitSet unusedSymbol = bitSet())
  : mUnusedSymbol(unusedSymbol)
  , mSymbolSize(symbolSize)
{
   mEncodingMap = createEncodingMap(computeMarkovChain(data, symbolSize), threshold);
}

///////////////////////////////////////////////////////////////////////////////
// MarkovEncoder - for deserialization
///////////////////////////////////////////////////////////////////////////////

MarkovEncoder::MarkovEncoder(const std::map<bitSet, bitSet>& iSymbolMap,
                             bitSet iUnusedSymbol,
                             size_t iSymbolSize)
  : mEncodingMap(iSymbolMap)
  , mUnusedSymbol(iUnusedSymbol)
  , mSymbolSize(iSymbolSize)
{}

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

   for (size_t j = 0; j < symbolSize && j < data.size(); ++j)
      previousSymbol[j] = data[j];

   for (size_t i = 0; i < data.size(); i += symbolSize) {
      for (size_t j = 0; j < symbolSize; ++j)
         currentSymbol[j] = data[i + j];

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

std::map<bitSet, bitSet>
MarkovEncoder::createEncodingMap(const MarkovEncoder::MarkovChain& markovChain,
                                 float probabiltyThreshold)
{
   std::map<bitSet, bitSet> result;
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
   std::map<bitSet, bitSet> result(mEncodingMap);
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// serialize
///////////////////////////////////////////////////////////////////////////////
bitSet
MarkovEncoder::serialize()
{
   bitSet result;
   reverseAppend(result, convertToBitSet(mEncodingMap.size(), S_WIDTH));
   reverseAppend(result, convertToBitSet(mSymbolSize, 8));
   reverseAppend(result, mUnusedSymbol);
   for (auto p : mEncodingMap) {
      reverseAppend(result, p.first);
      reverseAppend(result, p.second);
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// deserialize
///////////////////////////////////////////////////////////////////////////////

MarkovEncoder
MarkovEncoder::deserialize(const bitSet& data)
{
   std::map<bitSet, bitSet> result;
   size_t currentIdx = 0;

   size_t numSymbols = sliceBitSet(data, currentIdx, S_WIDTH).to_ulong();
   currentIdx += S_WIDTH;
   size_t symbolSize = sliceBitSet(data, currentIdx, 8).to_ulong();
   currentIdx += 8;
   bitSet unusedSymbol = sliceBitSet(data, currentIdx, symbolSize);
   currentIdx += symbolSize;

   size_t symbolCounter = 0;
   while (symbolCounter < numSymbols && currentIdx + symbolSize * 2 <= data.size()) {
      result.emplace(sliceBitSet(data, currentIdx, symbolSize),
                     sliceBitSet(data, currentIdx + symbolSize, symbolSize));
      currentIdx += 2 * symbolSize;
      ++symbolCounter;
   }
   return MarkovEncoder(result, unusedSymbol, symbolSize);
}

///////////////////////////////////////////////////////////////////////////////
// Encode data using the encoding map
///////////////////////////////////////////////////////////////////////////////
bitSet
MarkovEncoder::encode(const bitSet& data)
{
   bitSet result(data.size());

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

   else
      for (size_t i = 0; i < data.size(); i += mSymbolSize) {
         for (size_t j = 0; j < mSymbolSize && j + i < data.size(); ++j)
            currentSymbol[j] = data[j + i];

         if (i == 0 || currentSymbol != mapped)
            for (size_t j = 0; j < mSymbolSize && j + i < data.size(); ++j)
               result[i + j] = data[i + j];
         else
            for (size_t j = 0; j < mSymbolSize && j + i < data.size(); ++j)
               result[i + j] = mUnusedSymbol[j];

         if (mEncodingMap.find(currentSymbol) != mEncodingMap.end())
            mapped = mEncodingMap.at(currentSymbol);
         else
            mapped = bitSet(mSymbolSize);
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

         for (size_t j = 0; j < mSymbolSize && j + i < data.size(); ++j)
            currentSymbol[j] = data[j + i];

         if (i != 0 && currentSymbol == mUnusedSymbol)
            currentSymbol = mapped;

         for (size_t j = 0; j < mSymbolSize && j + i < result.size(); ++j)
            result[i + j] = currentSymbol[j];

         if (mEncodingMap.find(currentSymbol) != mEncodingMap.end())
            mapped = mEncodingMap.at(currentSymbol);
         else
            mapped = bitSet(mSymbolSize);
      }

   return result;
}