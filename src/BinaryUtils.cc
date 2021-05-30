#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include "BinaryUtils.hh"
#include "HuffmanTransducer.hh"

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

#include <sys/stat.h>

#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>

using namespace BinaryUtils;

///////////////////////////////////////////////////////////////////////////////
// Read binary from file
///////////////////////////////////////////////////////////////////////////////

bitSet
BinaryUtils::readBinary(const std::string& fileName, size_t maxSize)
{
   // https://www.cplusplus.com/reference/fstream/ifstream/rdbuf/
   std::ifstream ifs{ fileName, std::ifstream::binary };
   std::filebuf* pbuf = ifs.rdbuf();
   std::size_t size = pbuf->pubseekoff(0, ifs.end, ifs.in);
   pbuf->pubseekpos(0, ifs.in);
   char* buffer = new char[size];
   pbuf->sgetn(buffer, size);
   ifs.close();
   //----------------------------------------------------------
   std::cout << size << std::endl;
   bitSet output(size * 8);
   for (size_t i = 0; i < size; i++) {
      for (size_t j = 0; j < 8; j++) {
         buffer[i] & (1 << (7 - j)) ? output[i * 8 + j] = true : output[i * 8 + j] = false;
      }
   }

   if (0 < maxSize && maxSize < output.size())
      output.resize(maxSize);
   return output;
}

///////////////////////////////////////////////////////////////////////////////
// Write binary to file
///////////////////////////////////////////////////////////////////////////////

void
BinaryUtils::writeBinary(const std::string& fileName, const bitSet& data, bool paddToBytes)
{
   if (data.size() % 8 != 0 && !paddToBytes) {
      throw std::runtime_error("Inappropriate length for a stream of bytes!");
   }
   std::ofstream out{ fileName };

   size_t length = size_t(std::ceil(float(data.size()) / 8));
   char buffer[length] = { 0 };

   for (size_t i = 0; i < length * 8; i += 8)
      for (int j = 0; j < 8 && i + j < data.size(); ++j)
         buffer[size_t(i / 8)] += data[i + j] * pow(2, 7 - j);

   out.write(buffer, sizeof(buffer));

   if (!out.good()) {
      throw std::runtime_error("An error occured during writing!");
   }
}

///////////////////////////////////////////////////////////////////////////////
// Convert to BitSet with padding
///////////////////////////////////////////////////////////////////////////////
BinaryUtils::bitSet
BinaryUtils::convertToBitSet(size_t number, size_t numBits)
{
   size_t size = std::ceil(log2(number));
   size_t finalSize = size > numBits ? size : numBits;
   BinaryUtils::bitSet result(finalSize);

   int pos = finalSize - 1;
   for (size_t i = number % 2; number > 0 && pos >= 0; number /= 2, i = number % 2) {
      i ? result[pos] = 1 : result[pos] = 0;
      --pos;
   }

   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Get statistics of binary data
// The keys of the map are hash values of the symbols
///////////////////////////////////////////////////////////////////////////////

std::map<size_t, std::tuple<bitSet, double>>
BinaryUtils::getStatistics(bitSet data, size_t symbolSize)
{
   if (data.size() % symbolSize != 0) {
      throw std::runtime_error(
        "The symbolsize does not correspond to the data size! You may need to "
        "change the symbolsize to 8 or 16.");
   }

   std::map<size_t, std::tuple<bitSet, double>> result;
   std::tuple<bitSet, double> stats;
   bitSet symbolBuffer(symbolSize);

   for (boost::dynamic_bitset<>::size_type i = 0; i < data.size(); i += symbolSize) {
      symbolBuffer.clear();
      for (boost::dynamic_bitset<>::size_type j = i; j < i + symbolSize; ++j) {
         symbolBuffer.push_back(data[j]);
      }
      std::get<0>(result[BinaryUtils::hashValue(symbolBuffer)]) = symbolBuffer;
      std::get<1>(result[BinaryUtils::hashValue(symbolBuffer)]) += 1.0 / (data.size() / symbolSize);
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Get random binary data in exponential distribution
///////////////////////////////////////////////////////////////////////////////

bitSet
BinaryUtils::getExpRandomData(size_t numBits, bool paddToBytes, size_t distribution)
{
   const int intervals = 256; // number of intervals

   std::random_device rd;
   std::mt19937 rnd_gen(rd());

   std::default_random_engine generator;
   std::exponential_distribution<double> rng(distribution);

   size_t length = numBits;
   if (paddToBytes && numBits % 8 != 0)
      length += 8 - (numBits % 8);
   bitSet output(length);

   for (size_t i = 0; i < length; i += 8)
      for (size_t j = 0; j < 8; j++)
         output[i + j] = size_t(intervals * rng(generator)) & (1 << (7 - j));

   return output;
}

///////////////////////////////////////////////////////////////////////////////
// Find unused symbol
// The unused symbol can be used as a unusedSymbol
///////////////////////////////////////////////////////////////////////////////

void
BinaryUtils::findUnusedSymbol(const std::map<bitSet, bitSet>& encodingMap,
                              bitSet& result,
                              size_t symbolSize)
{
   bitSet current(symbolSize);
   size_t n = 1;
   while (encodingMap.find(current) != encodingMap.end() && current.size() >= log2(n)) {
      current = BinaryUtils::convertToBitSet(n, current.size());
      ++n;
   }
   if (encodingMap.find(current) == encodingMap.end()) {
      result = bitSet(current);
   }
}

///////////////////////////////////////////////////////////////////////////////
// Hash binary data
///////////////////////////////////////////////////////////////////////////////

size_t
BinaryUtils::hashValue(const bitSet& b)
{
   return boost::hash_value(b);
}