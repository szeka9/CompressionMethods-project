#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include "BinaryUtils.hh"
#include <boost/functional/hash.hpp>
namespace boost {
template <typename B, typename A>
std::size_t hash_value(const boost::dynamic_bitset<B, A>& a)
{
   std::size_t res = hash_value(a.m_num_bits);
   boost::hash_combine(res, a.m_bits);
   return res;
}
}

#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <sys/stat.h>

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
      std::cout << "An error has occured while reading the file.\n";
      throw 1;
   }

   BinaryUtils::bitSet outputStream(numBytes * 8);
   char buffer[numBytes];

   std::ifstream in { fileName, std::ifstream::binary };
   in.read(buffer, sizeof(buffer));

   if (!in) {
      std::cout << "An error has occured while reading the bit stream!\n";
      throw 1;
   }

   //#pragma omp parallel for
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

void BinaryUtils::writeBinary(const std::string& fileName, const BinaryUtils::bitSet& bitStream, bool paddToBytes)
{

   if (bitStream.size() % 8 != 0 && !paddToBytes) {
      std::cout << "Inappropriate length for a stream of bytes!";
      throw 1;
   }
   std::ofstream out { fileName };

   size_t streamLength = size_t(std::ceil(float(bitStream.size()) / 8));
   char buffer[streamLength] = { 0 };

   //#pragma omp parallel for
   for (size_t i = 0; i < streamLength * 8; i += 8)
      for (int j = 0; j < 8 && i + j < bitStream.size(); ++j)
         buffer[size_t(i / 8)] += bitStream[i + j] * pow(2, 7 - j);

   out.write(buffer, sizeof(buffer));

   if (!out.good()) {
      std::cout << "An error occured during writing!";
      throw 1;
   }
}

///////////////////////////////////////////////////////////////////////////////
// Get statistics of binary data
///////////////////////////////////////////////////////////////////////////////

std::map<unsigned int, std::tuple<BinaryUtils::bitSet, double>>
BinaryUtils::getStatistics(BinaryUtils::bitSet input, size_t symbolSize)
{
   if (input.size() % symbolSize != 0) {
      throw 1;
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
      std::get<1>(result[BinaryUtils::hashValue(symbolBuffer)]) += 1.0 / (input.size() / symbolSize);
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

   //#pragma omp parallel for
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
// Generate pattern
///////////////////////////////////////////////////////////////////////////////

BinaryUtils::bitSet
BinaryUtils::generatePattern(const std::vector<bool>& b, unsigned int size)
{
   BinaryUtils::bitSet result;
   for (size_t i = 0; i < size; ++i) {
      result.push_back(b[i % b.size()]);
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