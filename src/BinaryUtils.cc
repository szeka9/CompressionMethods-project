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

///////////////////////////////////////////////////////////////////////////////
// Read binary from file
///////////////////////////////////////////////////////////////////////////////

BinaryUtils::bitSet
BinaryUtils::readBinary(const std::string& fileName, size_t maxSize)
{
   struct stat results;
   size_t numBytes = 0;
   if (stat(fileName.c_str(), &results) == 0) {
      numBytes = results.st_size;
      std::cout << "File size: " << numBytes << " bytes." << std::endl;
   } else {
      throw std::runtime_error("An error has occured while reading the file.\n");
   }

   BinaryUtils::bitSet outputStream(numBytes * 8);
   char buffer[numBytes];

   std::ifstream in{ fileName, std::ifstream::binary };
   in.read(buffer, sizeof(buffer));

   if (!in) {
      throw std::runtime_error("An error has occured while reading the bit stream!\n");
   }

   for (size_t i = 0; i < numBytes; i++) {
      for (size_t j = 0; j < 8; j++) {
         buffer[i] & (1 << (7 - j)) ? outputStream[i * 8 + j] = true
                                    : outputStream[i * 8 + j] = false;
      }
   }

   if (0 < maxSize && maxSize < outputStream.size())
      outputStream.resize(maxSize);

   return outputStream;
}

///////////////////////////////////////////////////////////////////////////////
// Write binary to file
///////////////////////////////////////////////////////////////////////////////

void
BinaryUtils::writeBinary(const std::string& fileName,
                         const BinaryUtils::bitSet& bitStream,
                         bool paddToBytes)
{
   if (bitStream.size() % 8 != 0 && !paddToBytes) {
      throw std::runtime_error("Inappropriate length for a stream of bytes!");
   }
   std::ofstream out{ fileName };

   size_t streamLength = size_t(std::ceil(float(bitStream.size()) / 8));
   char buffer[streamLength] = { 0 };

   //#pragma omp parallel for
   for (size_t i = 0; i < streamLength * 8; i += 8)
      for (int j = 0; j < 8 && i + j < bitStream.size(); ++j)
         buffer[size_t(i / 8)] += bitStream[i + j] * pow(2, 7 - j);

   out.write(buffer, sizeof(buffer));

   if (!out.good()) {
      throw std::runtime_error("An error occured during writing!");
   }
}

///////////////////////////////////////////////////////////////////////////////
// Get statistics of binary data
// The keys of the map are hash values of the symbols
///////////////////////////////////////////////////////////////////////////////

std::map<unsigned int, std::tuple<BinaryUtils::bitSet, double>>
BinaryUtils::getStatistics(BinaryUtils::bitSet input, size_t symbolSize)
{
   if (input.size() % symbolSize != 0) {
      throw std::runtime_error("Symbolsize does not correspond to the input size! You may need to "
                               "change the symbolsize to 8 or 16.");
   }

   std::map<unsigned int, std::tuple<BinaryUtils::bitSet, double>> result;
   std::tuple<BinaryUtils::bitSet, double> stats;
   BinaryUtils::bitSet symbolBuffer(symbolSize);

   for (boost::dynamic_bitset<>::size_type i = 0; i < input.size(); i += symbolSize) {
      symbolBuffer.clear();
      for (boost::dynamic_bitset<>::size_type j = i; j < i + symbolSize; ++j) {
         symbolBuffer.push_back(input[j]);
      }
      std::get<0>(result[BinaryUtils::hashValue(symbolBuffer)]) = symbolBuffer;
      std::get<1>(result[BinaryUtils::hashValue(symbolBuffer)]) +=
        1.0 / (input.size() / symbolSize);
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Get random binary data in exponential distribution
///////////////////////////////////////////////////////////////////////////////

BinaryUtils::bitSet
BinaryUtils::getExpRandomBitStream(unsigned long int numBits, bool paddToBytes, size_t distribution)
{
   const int intervals = 256; // number of intervals

   std::random_device rd;
   std::mt19937 rnd_gen(rd());

   std::default_random_engine generator;
   std::exponential_distribution<double> rng(distribution);

   unsigned long int streamLength = numBits;
   if (paddToBytes && numBits % 8 != 0)
      streamLength += 8 - (numBits % 8);
   BinaryUtils::bitSet output(streamLength);

   for (unsigned long int i = 0; i < streamLength; i += 8) {
      double number = rng(generator);
      if (number < 1.0) {
         size_t current_num = size_t(intervals * number);
         if (current_num == 0)
            current_num = 1;
         for (size_t j = 0; j < 8; j++) {
            if (size_t(intervals * number) & (1 << (7 - j)))
               output[i + j] = true;
            else
               output[i + j] = false;
         }
      }
   }

   return output;
}

///////////////////////////////////////////////////////////////////////////////
// Generate probability map
// Returns  map of symbols with consequtive states and frequencies.
// The outer map contains all symbols.
// The inner map contains the possible state transitions with frequencies.
///////////////////////////////////////////////////////////////////////////////

std::map<BinaryUtils::bitSet, std::map<BinaryUtils::bitSet, float>>
BinaryUtils::computeMarkovChain(const BinaryUtils::bitSet& b, size_t symbolSize)
{
   std::map<BinaryUtils::bitSet, std::map<BinaryUtils::bitSet, float>> result;
   BinaryUtils::bitSet previousSymbol(symbolSize);
   BinaryUtils::bitSet currentSymbol(symbolSize);

   for (size_t j = 0; j < symbolSize && j < b.size(); ++j)
      previousSymbol[j] = b[j];

   for (size_t i = 0; i < b.size(); i += symbolSize) {
      for (size_t j = 0; j < symbolSize; ++j)
         currentSymbol[j] = b[i + j];

      std::map<BinaryUtils::bitSet, float> nextStates;
      result.emplace(previousSymbol, nextStates);
      result.at(previousSymbol).emplace(currentSymbol, 0);
      result.at(previousSymbol).at(currentSymbol) += 1;

      previousSymbol = currentSymbol;
   }

   // Print result
   /*
   for (auto it = result.begin(); it != result.end(); ++it) {
      auto symbol = it->first;
      for (size_t i = 0; i < symbol.size(); ++i) {
         std::cout << symbol[i];
      }
      std::cout << std::endl;
      auto nextStates = it->second;

      for (auto it2 = nextStates.begin(); it2 != nextStates.end(); ++it2) {
         symbol = it2->first;
         std::cout << "     ";
         for (size_t i = 0; i < symbol.size(); ++i) {
            std::cout << symbol[i];
         }
         std::cout << " | " << it2->second;
         std::cout << std::endl;
      }
   }*/

   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Create encoding map based on Markov chain
// Example: key=0001, value=0110 means that the next symbol to 0001 is 0110.
///////////////////////////////////////////////////////////////////////////////

std::map<BinaryUtils::bitSet, BinaryUtils::bitSet>
BinaryUtils::getMarkovEncodingMap(
  const std::map<BinaryUtils::bitSet, std::map<BinaryUtils::bitSet, float>>& markovChain,
  float probabiltyThreshold)
{
   std::map<BinaryUtils::bitSet, BinaryUtils::bitSet> result;
   for (auto it = markovChain.begin(); it != markovChain.end(); ++it) {
      auto currentSymbol = it->first;

      BinaryUtils::bitSet candidate;
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
      } else {
         // result.emplace(currentSymbol, currentSymbol);
      }
   }

   // Print result
   /*for (auto it = result.begin(); it != result.end(); ++it) {
      auto first = it->first;
      for (size_t i = 0; i < first.size(); ++i) {
         std::cout << first[i];
      }
      std::cout << " ";
      auto second = it->second;
      for (size_t i = 0; i < second.size(); ++i) {
         std::cout << second[i];
      }
      std::cout << std::endl;
   }*/

   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Encode data using the encoding map using XOR
///////////////////////////////////////////////////////////////////////////////
BinaryUtils::bitSet
BinaryUtils::markovEncode(const std::map<BinaryUtils::bitSet, BinaryUtils::bitSet>& mapping,
                          const BinaryUtils::bitSet& data,
                          size_t symbolSize)
{
   BinaryUtils::bitSet result(data.size());
   BinaryUtils::bitSet currentSymbol(symbolSize);
   BinaryUtils::bitSet mapped(symbolSize);

   for (size_t i = 0; i < data.size(); i += symbolSize) {
      for (size_t j = 0; j < symbolSize && j + i < data.size(); ++j) {
         currentSymbol[j] = data[j + i];
         result[i + j] = data[j + i] ^ mapped[j];
      }

      if (mapping.find(currentSymbol) != mapping.end()) {
         mapped = mapping.at(currentSymbol);
      } else {
         mapped = BinaryUtils::bitSet(symbolSize);
      }
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Decode data using the encoding map using XOR
///////////////////////////////////////////////////////////////////////////////
BinaryUtils::bitSet
BinaryUtils::markovDecode(const std::map<BinaryUtils::bitSet, BinaryUtils::bitSet>& mapping,
                          const BinaryUtils::bitSet& data,
                          size_t symbolSize)
{
   BinaryUtils::bitSet result(data.size());
   BinaryUtils::bitSet currentSymbol(symbolSize);
   BinaryUtils::bitSet mapped(symbolSize);

   for (size_t i = 0; i < data.size(); i += symbolSize) {
      for (size_t j = 0; j < symbolSize && j + i < data.size(); ++j) {
         currentSymbol[j] = data[j + i] ^ mapped[j];
         result[i + j] = currentSymbol[j];
      }

      if (mapping.find(currentSymbol) != mapping.end()) {
         mapped = mapping.at(currentSymbol);
      } else {
         mapped = BinaryUtils::bitSet(symbolSize);
      }
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
// Hash binary data
///////////////////////////////////////////////////////////////////////////////

unsigned int
BinaryUtils::hashValue(const BinaryUtils::bitSet& b)
{
   return boost::hash_value(b);
}