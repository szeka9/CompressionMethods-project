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

std::map<size_t, std::tuple<bitSet, double>>
getStatistics(bitSet data, size_t symbolSize = 8);

bitSet
getExpRandomData(size_t numBits, bool paddToBytes = false, size_t distribution = 20);

std::map<bitSet, std::map<bitSet, float>>
computeMarkovChain(const bitSet& data, size_t symbolSize = 8);

std::map<bitSet, bitSet>
getMarkovEncodingMap(const std::map<bitSet, std::map<bitSet, float>>& markovChain,
                     float probabiltyThreshold);

bitSet
markovEncode(const std::map<bitSet, bitSet>& mapping, const bitSet& data, size_t symbolSize);

bitSet
markovDecode(const std::map<bitSet, bitSet>& mapping, const bitSet& data, size_t symbolSize);

size_t
hashValue(const bitSet& b);

} // namespace BinaryUtils

#endif // BINARYUTILS_HH