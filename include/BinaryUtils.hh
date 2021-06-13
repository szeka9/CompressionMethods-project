#ifndef BINARYUTILS_HH
#define BINARYUTILS_HH

#include <boost/dynamic_bitset.hpp>
#include <boost/functional/hash.hpp>
namespace boost {
template<typename B, typename A>
std::size_t
hash_value(const boost::dynamic_bitset<B, A>& a)
{
   std::size_t res = hash_value(a.m_num_bits);
   boost::hash_combine(res, a.m_bits);
   return res;
}
} // namespace boost

#include <boost/unordered_map.hpp>
#include <map>
#include <string>
#include <vector>

class HuffmanTransducer;

namespace BinaryUtils {

typedef boost::dynamic_bitset<> bitSet;
typedef boost::unordered_map<bitSet, double> CodeProbabilityMap;

///////////////////////////////////////////////////////////////////////////////

bitSet
readBinary(const std::string& inputPath, size_t maxSize);

void
writeBinary(const std::string& outputPath, const bitSet& data);

void
writeBinary(const std::string& outputPath, const std::vector<bitSet>& data);

bitSet
convertToBitSet(size_t number, size_t numBits = 0);

CodeProbabilityMap
getStatistics(bitSet data, size_t symbolSize = 8);

bitSet
getExpRandomData(size_t numBits, bool paddToBytes = false, size_t distribution = 20);

void
findUnusedSymbol(const std::map<bitSet, bitSet>& encodingMap, bitSet& result, size_t symbolSize);

void
findUnusedSymbol(const bitSet& data, bitSet& result, size_t symbolSize);

size_t
hashValue(const bitSet&);

void
reverseBits(bitSet&);

bitSet
copyReverseBits(const bitSet&);

void
append(bitSet& to, const bitSet& from);

void
assign(bitSet& to,
       const bitSet& from,
       size_t startIdx_to,
       size_t numBits,
       size_t startIdx_from = 0);

bitSet
slice(const bitSet& b, size_t startIdx, size_t numBits);

size_t
countZeros(const bitSet&);

size_t
findMostZeros(const bitSet&);

bitSet
serialize(std::vector<bitSet> data, size_t numBytes);

std::vector<bitSet>
deserialize(const bitSet& data, size_t numBytes);

} // namespace BinaryUtils

#endif // BINARYUTILS_HH