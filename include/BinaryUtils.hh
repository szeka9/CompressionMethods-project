#ifndef BINARYUTILS_HH
#define BINARYUTILS_HH

#include <boost/dynamic_bitset.hpp>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

class HuffmanTransducer;

namespace BinaryUtils {
typedef boost::dynamic_bitset<> bitSet;

bitSet
readBinary(const std::string&, size_t);
void
writeBinary(const std::string&, const bitSet&, bool paddToBytes);

bitSet
convertToBitSet(size_t number, size_t numBits = 0);

std::map<size_t, std::tuple<bitSet, double>>
getStatistics(bitSet data, size_t symbolSize = 8);

bitSet
getExpRandomData(size_t numBits, bool paddToBytes = false, size_t distribution = 20);

void
findUnusedSymbol(const std::map<bitSet, bitSet>& encodingMap, bitSet& result, size_t symbolSize);

size_t
hashValue(const bitSet& b);

void
reverseBits(bitSet& b);

void
appendBits(bitSet& to, const bitSet& from);

bitSet
sliceBitSet(const bitSet& b, size_t startIdx, size_t numBits);

size_t
countZeros(const bitSet& b);

size_t
findMostZeros(const bitSet& b);

bitSet
serialize(std::vector<bitSet> data, size_t numBytes);

std::vector<bitSet>
deSerialize(const bitSet& data, size_t numBytes);

} // namespace BinaryUtils

#endif // BINARYUTILS_HH