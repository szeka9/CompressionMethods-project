#ifndef MARKOVENCODER_HH
#define MARKOVENCODER_HH

#include "IEncoder.hh"

#include <map>

class MarkovEncoder : public IEncoder
{
 public:
   MarkovEncoder(const bitSet& data, size_t symbolSize, double threshold, bitSet unusedSymbol);

   MarkovEncoder& operator=(const MarkovEncoder&) = delete;
   MarkovEncoder(const MarkovEncoder&) = delete;

   // Inherited functions from IEncoder
   bitSet encode(const bitSet& data) override;
   bitSet decode(const bitSet& data) override;
   size_t getTableSize() const override;
   std::map<bitSet, bitSet> getEncodingMap() const override;
   bitSet serialize(const bitSet& data) const override;
   bitSet deSerialize(const bitSet& data) const override;

 private:
   typedef std::map<bitSet, std::map<bitSet, float>> MarkovChain;
   MarkovChain computeMarkovChain(const bitSet& data, size_t symbolSize = 8);

   std::map<bitSet, bitSet> createEncodingMap(const MarkovChain& markovChain,
                                              float probabiltyThreshold);

   bitSet mUnusedSymbol;
   size_t mSymbolSize;
   std::map<bitSet, bitSet> mEncodingMap;
};

#endif // MARKOVENCODER_HH