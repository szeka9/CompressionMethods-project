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
   bitSet currentSymbol = context->mSymbolMap.at(this);
   for (size_t i = 0; i < currentSymbol.size(); ++i)
      context->mBuffer.push_back(currentSymbol[i]);
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

HuffmanTransducer::HuffmanTransducer(std::map<size_t, std::tuple<bitSet, double>> iSymbolMap,
                                     size_t symbolSize)
  : mSymbolSize(symbolSize)
  , mRootState(new state())
  , mCurrentState(mRootState)
{

   // Process the symbol map (create end states)
   std::multimap<double, state*> grouppingMap;
   std::map<size_t, std::tuple<bitSet, double>>::iterator it;

   for (it = iSymbolMap.begin(); it != iSymbolMap.end(); ++it) {
      endState* s = new endState(this, mRootState, mRootState);
      mEndStates.insert(std::make_pair(it->first, s));
      grouppingMap.insert(std::make_pair(std::get<1>(it->second), s));
      mSymbolMap.insert(std::make_pair(s, std::get<0>(it->second)));

      mCodeProbability.insert(std::make_pair(s, std::get<1>(it->second)));
      mEntropy += std::get<1>(it->second) * log2(1 / std::get<1>(it->second));
   }

   // Construct the tree based on minimum probabilities
   while (grouppingMap.size() > 0) {
      if (grouppingMap.size() > 2) {
         state* parentElement =
           new state(grouppingMap.begin()->second, std::next(grouppingMap.begin())->second);
         double sum = grouppingMap.begin()->first + std::next(grouppingMap.begin())->first;
         grouppingMap.erase(grouppingMap.begin(), std::next(grouppingMap.begin(), 2));
         grouppingMap.insert(std::make_pair(sum, parentElement));
      } else {
         mRootState->stateTransitions[0] = grouppingMap.begin()->second;
         mRootState->stateTransitions[1] = std::next(grouppingMap.begin())->second;
         grouppingMap.erase(grouppingMap.begin(), std::next(grouppingMap.begin(), 2));
      }
   }

   // Assign Huffman codes to the symbols by traversing the tree
   bitSet currentCode;
   state* prevState = mRootState;

   while (!(mRootState->mVisited)) {
      if (!(mCurrentState->mZeroVisited)) {
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
      } else if (!(mCurrentState->mOneVisited)) {
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
// HuffmanTransducer - for deserialization
///////////////////////////////////////////////////////////////////////////////

HuffmanTransducer::HuffmanTransducer(const std::map<bitSet, bitSet>& iSymbolMap, size_t symbolSize)
  : mSymbolSize(symbolSize)
  , mRootState(new state())
  , mCurrentState(mRootState)
{
   for (auto it = iSymbolMap.begin(); it != iSymbolMap.end(); ++it) {
      // std::cout << it->first << " " << it->second << std::endl;
      bitSet b = it->second;
      for (size_t i = 0; i < it->second.size(); ++i) {
         if (mCurrentState->next(b[i]) == nullptr) {
            if (i == it->second.size() - 1) {
               auto e = new endState(this, mRootState, mRootState);
               mCurrentState->stateTransitions[b[i]] = e;
               static_cast<endState*>(mCurrentState->stateTransitions[b[i]])->encoded = it->second;
               mEndStates.emplace(hashValue(it->first), e);
               mSymbolMap.emplace(e, it->first);
            } else {
               mCurrentState->stateTransitions[b[i]] = new state();
            }
         } else if (i == it->second.size() - 1) {
            std::cout << "Symbol collision" << std::endl;
         }
         mCurrentState = mCurrentState->next(b[i]);
      }
      mCurrentState = mRootState;
   }
}

///////////////////////////////////////////////////////////////////////////////
// ~HuffmanTransducer
///////////////////////////////////////////////////////////////////////////////
HuffmanTransducer::~HuffmanTransducer()
{
   delete mRootState;
}

///////////////////////////////////////////////////////////////////////////////
// encodeSymbol
///////////////////////////////////////////////////////////////////////////////

bitSet
HuffmanTransducer::encodeSymbol(const bitSet& b) const
{
   // if (mEndStates.at(hashValue(b)) != nullptr)
   return mEndStates.at(hashValue(b))->encoded;
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
   bitSet currentSymbol(mSymbolSize);
   mCurrentState = mRootState;

   for (size_t i = 0; i < data.size(); i += mSymbolSize) {

      for (size_t j = 0; j < mSymbolSize; ++j) {
         currentSymbol[j] = data[j + i];
      }

      bitSet encoded = encodeSymbol(currentSymbol);

      for (size_t j = 0; j < encoded.size(); ++j) {
         output.push_back(encoded[j]);
      }

      /*if (mCurrentState != mRootState) {
         throw std::runtime_error("Did not return to root state!");
      }*/
   }
   return output;
}

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
   for (size_t i = 0; i < data.size(); ++i)
      decodeChangeState(data[i]);
   decodeChangeState(0); // write buffer with last bit

   bitSet output = std::move(mBuffer);
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
     mEndStates.begin(),
     mEndStates.end(),
     0,
     [&](int value, const std::unordered_map<size_t, endState*>::value_type& p) {
        return value + p.second->encoded.size() + mSymbolMap.at(p.second).size();
     });
}

///////////////////////////////////////////////////////////////////////////////
// Serialize
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
// which should always add up to whole bytes (measured in the entry size). The nubmer
// of 0s used must be more than the number of 0s in the encoded size or the offset.
///////////////////////////////////////////////////////////////////////////////

bitSet
HuffmanTransducer::serialize()
{
   bitSet serialized;

   // number of symbols
   auto encodingMap = getEncodingMap();
   bitSet bSize = convertToBitSet(encodingMap.size(), 3 * 8);
   appendBits(serialized, bSize);

   // symbol size and start symbol
   auto it = encodingMap.begin();
   auto currentSymbol = it->first;
   auto bSymbolSize = convertToBitSet(mSymbolSize, 1 * 8);
   if (bSymbolSize.size() > 8)
      throw std::runtime_error("Symbol size takes more than one byte!");
   appendBits(serialized, bSymbolSize);
   appendBits(serialized, currentSymbol);

   bitSet nextSymbol;
   bitSet bOffset;
   int offset;

   size_t i = 0;
   for (it = encodingMap.begin(); it != encodingMap.end(); ++it) {

      if (std::next(it) != encodingMap.end()) {
         nextSymbol = std::next(it)->first;
         offset = int(nextSymbol.to_ulong()) - int(currentSymbol.to_ulong());
         if (offset < 0)
            throw std::runtime_error("Negative offset!");
         bOffset = convertToBitSet(offset);
      } else {
         offset = 0;
         bOffset = bitSet(1);
         nextSymbol = bitSet();
      }

      // Encoded size, offset, zero separators
      auto encodedSize = convertToBitSet(it->second.size());
      reverseBits(encodedSize);
      size_t numSeparator = countZeros(bOffset) > countZeros(encodedSize)
                              ? countZeros(bOffset) + 1
                              : countZeros(encodedSize) + 1;

      if ((encodedSize.size() + bOffset.size() + numSeparator) % 8)
         numSeparator += 8 - ((encodedSize.size() + bOffset.size() + numSeparator) % 8);

      // Entry size computed from the previous values
      size_t entrySize = (encodedSize.size() + bOffset.size() + numSeparator) / 8;
      auto bEntrySize = convertToBitSet(entrySize, 3);

      // Write to output
      appendBits(serialized, bEntrySize);
      appendBits(serialized, encodedSize);
      appendBits(serialized, bitSet(numSeparator));
      appendBits(serialized, bOffset);
      appendBits(serialized, it->second);

      currentSymbol = nextSymbol;
   }

   return serialized;
}

///////////////////////////////////////////////////////////////////////////////
// deSerialize
///////////////////////////////////////////////////////////////////////////////

HuffmanTransducer
HuffmanTransducer::deserialize(const bitSet& data)
{
   std::map<bitSet, bitSet> result;

   auto numSymbols = sliceBitSet(data, 0, 3 * 8);
   auto symbolsize = sliceBitSet(data, 3 * 8, 8);
   bitSet currentSymbol = sliceBitSet(data, 4 * 8, symbolsize.to_ulong());

   size_t symbolCounter = 1;
   size_t currentIdx = 4 * 8 + symbolsize.to_ulong();

   while (symbolCounter != numSymbols.to_ulong() + 1) {
      auto entrySize = sliceBitSet(data, currentIdx, 3).to_ulong();
      currentIdx += 3;

      auto entry = sliceBitSet(data, currentIdx, entrySize * 8);
      auto reversedEntry = entry;
      reverseBits(reversedEntry);

      auto encodedSize = sliceBitSet(reversedEntry, 0, findMostZeros(entry));
      auto offs = sliceBitSet(entry, 0, findMostZeros(reversedEntry));
      reverseBits(encodedSize);
      reverseBits(offs);

      currentIdx += entrySize * 8;

      auto encoded = sliceBitSet(data, currentIdx, encodedSize.to_ulong());
      currentIdx += encodedSize.to_ulong();
      result.emplace(currentSymbol, encoded);

      currentSymbol =
        convertToBitSet(currentSymbol.to_ulong() + offs.to_ulong(), symbolsize.to_ulong());

      symbolCounter += 1;
   }

   return HuffmanTransducer(result, symbolsize.to_ulong());
}

///////////////////////////////////////////////////////////////////////////////
// hasSymbol
///////////////////////////////////////////////////////////////////////////////
std::map<bitSet, bitSet>
HuffmanTransducer::getEncodingMap() const
{
   std::map<bitSet, bitSet> result;
   for (auto p : mEndStates) {
      result.emplace(mSymbolMap.at(p.second), p.second->encoded);
   }
   return result;
}
