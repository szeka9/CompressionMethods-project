#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include "HuffmanTransducer.hh"
#include "BinaryUtils.hh"

#include <iostream>
#include <math.h>
#include <numeric>

using namespace BinaryUtils;

///////////////////////////////////////////////////////////////////////////////
// HuffmanTransducer::state
///////////////////////////////////////////////////////////////////////////////

HuffmanTransducer::state::state(HuffmanTransducer::state* iZero, HuffmanTransducer::state* iOne)
{
   mZeroVisited = false;
   mOneVisited = false;
   mVisited = false;
   stateTransitions[0] = iZero;
   stateTransitions[1] = iOne;
}

HuffmanTransducer::state::~state()
{
   if (stateTransitions[0] != nullptr)
      delete stateTransitions[0];
   if (stateTransitions[1] != nullptr)
      delete stateTransitions[1];
}

HuffmanTransducer::state*
HuffmanTransducer::state::next(bool b)
{
   return stateTransitions[b];
}

HuffmanTransducer::state*
HuffmanTransducer::state::forward(bool b)
{
   return stateTransitions[b];
}

///////////////////////////////////////////////////////////////////////////////
// HuffmanTransducer::endState
///////////////////////////////////////////////////////////////////////////////

HuffmanTransducer::endState::endState(HuffmanTransducer* iContext,
                                      HuffmanTransducer::state* iZero,
                                      HuffmanTransducer::state* iOne)
  : state(iZero, iZero)
{
   context = iContext;
}

HuffmanTransducer::endState::~endState()
{
   stateTransitions[0] = nullptr;
   stateTransitions[1] = nullptr;
}

void
HuffmanTransducer::endState::writeBuffer()
{
   bitSet currentSymbol = context->mDecodingMap.at(this);
   for (size_t i = 0; i < currentSymbol.size(); ++i) {
      context->mBuffer.push_back(currentSymbol[i]);
   }
}

HuffmanTransducer::state*
HuffmanTransducer::endState::next(bool b)
{
   writeBuffer();
   return stateTransitions[b];
}

HuffmanTransducer::state*
HuffmanTransducer::endState::forward(bool b)
{
   writeBuffer();
   return stateTransitions[b]->forward(b);
}

///////////////////////////////////////////////////////////////////////////////
// HuffmanTransducer
///////////////////////////////////////////////////////////////////////////////

HuffmanTransducer::HuffmanTransducer(const bitSet& sourceData, size_t symbolSize, size_t numThreads)
  : mSymbolSize(symbolSize)
  , mNumThreads(numThreads)
  , mRootState(new state())
  , mCurrentState(mRootState)
{
   setupByProbability(getStatistics(sourceData, symbolSize));
}

///////////////////////////////////////////////////////////////////////////////
// HuffmanTransducer - only for deserialization
// Limitation: code probability, avg. length and entropy are not serialized
///////////////////////////////////////////////////////////////////////////////

HuffmanTransducer::HuffmanTransducer(const std::map<bitSet, bitSet>& symbolMap,
                                     size_t symbolSize,
                                     size_t numThreads)
  : mSymbolSize(symbolSize)
  , mNumThreads(numThreads)
  , mRootState(new state())
  , mCurrentState(mRootState)
{
   try {
      for (auto it = symbolMap.begin(); it != symbolMap.end(); ++it) {
         bitSet encoded = it->second;

         // Generate tree
         for (size_t i = 0; i < it->second.size(); ++i) {
            if (mCurrentState->next(encoded[i]) == nullptr) {
               if (i == it->second.size() - 1) {
                  auto e = new endState(this, mRootState, mRootState);
                  mCurrentState->stateTransitions[encoded[i]] = e;
                  static_cast<endState*>(mCurrentState->stateTransitions[encoded[i]])->encoded =
                    it->second;
                  mEncodingMap.emplace(it->first, e);
                  mDecodingMap.emplace(e, it->first);
               } else {
                  mCurrentState->stateTransitions[encoded[i]] = new state();
               }
            } else if (i == it->second.size() - 1) {
               throw std::runtime_error("Symbol collision");
            }
            mCurrentState = mCurrentState->next(encoded[i]);
         }
         mCurrentState = mRootState;
      }
   } catch (...) {
      reset();
   }
}

///////////////////////////////////////////////////////////////////////////////
// HuffmanTransducer
///////////////////////////////////////////////////////////////////////////////

HuffmanTransducer::HuffmanTransducer(size_t symbolSize, size_t numThreads)
  : mSymbolSize(symbolSize)
  , mNumThreads(numThreads)
  , mRootState(new state())
  , mCurrentState(mRootState)
{}

///////////////////////////////////////////////////////////////////////////////
// ~HuffmanTransducer
///////////////////////////////////////////////////////////////////////////////
HuffmanTransducer::~HuffmanTransducer()
{
   delete mRootState;
}

///////////////////////////////////////////////////////////////////////////////
// Setup source data
///////////////////////////////////////////////////////////////////////////////

void
HuffmanTransducer::setup(const bitSet& sourceData)
{
   reset();
   setupByProbability(getStatistics(sourceData, mSymbolSize));
}

///////////////////////////////////////////////////////////////////////////////
// Reset encoder
///////////////////////////////////////////////////////////////////////////////

void
HuffmanTransducer::reset()
{
   mBuffer.clear();
   delete mRootState;
   mRootState = new state();
   mCurrentState = mRootState;
   mEntropy = 0;

   mEncodingMap.clear();
   mDecodingMap.clear();
   mCodeProbability.clear();
}

///////////////////////////////////////////////////////////////////////////////
// Setup source data
///////////////////////////////////////////////////////////////////////////////

void
HuffmanTransducer::setupByProbability(CodeProbabilityMap&& symbolMap)
{
   // Process the symbol map (create end states)
   std::multimap<double, state*> grouppingMap;

   for (auto it = symbolMap.begin(); it != symbolMap.end(); ++it) {
      endState* s = new endState(this, mRootState, mRootState);
      mEncodingMap.insert(std::make_pair(it->first, s));
      mDecodingMap.insert(std::make_pair(s, it->first));
      grouppingMap.insert(std::make_pair(it->second, s));

      mCodeProbability.insert(std::make_pair(s, it->second));
      mEntropy += it->second * log2(1 / it->second);
   }

   // Construct the tree based on minimum probabilities (the elements are ordered)
   while (grouppingMap.size() > 0) {
      auto begin = grouppingMap.begin();
      if (grouppingMap.size() > 2) {
         state* parentElement = new state(begin->second, std::next(begin)->second);
         double sum = begin->first + std::next(begin)->first;
         grouppingMap.erase(begin, std::next(begin, 2));
         grouppingMap.insert(std::make_pair(sum, parentElement));
      } else {
         mRootState->stateTransitions[0] = begin->second;
         mRootState->stateTransitions[1] = std::next(begin)->second;
         grouppingMap.erase(begin, std::next(begin, 2));
      }
   }

   // Assign Huffman codes to the symbols by traversing the tree
   bitSet currentCode;
   state* prevState = mRootState;

   while (!(mRootState->mVisited)) {
      if (!mCurrentState->mZeroVisited) {
         if (mCurrentState->stateTransitions[0]->mVisited) {
            mCurrentState->mZeroVisited = true;
         } else {
            prevState = mCurrentState;
            mCurrentState = mCurrentState->next(0);
            if (mCurrentState != mRootState) {
               currentCode.push_back(0);
            } else {
               prevState->mVisited = true;
               prevState->mZeroVisited = true;
               prevState->mOneVisited = true;
            }
         }
      } else if (!mCurrentState->mOneVisited) {
         if (mCurrentState->stateTransitions[1]->mVisited) {
            mCurrentState->mOneVisited = true;
         } else {
            prevState = mCurrentState;
            mCurrentState = mCurrentState->next(1);
            if (mCurrentState != mRootState) {
               currentCode.push_back(1);
            } else {
               prevState->mVisited = true;
               prevState->mZeroVisited = true;
               prevState->mOneVisited = true;
            }
         }
      } else {
         mCurrentState->mVisited = true;
         currentCode.clear();
         prevState = mRootState;
         mCurrentState = mRootState;
      }

      if (mBuffer.size() > 0) {
         static_cast<endState*>(prevState)->encoded = currentCode;
         mBuffer.clear();
         currentCode.clear();
      }
   }
}

///////////////////////////////////////////////////////////////////////////////
// encodeSymbol
///////////////////////////////////////////////////////////////////////////////

bitSet
HuffmanTransducer::encodeSymbol(const bitSet& b) const
{
   // if (mEncodingMap.at(b) != nullptr)
   return mEncodingMap.at(b)->encoded;
   // else
   // throw std::runtime_error("Null pointer in the end states!");
}

///////////////////////////////////////////////////////////////////////////////
// encode
///////////////////////////////////////////////////////////////////////////////

bitSet
HuffmanTransducer::encode(const bitSet& data)
{
   bitSet output;

   if (!isValid()) {
      return output;
   }

   for (size_t i = 0; i < data.size(); i += mSymbolSize)
      append(output, mEncodingMap.at(slice(data, i, mSymbolSize))->encoded);

   return output;
}

/*
bitSet
HuffmanTransducer::encode(const bitSet& data)
{

   bitSet output;
   std::vector<bitSet> buffers(mNumThreads);

   if (!isValid()) {
      return output;
   }

   mCurrentState = mRootState;

   size_t numSymbols = data.size() / mSymbolSize;
   size_t increment = std::floor(numSymbols / mNumThreads) * mSymbolSize;

#pragma omp parallel for
   for (size_t n = 0; n < mNumThreads; ++n) {
      bitSet currentSymbol(mSymbolSize);
      for (size_t i = n * increment;
           i < (n + 1) * increment || (n == mNumThreads - 1 && i < data.size());
           i += mSymbolSize) {
         currentSymbol = slice(data, i, mSymbolSize);
         auto encoded = encodeSymbol(currentSymbol);
         for (size_t j = 0; j < encoded.size(); ++j) {
            buffers[n].push_back(encoded[j]);
         }
      }
   }
   for (auto b : buffers) {
      append(output, b);
   }
   return output;
}*/

///////////////////////////////////////////////////////////////////////////////
// decodeChangeState
///////////////////////////////////////////////////////////////////////////////

void
HuffmanTransducer::decodeChangeState(bool b)
{
   mCurrentState = mCurrentState->forward(b);
}

///////////////////////////////////////////////////////////////////////////////
// decode
///////////////////////////////////////////////////////////////////////////////

bitSet
HuffmanTransducer::decode(const bitSet& data)
{
   bitSet output;

   if (!isValid()) {
      return output;
   }

   for (size_t i = 0; i < data.size(); ++i) {
      decodeChangeState(data[i]);
   }
   decodeChangeState(0); // write buffer with trailing bit
   mCurrentState = mRootState;

   output = std::move(mBuffer);
   mBuffer.clear();
   return output;
}

///////////////////////////////////////////////////////////////////////////////
// getEntropy
///////////////////////////////////////////////////////////////////////////////

double
HuffmanTransducer::getEntropy() const
{
   return mEntropy;
}

///////////////////////////////////////////////////////////////////////////////
// getAvgCodeLength
///////////////////////////////////////////////////////////////////////////////

double
HuffmanTransducer::getAvgCodeLength() const
{
   double sum = 0;
   for (auto i : mCodeProbability) {
      sum += i.first->encoded.size() * i.second;
   }
   return sum;
}

///////////////////////////////////////////////////////////////////////////////
// getTableSize
///////////////////////////////////////////////////////////////////////////////

size_t
HuffmanTransducer::getTableSize() const
{
   return std::accumulate(
     mEncodingMap.begin(),
     mEncodingMap.end(),
     0,
     [&](int value, const boost::unordered_map<bitSet, endState*>::value_type& p) {
        return value + p.second->encoded.size() + mDecodingMap.at(p.second).size();
     });
}

///////////////////////////////////////////////////////////////////////////////
// serialize
// Stores the encoding map in a format so that the encoder/decoder can be reconstructed
// format:
// [number of symbols (3 bytes)]
// [symbol size (non-encoded) (1 byte)]
// [start symbol (non-encoded): DEF_SYMBOL_SIZE]
// ...
// [entry size (3 bit) - # of bytes]
// [encoded symbol size - # of bits in reversed bit order][0 separators][offset to next
// (non-encoded) symbol] [encoded symbol: m bits]
// ...
//
// 0 separators are used to be able to separate the encoded symbol size and offset
// which should always add up to whole bytes (measured in the entry size). The number
// of 0s used must be more than the number of 0s in the encoded size or the offset.
///////////////////////////////////////////////////////////////////////////////

bitSet
HuffmanTransducer::serialize() const
{
   bitSet serialized;

   if (!isValid()) {
      return serialized;
   }

   append(serialized, convertToBitSet(getEncoderId(), sizeof(uint16_t) * 8));

   // number of symbols
   auto encodingMap = getEncodingMap();
   auto bSize = convertToBitSet(encodingMap.size(), 3 * 8);
   append(serialized, bSize);

   // symbol size and start symbol
   auto it = encodingMap.begin();
   auto currentSymbol = it->first;
   auto bSymbolSize = convertToBitSet(mSymbolSize, 1 * 8);
   if (bSymbolSize.size() > 8)
      throw std::runtime_error("Symbol size takes more than one byte!");
   append(serialized, bSymbolSize);
   append(serialized, currentSymbol);

   bitSet nextSymbol;
   bitSet bOffset;
   int offset;

   for (it = encodingMap.begin(); it != encodingMap.end(); ++it) {

      if (std::next(it) != encodingMap.end()) {
         nextSymbol = std::next(it)->first;
         offset = int(nextSymbol.to_ulong()) - int(currentSymbol.to_ulong());
         if (offset < 0)
            throw std::runtime_error("Negative offset! (the encoding may not be ordered)");
         bOffset = convertToBitSet(offset);
      } else {
         offset = 0;
         bOffset = bitSet(1);
         nextSymbol = bitSet();
      }

      // Encoded size, offset, zero separators
      auto bEncodedSize = convertToBitSet(it->second.size());
      auto numSeparator = countZeros(bOffset) > countZeros(bEncodedSize)
                            ? countZeros(bOffset) + 1
                            : countZeros(bEncodedSize) + 1;

      if ((bEncodedSize.size() + bOffset.size() + numSeparator) % 8)
         numSeparator += 8 - ((bEncodedSize.size() + bOffset.size() + numSeparator) % 8);

      // Entry size computed from the previous values
      size_t entrySize = (bEncodedSize.size() + bOffset.size() + numSeparator) / 8;
      auto bEntrySize = convertToBitSet(entrySize, 3);

      // Write to output
      append(serialized, bEntrySize);
      append(serialized, bEncodedSize);
      append(serialized, bitSet(numSeparator));
      append(serialized, copyReverseBits(bOffset));
      append(serialized, it->second);

      currentSymbol = nextSymbol;
   }

   return serialized;
}

///////////////////////////////////////////////////////////////////////////////
// deserialize
///////////////////////////////////////////////////////////////////////////////

HuffmanTransducer*
HuffmanTransducer::deserializerFactory(const bitSet& data)
{
   std::map<bitSet, bitSet> result;
   size_t currentIdx = 0;

   if (data.size() < sizeof(uint16_t) * 8) {
      return new HuffmanTransducer(result, 0);
   }

   auto encoderId = slice(data, 0, sizeof(uint16_t) * 8).to_ulong();
   currentIdx += sizeof(uint16_t) * 8;
   if (encoderId != mEncoderId) {
      return new HuffmanTransducer(result, 0);
   }

   size_t numSymbols = 0;
   size_t symbolsize = 0;

   if (data.size() > currentIdx + 4 * 8) {
      numSymbols = slice(data, currentIdx, 3 * 8).to_ulong();
      currentIdx += 3 * 8;
      symbolsize = slice(data, currentIdx, 8).to_ulong();
      currentIdx += 8;
   } else {
      return new HuffmanTransducer(result, 0);
   }

   bitSet currentSymbol;
   if (data.size() > currentIdx + symbolsize) {
      currentSymbol = slice(data, currentIdx, symbolsize);
      currentIdx += symbolsize;
   } else {
      return new HuffmanTransducer(result, 0);
   }

   size_t symbolCounter = 0;
   while (symbolCounter < numSymbols && currentIdx + 3 <= data.size()) {
      auto entrySize = slice(data, currentIdx, 3).to_ulong();

      currentIdx += 3;
      if (currentIdx + entrySize * 8 >= data.size())
         break;

      auto entry = slice(data, currentIdx, entrySize * 8);
      auto encodedSize = slice(entry, 0, findMostZeros(entry));
      auto offs = slice(copyReverseBits(entry), 0, findMostZeros(copyReverseBits(entry)));

      currentIdx += entrySize * 8;
      if (currentIdx + encodedSize.to_ulong() > data.size())
         break;

      auto encoded = slice(data, currentIdx, encodedSize.to_ulong());
      currentIdx += encodedSize.to_ulong();
      result.emplace(currentSymbol, encoded);

      currentSymbol = convertToBitSet(currentSymbol.to_ulong() + offs.to_ulong(), symbolsize);
      symbolCounter += 1;
   }

   if (symbolCounter != numSymbols) {
      result.clear();
      symbolsize = 0;
   }

   return new HuffmanTransducer(result, symbolsize);
}

///////////////////////////////////////////////////////////////////////////////
// getEncodingMap
///////////////////////////////////////////////////////////////////////////////
std::map<bitSet, bitSet>
HuffmanTransducer::getEncodingMap() const
{
   std::map<bitSet, bitSet> result;
   for (auto p : mEncodingMap) {
      result.emplace(mDecodingMap.at(p.second), p.second->encoded);
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// isValid
///////////////////////////////////////////////////////////////////////////////
bool
HuffmanTransducer::isValid() const
{
   return mRootState && mCurrentState && mSymbolSize && mEncodingMap.size() &&
          mDecodingMap.size() && mEncodingMap.size() == mDecodingMap.size();
}
