#ifndef BINARYUTILS_HH
#define BINARYUTILS_HH

#include <boost/dynamic_bitset.hpp>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace BinaryUtils {
typedef boost::dynamic_bitset<> bitSet;

bitSet readBinary(const std::string &, size_t);
void writeBinary(const std::string &, const bitSet &, bool paddToBytes);

std::map<unsigned int, std::tuple<BinaryUtils::bitSet, double>>
getStatistics(BinaryUtils::bitSet input, size_t symbolSize = 8);

bitSet getExpRandomBitStream(unsigned long int numBits,
                             bool paddToBytes = false,
                             size_t distribution = 20);

std::map<BinaryUtils::bitSet, std::map<BinaryUtils::bitSet, float>>
computeMarkovChain(const BinaryUtils::bitSet &b, size_t symbolSize = 8);

std::map<BinaryUtils::bitSet, BinaryUtils::bitSet> getMarkovEncodingMap(
    const std::map<BinaryUtils::bitSet, std::map<BinaryUtils::bitSet, float>>
        &markovChain,
    float probabiltyThreshold);

BinaryUtils::bitSet
markovEncode(const std::map<BinaryUtils::bitSet, BinaryUtils::bitSet> &mapping,
             const BinaryUtils::bitSet &data, size_t symbolSize);

BinaryUtils::bitSet
markovDecode(const std::map<BinaryUtils::bitSet, BinaryUtils::bitSet> &mapping,
             const BinaryUtils::bitSet &data, size_t symbolSize);

unsigned int hashValue(const bitSet &b);

} // namespace BinaryUtils

#endif // BINARYUTILS_HH