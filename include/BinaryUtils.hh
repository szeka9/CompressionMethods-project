#ifndef BINARYUTILS_HH
#define BINARYUTILS_HH

#include <boost/dynamic_bitset.hpp>
#include <map>
#include <string>
#include <vector>

namespace BinaryUtils {
typedef boost::dynamic_bitset<> bitSet;

bitSet readBinary(const std::string&, size_t);
void writeBinary(const std::string&, const bitSet&, bool paddToBytes);

std::map<unsigned int, std::tuple<BinaryUtils::bitSet, double>> getStatistics(BinaryUtils::bitSet input, size_t symbolSize = 8);
bitSet getExpRandomBitStream(unsigned long int numBits, bool paddToBytes = false, size_t distribution = 20);
bitSet generatePattern(const std::vector<bool>& b, unsigned int size = 8);

unsigned int hashValue(const bitSet& b);

} // namespace BinaryUtils

#endif // BINARYUTILS_HH