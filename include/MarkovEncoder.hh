#ifndef MARKOVENCODER_HH
#define MARKOVENCODER_HH

#include "IEncoder.hh"

#include <boost/unordered_map.hpp>
#include <map>

class MarkovEncoder : public IEncoder
{
   static const uint16_t mEncoderId = 0x0002;

 public:
   MarkovEncoder(const bitSet& data, size_t symbolSize, double threshold);
   MarkovEncoder(size_t symbolSize, double threshold);
   static MarkovEncoder* deserializerFactory(const bitSet&);

   // Inherited functions from IEncoder
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
   MarkovEncoder(const std::map<bitSet, bitSet>&, bitSet, size_t);

   typedef boost::unordered_map<bitSet, boost::unordered_map<bitSet, float>> MarkovChain;
   MarkovChain computeMarkovChain(const bitSet& data, size_t symbolSize = 8);

   boost::unordered_map<bitSet, bitSet> createEncodingMap(const MarkovChain& markovChain,
                                                          float probabiltyThreshold);

   boost::unordered_map<bitSet, bitSet> mEncodingMap;
   bitSet mUnusedSymbol;
   size_t mSymbolSize;
   float mThreshold;
};

#endif // MARKOVENCODER_HH