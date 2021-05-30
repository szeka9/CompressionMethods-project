#ifndef HUFFMANTRANSDUCER_HH
#define HUFFMANTRANSDUCER_HH

#include "IEncoder.hh"

#include <map>
#include <unordered_map>

class HuffmanTransducer : public IEncoder
{
 private:
   class state
   {
    public:
      state(state* iZero = nullptr, state* iOne = nullptr);
      virtual ~state();

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
      ~endState();

      void writeBuffer();
      state* next(bool) override;
      state* forward(bool) override;

      bitSet encoded;
      HuffmanTransducer* context;
   };

 public:
   HuffmanTransducer(std::map<size_t, std::tuple<bitSet, double>> iSymbolMap, size_t symbolSize);
   ~HuffmanTransducer();

   HuffmanTransducer& operator=(const HuffmanTransducer&) = delete;
   HuffmanTransducer(const HuffmanTransducer&) = delete;

   bitSet encodeSymbol(const bitSet& b) const;
   double getEntropy() const;
   double getAvgCodeLength() const;

   // Inherited functions from IEncoder
   bitSet encode(const bitSet& data) override;
   bitSet decode(const bitSet& data) override;
   bitSet serialize(const bitSet& data) override;
   bitSet deSerialize(const bitSet& data) override;

   size_t getTableSize() const override;
   std::map<bitSet, bitSet> getEncodingMap() const override;

 private:
   void decodeChangeState(bool b);

   size_t mSymbolSize;
   bitSet mBuffer;
   double mEntropy;
   state* mRootState;
   state* mCurrentState;

   std::unordered_map<size_t, endState*> mEndStates;
   std::unordered_map<endState*, double> mCodeProbability;
   std::unordered_map<state*, bitSet> mSymbolMap;
};

#endif // HUFFMANTRANSDUCER_HH