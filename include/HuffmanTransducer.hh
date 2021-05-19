#ifndef HUFFMANTRANSDUCER_HH
#define HUFFMANTRANSDUCER_HH

#include "BinaryUtils.hh"

#include <map>
#include <unordered_map>

class HuffmanTransducer {
   private:
   class state {
  public:
      state(state* iZero = nullptr, state* iOne = nullptr);
      virtual ~state() { }

      state* stateTransitions[2];
      bool mZeroVisited;
      bool mOneVisited;
      bool mVisited;

      virtual state* next(bool);
      virtual state* forward(bool);
   };

   class endState : public state {
  public:
      endState(HuffmanTransducer* iContext,
          state* iZero = nullptr,
          state* iOne = nullptr);

      void writeBuffer();
      state* next(bool) override;
      state* forward(bool) override;

      BinaryUtils::bitSet encoded;
      HuffmanTransducer* context;
   };

   public:
   HuffmanTransducer(std::map<unsigned int, std::tuple<BinaryUtils::bitSet, double>> iSymbolMap);

   BinaryUtils::bitSet encodeSymbol(const BinaryUtils::bitSet& b) const;
   BinaryUtils::bitSet encodeChunk(const BinaryUtils::bitSet& chunk, size_t symbolSize);

   void decodeChunk(const BinaryUtils::bitSet& chunk);
   void flushBuffer(BinaryUtils::bitSet& output);
   //unsigned int getRepresentationSize();
   //void printEncodingTable() const;
   double getEntropy() const;
   double getAvgCodeLength() const;

   private:
   void decodeChangeState(bool b);

   double mEntropy;
   state* mRootState;
   state* mCurrentState;
   std::unordered_map<unsigned int, endState*> mEndStates;
   std::unordered_map<endState*, double> mCodeProbability;
   std::unordered_map<state*, BinaryUtils::bitSet> mSymbolMap;
   BinaryUtils::bitSet mBuffer;
};

#endif // HUFFMANTRANSDUCER_HH