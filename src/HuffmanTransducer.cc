#include "HuffmanTransducer.hh"
#include "BinaryUtils.hh"

#include <iostream>
#include <math.h>
#include <numeric>

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

void
HuffmanTransducer::endState::writeBuffer()
{
   BinaryUtils::bitSet currentSymbol = context->mSymbolMap.at(this);
   for (boost::dynamic_bitset<>::size_type i = 0; i < currentSymbol.size(); ++i)
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

HuffmanTransducer::HuffmanTransducer(
  std::map<unsigned int, std::tuple<BinaryUtils::bitSet, double>> iSymbolMap)
{
   mRootState = new state();
   mCurrentState = mRootState;

   // Process the symbol map (create end states)
   std::multimap<double, state*> grouppingMap;
   std::map<unsigned int, std::tuple<BinaryUtils::bitSet, double>>::iterator it;

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
   BinaryUtils::bitSet currentCode;
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
// encodeSymbol
///////////////////////////////////////////////////////////////////////////////

BinaryUtils::bitSet
HuffmanTransducer::encodeSymbol(const BinaryUtils::bitSet& b) const
{
   // if (mEndStates.at(BinaryUtils::hashValue(b)) != nullptr)
   return mEndStates.at(BinaryUtils::hashValue(b))->encoded;
   // else
   // throw "Null pointer in the end states!";
}

///////////////////////////////////////////////////////////////////////////////
// encode
///////////////////////////////////////////////////////////////////////////////

BinaryUtils::bitSet
HuffmanTransducer::encode(const BinaryUtils::bitSet& chunk, size_t symbolSize)
{
   BinaryUtils::bitSet output;
   BinaryUtils::bitSet buffer(symbolSize);
   mCurrentState = mRootState;

   for (size_t i = 0; i < chunk.size(); i += symbolSize) {

      for (unsigned int j = 0; j < symbolSize; ++j) {
         buffer[j] = chunk[j + i];
      }

      BinaryUtils::bitSet encoded = encodeSymbol(buffer);

      for (boost::dynamic_bitset<>::size_type j = 0; j < encoded.size(); ++j) {
         output.push_back(encoded[j]);
      }

      /*if (mCurrentState != mRootState) {
         throw "Did not return to root state!";
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

void
HuffmanTransducer::decode(const BinaryUtils::bitSet& chunk)
{
   for (boost::dynamic_bitset<>::size_type i = 0; i < chunk.size(); ++i)
      decodeChangeState(chunk[i]);
   decodeChangeState(0); // write buffer with last bit
}

///////////////////////////////////////////////////////////////////////////////
// moveBuffer
///////////////////////////////////////////////////////////////////////////////

void
HuffmanTransducer::moveBuffer(BinaryUtils::bitSet& output)
{
   output = std::move(mBuffer);
   mBuffer.clear();
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

/*
int HuffmanTransducer::getRepresentationSize()
{
   return std::accumulate(
       mEndStates.begin(), mEndStates.end(), 0,
       [](int value,
           std::unordered_map<BinaryUtils::bitSet, endState*>::value_type& p) {
return value + p.second->encoded.size(); });
}*/
