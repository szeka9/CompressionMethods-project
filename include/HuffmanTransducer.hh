#ifndef HUFFMANTRANSDUCER_HH
#define HUFFMANTRANSDUCER_HH

#include "IEncoder.hh"

#include <boost/unordered_map.hpp>
#include <map>

class HuffmanTransducer : public IEncoder
{
   static const uint16_t mEncoderId = 0x0001;

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
   HuffmanTransducer(const bitSet& sourceData, size_t symbolSize, size_t numThreads = 1);
   HuffmanTransducer(size_t symbolSize, size_t numThreads = 1);
   ~HuffmanTransducer();

   bitSet encodeSymbol(const bitSet& b) const;
   double getEntropy() const;
   double getAvgCodeLength() const;
   static HuffmanTransducer* deserializerFactory(const bitSet&);

   // Inherited functions
   bitSet encode(const bitSet&) override;
   bitSet decode(const bitSet&) override;
   bitSet serialize() const override;
   size_t getTableSize() const override;
   std::map<bitSet, bitSet> getEncodingMap() const override;
   bool isValid() const override;
   uint16_t getEncoderId() const override { return mEncoderId; };
   void setup(const bitSet&) override;
   void reset() override;

 private:
   HuffmanTransducer(const std::map<bitSet, bitSet>& symbolMap,
                     size_t symbolSize,
                     size_t numThreads = 1);
   void decodeChangeState(bool);
   void setupByProbability(CodeProbabilityMap&& symbolMap);

   size_t mSymbolSize;
   size_t mNumThreads;
   bitSet mBuffer;
   state* mRootState;
   state* mCurrentState;
   double mEntropy;

   boost::unordered_map<bitSet, endState*> mEncodingMap;
   boost::unordered_map<state*, bitSet> mDecodingMap;
   boost::unordered_map<endState*, double> mCodeProbability;
};

#endif // HUFFMANTRANSDUCER_HH