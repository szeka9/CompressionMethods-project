#ifndef MARKOVENCODER_HH
#define MARKOVENCODER_HH

#include "IDecoder.hh"
#include "IEncoder.hh"

#include <boost/unordered_map.hpp>
#include <map>

class MarkovEncoder
  : public IEncoder
  , public IDecoder<MarkovEncoder>
{
 public:
   MarkovEncoder(const bitSet& data, size_t symbolSize, double threshold, bitSet unusedSymbol);

   // Inherited functions from IEncoder
   bitSet encode(const bitSet&) override;
   bitSet decode(const bitSet&) override;
   bitSet serialize() override;
   static MarkovEncoder deserialize(const bitSet&);
   size_t getTableSize() const override;
   std::map<bitSet, bitSet> getEncodingMap() const override;

 private:
   MarkovEncoder(const std::map<bitSet, bitSet>&, bitSet, size_t);

   typedef boost::unordered_map<bitSet, boost::unordered_map<bitSet, float>> MarkovChain;
   MarkovChain computeMarkovChain(const bitSet& data, size_t symbolSize = 8);

   std::map<bitSet, bitSet> createEncodingMap(const MarkovChain& markovChain,
                                              float probabiltyThreshold);

   std::map<bitSet, bitSet> mEncodingMap;
   bitSet mUnusedSymbol;
   size_t mSymbolSize;
};

#endif // MARKOVENCODER_HH