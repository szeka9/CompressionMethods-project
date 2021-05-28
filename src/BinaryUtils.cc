#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include "BinaryUtils.hh"

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
// Get statistics of binary data
// The keys of the map are hash values of the symbols
///////////////////////////////////////////////////////////////////////////////

std::map<size_t, std::tuple<bitSet, double>>
BinaryUtils::getStatistics(bitSet data, size_t symbolSize)
{
   if (data.size() % symbolSize != 0) {
      throw std::runtime_error("Symbolsize does not correspond to the data size! You may need to "
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
// Generate probability map
// Returns  map of symbols with consequtive states and frequencies.
// The outer map contains all symbols.
// The inner map contains the possible state transitions with frequencies.
///////////////////////////////////////////////////////////////////////////////

std::map<bitSet, std::map<bitSet, float>>
BinaryUtils::computeMarkovChain(const bitSet& data, size_t symbolSize)
{
   std::map<bitSet, std::map<bitSet, float>> result;
   bitSet previousSymbol(symbolSize);
   bitSet currentSymbol(symbolSize);

   for (size_t j = 0; j < symbolSize && j < data.size(); ++j)
      previousSymbol[j] = data[j];

   for (size_t i = 0; i < data.size(); i += symbolSize) {
      for (size_t j = 0; j < symbolSize; ++j)
         currentSymbol[j] = data[i + j];

      std::map<bitSet, float> nextStates;
      result.emplace(previousSymbol, nextStates);
      result.at(previousSymbol).emplace(currentSymbol, 0);
      result.at(previousSymbol).at(currentSymbol) += 1;

      previousSymbol = currentSymbol;
   }

   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Create encoding map based on Markov chain
// Example: key=0001, value=0110 means that the next symbol to 0001 is 0110.
///////////////////////////////////////////////////////////////////////////////

std::map<bitSet, bitSet>
BinaryUtils::getMarkovEncodingMap(const std::map<bitSet, std::map<bitSet, float>>& markovChain,
                                  float probabiltyThreshold)
{
   std::map<bitSet, bitSet> result;
   for (auto it = markovChain.begin(); it != markovChain.end(); ++it) {
      auto currentSymbol = it->first;

      bitSet candidate;
      float currentFrequency = 0;
      float sum = 0;

      auto nextStates = it->second;

      for (auto it2 = nextStates.begin(); it2 != nextStates.end(); ++it2) {
         if (it2->second > currentFrequency) {
            candidate = it2->first;
            currentFrequency = it2->second;
         }
         sum += it2->second;
      }
      if ((currentFrequency / sum) > probabiltyThreshold) {
         result.emplace(currentSymbol, candidate);
      }
   }

   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Encode data using the encoding map using XOR
///////////////////////////////////////////////////////////////////////////////
bitSet
BinaryUtils::markovEncode(const std::map<bitSet, bitSet>& mapping,
                          const bitSet& data,
                          size_t symbolSize)
{
   bitSet result(data.size());
   bitSet currentSymbol(symbolSize);
   bitSet mapped(symbolSize);

   for (size_t i = 0; i < data.size(); i += symbolSize) {
      for (size_t j = 0; j < symbolSize && j + i < data.size(); ++j) {
         currentSymbol[j] = data[j + i];
         result[i + j] = data[j + i] ^ mapped[j];
      }

      if (mapping.find(currentSymbol) != mapping.end()) {
         mapped = mapping.at(currentSymbol);
      } else {
         mapped = bitSet(symbolSize);
      }
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Decode data using the encoding map using XOR
///////////////////////////////////////////////////////////////////////////////
bitSet
BinaryUtils::markovDecode(const std::map<bitSet, bitSet>& mapping,
                          const bitSet& data,
                          size_t symbolSize)
{
   bitSet result(data.size());
   bitSet currentSymbol(symbolSize);
   bitSet mapped(symbolSize);

   for (size_t i = 0; i < data.size(); i += symbolSize) {
      for (size_t j = 0; j < symbolSize && j + i < data.size(); ++j) {
         currentSymbol[j] = data[j + i] ^ mapped[j];
         result[i + j] = currentSymbol[j];
      }

      if (mapping.find(currentSymbol) != mapping.end()) {
         mapped = mapping.at(currentSymbol);
      } else {
         mapped = bitSet(symbolSize);
      }
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Hash binary data
///////////////////////////////////////////////////////////////////////////////

size_t
BinaryUtils::hashValue(const bitSet& b)
{
   return boost::hash_value(b);
}