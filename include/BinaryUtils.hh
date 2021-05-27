#ifndef BINARYUTILS_HH
#define BINARYUTILS_HH

#include <boost/dynamic_bitset.hpp>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace BinaryUtils {
typedef boost::dynamic_bitset<> bitSet;

bitSet
readBinary(const std::string&, size_t);
void
writeBinary(const std::string&, const bitSet&, bool paddToBytes);

std::map<unsigned int, std::tuple<bitSet, double>>
getStatistics(bitSet input, size_t symbolSize = 8);

bitSet
getExpRandomBitStream(unsigned long int numBits,
                      bool paddToBytes = false,
                      size_t distribution = 20);

std::map<bitSet, std::map<bitSet, float>>
computeMarkovChain(const bitSet& b, size_t symbolSize = 8);

std::map<bitSet, bitSet>
getMarkovEncodingMap(const std::map<bitSet, std::map<bitSet, float>>& markovChain,
                     float probabiltyThreshold);

bitSet
markovEncode(const std::map<bitSet, bitSet>& mapping, const bitSet& data, size_t symbolSize);

bitSet
markovDecode(const std::map<bitSet, bitSet>& mapping, const bitSet& data, size_t symbolSize);

unsigned int
hashValue(const bitSet& b);

} // namespace BinaryUtils

#endif // BINARYUTILS_HH