#ifndef HUFFMANTRANSDUCER_HH
#define HUFFMANTRANSDUCER_HH

#include "BinaryUtils.hh"

#include <boost/dynamic_bitset.hpp>
#include <map>
#include <unordered_map>

typedef boost::dynamic_bitset<> bitSet;

class HuffmanTransducer
{
 private:
   class state
   {
    public:
      state(state* iZero = nullptr, state* iOne = nullptr);
      virtual ~state() {}

      state* stateTransitions[2];
      bool mZeroVisited;
      bool mOneVisited;
      bool mVisited;

      virtual state* next(bool);
      virtual state* forward(bool);
   };

   class endState : public state
   {
    public:
      endState(HuffmanTransducer* iContext, state* iZero = nullptr, state* iOne = nullptr);

      void writeBuffer();
      state* next(bool) override;
      state* forward(bool) override;

      bitSet encoded;
      HuffmanTransducer* context;
   };

 public:
   HuffmanTransducer(std::map<size_t, std::tuple<bitSet, double>> iSymbolMap);

   bitSet encodeSymbol(const bitSet& b) const;
   bitSet encode(const bitSet& data, size_t symbolSize);
   void decode(const bitSet& data);
   void moveBuffer(bitSet& output);

   size_t getTableSize() const;
   double getEntropy() const;
   double getAvgCodeLength() const;
   std::map<bitSet, bitSet> getEncodingMap() const;

 private:
   void decodeChangeState(bool b);

   bitSet mBuffer;
   double mEntropy;
   state* mRootState;
   state* mCurrentState;

   std::unordered_map<size_t, endState*> mEndStates;
   std::unordered_map<endState*, double> mCodeProbability;
   std::unordered_map<state*, bitSet> mSymbolMap;
};

#endif // HUFFMANTRANSDUCER_HH