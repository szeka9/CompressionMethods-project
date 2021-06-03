#ifndef HUFFMANTRANSDUCER_HH
#define HUFFMANTRANSDUCER_HH

#include "IDecoder.hh"
#include "IEncoder.hh"

#include <boost/unordered_map.hpp>
#include <map>

class HuffmanTransducer
  : public IEncoder
  , public IDecoder<HuffmanTransducer>
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
   typedef boost::unordered_map<bitSet, double> CodeProbabilityMap;
   HuffmanTransducer(CodeProbabilityMap& symbolMap, size_t symbolSize);
   ~HuffmanTransducer();

   bitSet encodeSymbol(const bitSet& b) const;
   double getEntropy() const;
   double getAvgCodeLength() const;

   // Inherited functions
   bitSet encode(const bitSet&) override;
   bitSet decode(const bitSet&) override;
   bitSet serialize() override;
   static HuffmanTransducer deserialize(const bitSet&);
   size_t getTableSize() const override;
   std::map<bitSet, bitSet> getEncodingMap() const override;

 private:
   HuffmanTransducer(const std::map<bitSet, bitSet>& symbolMap, size_t symbolSize);
   void decodeChangeState(bool);

   size_t mSymbolSize;
   bitSet mBuffer;
   state* mRootState;
   state* mCurrentState;
   double mEntropy;

   boost::unordered_map<bitSet, endState*> mEncodingMap;
   boost::unordered_map<state*, bitSet> mDecodingMap;
   boost::unordered_map<endState*, double> mCodeProbability;
};

#endif // HUFFMANTRANSDUCER_HH