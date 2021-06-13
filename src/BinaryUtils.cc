#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include "BinaryUtils.hh"
#include "HuffmanTransducer.hh"

#include <algorithm>
#include <boost/unordered_set.hpp>
#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

using namespace BinaryUtils;

///////////////////////////////////////////////////////////////////////////////
// Read binary from file
// maxSize: max. number of bytes
///////////////////////////////////////////////////////////////////////////////
bitSet
BinaryUtils::readBinary(const std::string& inputPath, size_t maxSize)
{
   // https://www.cplusplus.com/reference/fstream/ifstream/rdbuf/
   std::ifstream ifs{ inputPath, std::ifstream::binary };
   std::filebuf* pbuf = ifs.rdbuf();
   std::size_t size = pbuf->pubseekoff(0, ifs.end, ifs.in);
   size = size > maxSize && maxSize != 0 ? maxSize : size;
   pbuf->pubseekpos(0, ifs.in);
   char* buffer = new char[size];
   pbuf->sgetn(buffer, size);
   ifs.close();
   //----------------------------------------------------------

   bitSet output(size * 8);

#pragma omp parallel for
   for (size_t i = 0; i < size; i++) {
      for (size_t j = 0; j < 8; j++) {
         buffer[i] & (1 << (7 - j)) ? output[i * 8 + j] = true : output[i * 8 + j] = false;
      }
   }

   delete buffer;
   return output;
}

///////////////////////////////////////////////////////////////////////////////
// Write binary to file
///////////////////////////////////////////////////////////////////////////////

void
BinaryUtils::writeBinary(const std::string& outputPath, const bitSet& data)
{
   std::ofstream out{ outputPath, std::ofstream::binary };
   size_t length = data.size() % 8 ? data.size() + 8 - data.size() % 8 : data.size();
   std::vector<char> buffer(length / 8);

#pragma omp parallel for
   for (size_t i = 0; i < length; i += 8)
      for (int j = 0; j < 8 && i + j < data.size(); ++j)
         buffer[size_t(i / 8)] += data[i + j] * pow(2, 7 - j);

   out.write(&buffer[0], buffer.size());

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
   size_t size = std::ceil(log2(number + 1));
   size_t finalSize = size > numBits ? size : numBits;
   BinaryUtils::bitSet result(finalSize);

   size_t pos = 0;
   for (size_t i = number % 2; number > 0 && pos < finalSize; number /= 2, i = number % 2, ++pos) {
      i ? result[pos] = 1 : result[pos] = 0;
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Get statistics of binary data
// The keys of the map are hash values of the symbols
///////////////////////////////////////////////////////////////////////////////

CodeProbabilityMap
BinaryUtils::getStatistics(bitSet data, size_t symbolSize)
{
   if (data.size() % symbolSize != 0) {
      throw std::runtime_error(
        "The symbolsize does not correspond to the data size! You may need to "
        "change the symbolsize to 8 or 16.");
   }

   CodeProbabilityMap result;
   for (size_t i = 0; i < data.size(); i += symbolSize) {
      result[slice(data, i, symbolSize)] += 1.0 / (data.size() / symbolSize);
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
///////////////////////////////////////////////////////////////////////////////

void
BinaryUtils::findUnusedSymbol(const std::map<bitSet, bitSet>& encodingMap,
                              bitSet& result,
                              size_t symbolSize)
{
   bitSet current(symbolSize);
   size_t n = 1;
   while (symbolSize >= log2(n) + 1 && encodingMap.find(current) != encodingMap.end()) {
      current = BinaryUtils::convertToBitSet(n, symbolSize);
      ++n;
   }
   if (encodingMap.find(current) == encodingMap.end()) {
      result = bitSet(current);
   }
}

///////////////////////////////////////////////////////////////////////////////
// Find unused symbol
///////////////////////////////////////////////////////////////////////////////

void
BinaryUtils::findUnusedSymbol(const bitSet& data, bitSet& result, size_t symbolSize)
{
   boost::unordered_set<bitSet> symbols;
   int idx = 0;
   while (idx + symbolSize <= data.size()) {
      symbols.emplace(slice(data, idx, symbolSize));
      idx += symbolSize;
   }

   idx = pow(2, symbolSize) - 1;
   bitSet current = BinaryUtils::convertToBitSet(idx, symbolSize);
   while (idx >= 0 && std::find(symbols.begin(), symbols.end(), current) != symbols.end()) {
      current = BinaryUtils::convertToBitSet(idx, symbolSize);
      --idx;
   }
   if (std::find(symbols.begin(), symbols.end(), current) == symbols.end()) {
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

///////////////////////////////////////////////////////////////////////////////
// Reverse bits
///////////////////////////////////////////////////////////////////////////////
void
BinaryUtils::reverseBits(bitSet& b)
{
   size_t size = b.size();
   for (size_t i = 0; i < size / 2; ++i) {
      bool tmp = b[i];
      b[i] = b[size - i - 1];
      b[size - i - 1] = tmp;
   }
}

bitSet
BinaryUtils::copyReverseBits(const bitSet& b)
{
   bitSet result(b.size());
   for (int i = b.size() - 1; i >= 0; --i)
      result[b.size() - i - 1] = b[i];
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// append
///////////////////////////////////////////////////////////////////////////////
void
BinaryUtils::append(bitSet& to, const bitSet& from)
{
   for (size_t i = 0; i < from.size(); ++i)
      to.push_back(from[i]);
}

///////////////////////////////////////////////////////////////////////////////
// assign
///////////////////////////////////////////////////////////////////////////////
void
BinaryUtils::assign(bitSet& to,
                    const bitSet& from,
                    size_t startIdx_to,
                    size_t numBits,
                    size_t startIdx_from)
{
   for (size_t i = 0; i + startIdx_to < to.size() && i + startIdx_from < to.size() && i < numBits;
        ++i)
      to[startIdx_to + i] = from[startIdx_from + i];
}

///////////////////////////////////////////////////////////////////////////////
// slice
// Get slice from bitSet
///////////////////////////////////////////////////////////////////////////////

bitSet
BinaryUtils::slice(const bitSet& b, size_t startIdx, size_t numBits)
{
   bitSet result(numBits);

   for (size_t i = startIdx; i < b.size() && i < startIdx + numBits; ++i) {
      result[i - startIdx] = b[i];
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////ű
// countZeros
// Count maximum number of consequtive zero bits
///////////////////////////////////////////////////////////////////////////////

size_t
BinaryUtils::countZeros(const bitSet& b)
{
   return b.size() - b.count();
}

///////////////////////////////////////////////////////////////////////////////
// findMostZeros
// Find the position of maximum number of consequtive zeros
///////////////////////////////////////////////////////////////////////////////

size_t
BinaryUtils::findMostZeros(const bitSet& b)
{
   size_t maxCount = 0;
   size_t iCount = 0;
   size_t idx = b.size();

   for (size_t i = 0; i < b.size(); ++i) {
      if (!b[i])
         ++iCount;
      else {
         if (iCount > maxCount) {
            maxCount = iCount;
            idx = i - iCount;
         }
         iCount = 0;
      }
   }

   if (iCount > maxCount) {
      idx = b.size() - iCount;
   }
   return idx;
}

///////////////////////////////////////////////////////////////////////////////
// serialize - common
///////////////////////////////////////////////////////////////////////////////

bitSet
BinaryUtils::serialize(std::vector<bitSet> data, size_t numBytes)
{
   bitSet result;
   for (const bitSet& b : data) {
      auto bSize = convertToBitSet(b.size(), numBytes * 8);
      if (bSize.size() != numBytes * 8)
         throw std::runtime_error("Incorrect width for indicating the data size!");

      append(result, bSize);
      append(result, b);
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// deserialize - common
///////////////////////////////////////////////////////////////////////////////

std::vector<bitSet>
BinaryUtils::deserialize(const bitSet& data, size_t numBytes)
{
   std::vector<bitSet> result;

   bitSet currentSize = slice(data, 0, numBytes * 8);
   size_t currentIdx = numBytes * 8;
   while (currentIdx + currentSize.to_ulong() <= data.size() && currentSize.to_ulong() > 0) {
      result.push_back(slice(data, currentIdx, currentSize.to_ulong()));
      currentIdx += currentSize.to_ulong();

      if (currentIdx + numBytes * 8 > data.size()) {
         break;
      }
      currentSize = slice(data, currentIdx, numBytes * 8);
      currentIdx += numBytes * 8;
   }

   return result;
}
