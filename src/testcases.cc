#include "BinaryUtils.hh"
#include "HuffmanTransducer.hh"
#include "MarkovEncoder.hh"

#include <iostream>
#include <vector>

using namespace BinaryUtils;

#define TEST_FUNCTION(function) addTestCase(function, #function)

#define DEF_SYMBOLSIZE 16              // Note: does not work for odd byte sizes
#define DEF_PROBABILITY_THRESHOLD 0.01 // State transitions with >10% probability

// BinaryUtils ################################################################
bool
readBinary_valid_rightSize()
{
   auto b = readBinary("../samples/text_data.txt", 1000);
   return b.size() == 1000;
}

bool
readBinary_invalid_exceptionThrown()
{
   try {
      auto b = readBinary("../samples/invalid", 0);
      return false;
   } catch (std::exception& E) {
      return true;
   }
}

bool
reverseBits_default_match()
{
   bool result = true;
   bitSet b(std::string("11001100"));
   reverseBits(b);
   result = result && b == bitSet(std::string("00110011"));

   b = bitSet(std::string("11111111"));
   reverseBits(b);
   result = result && b == bitSet(std::string("11111111"));

   return result;
}

bool
appendBits_default_match()
{
   bool result = true;
   bitSet b(std::string("111000"));
   appendBits(b, bitSet(std::string("01")));
   result = result && b == bitSet(std::string("10111000"));

   b = bitSet();
   appendBits(b, bitSet(std::string("01")));
   result = result && b == bitSet(std::string("10"));

   b = bitSet();
   appendBits(b, bitSet());
   result = result && b == bitSet();

   return result;
}

bool
findMostZeros_default_match()
{
   bool result = true;
   bitSet b(std::string("11011001"));
   size_t i = findMostZeros(b);
   result = result && i == 5;

   b = bitSet(std::string("00000001"));
   i = findMostZeros(b);
   result = result && i == 0;

   b = bitSet(std::string("11111110"));
   i = findMostZeros(b);
   result = result && i == 7;

   b = bitSet(std::string("00000001"));
   i = findMostZeros(b);
   result = result && i == 0;
   return result;
}

bool
countZeros_default_match()
{
   bool result = true;
   bitSet b(std::string("000101"));
   result = result && countZeros(b) == 4;

   b = bitSet(std::string("111000"));
   result = result && countZeros(b) == 3;

   b = bitSet(std::string("111"));
   result = result && countZeros(b) == 0;

   b = bitSet(std::string("00000"));
   result = result && countZeros(b) == 5;

   b = bitSet(std::string(""));
   result = result && countZeros(b) == 0;
   return result;
}

bool
sliceBitSet_default_match()
{
   bool result = true;
   bitSet b(std::string("0101110"));
   result = result && sliceBitSet(b, 0, 2) == bitSet(std::string("01"));
   result = result && sliceBitSet(b, 3, 4) == bitSet(std::string("1010"));
   result = result && sliceBitSet(b, 0, 7) == bitSet(std::string("0111010"));
   return result;
}

bool
convertToBitSet_default_match()
{
   bool result = true;
   bitSet b(std::string("101110"));
   result = result && convertToBitSet(46) == b;

   b = bitSet(std::string("000"));
   result = result && convertToBitSet(0, 3) == b;

   b = bitSet(std::string("1"));
   result = result && convertToBitSet(1) == b;

   return result;
}

// Encoders and serialization #################################################

bool
deserialize_huffman_encoding_match()
{

   bitSet inputData = readBinary("../samples/text_data.txt", 0);
   auto stats = getStatistics(inputData, DEF_SYMBOLSIZE);
   HuffmanTransducer h(stats, DEF_SYMBOLSIZE);

   bitSet serialized = h.serialize();
   bitSet encoded = h.encode(inputData);
   std::vector<bitSet> b;
   b.push_back(serialized);
   b.push_back(encoded);

   bitSet serialized_all = serialize(b, 3);
   auto bRes = deSerialize(serialized_all, 3);

   HuffmanTransducer deserilizedHuffman = HuffmanTransducer::deserialize(bRes[0]);

   /*printConsoleLine("Symbols");
   for (auto e : h.getEncodingMap()) {
      std::cout << e.first << " " << e.second << std::endl;
   }

   printConsoleLine("Deserialized symbols");
   for (auto e : deserilizedHuffman.getEncodingMap()) {
      std::cout << e.first << " " << e.second << std::endl;
   }*/

   // check serialization + decoding
   if (inputData != deserilizedHuffman.decode(bRes[1])) {
      std::cout << "Deserialization failed for decoding!" << std::endl;
      return false;
   }

   // check serialization + encoding
   if (h.encode(inputData) != deserilizedHuffman.encode(inputData)) {
      std::cout << "Deserialization failed for encoding!" << std::endl;
      return false;
   }

   return true;
}

bool
deserialize_markov_encoding_match()
{

   bitSet inputData = readBinary("../samples/text_data.txt", 0);
   auto stats = getStatistics(inputData, DEF_SYMBOLSIZE);
   HuffmanTransducer h(stats, DEF_SYMBOLSIZE);

   bitSet unusedSymbol;
   findUnusedSymbol(h.getEncodingMap(), unusedSymbol, DEF_SYMBOLSIZE);
   MarkovEncoder m(inputData, DEF_SYMBOLSIZE, DEF_PROBABILITY_THRESHOLD, unusedSymbol);

   if (MarkovEncoder::deserialize(m.serialize()).getEncodingMap() != m.getEncodingMap()) {
      std::cout << "Encoding map does not match!" << std::endl;
      return false;
   }
   return true;
}

// HuffmanTransducer ##########################################################

class TestExecutor
{
 public:
   TestExecutor()
   {
      TEST_FUNCTION(readBinary_valid_rightSize);
      TEST_FUNCTION(readBinary_invalid_exceptionThrown);

      TEST_FUNCTION(reverseBits_default_match);
      TEST_FUNCTION(appendBits_default_match);
      TEST_FUNCTION(findMostZeros_default_match);
      TEST_FUNCTION(countZeros_default_match);
      TEST_FUNCTION(sliceBitSet_default_match);
      TEST_FUNCTION(convertToBitSet_default_match);

      TEST_FUNCTION(deserialize_huffman_encoding_match);
      TEST_FUNCTION(deserialize_markov_encoding_match);
   }

   void addTestCase(bool (*testFunction)(), std::string name)
   {
      testFunctions.push_back(testFunction);
      testNames.push_back(name);
   }

   void execute()
   {
      auto functionIt = testFunctions.begin();
      auto nameIt = testNames.begin();
      for (; functionIt != testFunctions.end() && nameIt != testNames.end();
           ++functionIt, ++nameIt) {
         if (!(*functionIt)())
            std::cout << *nameIt << " failed." << std::endl;
         else
            std::cout << *nameIt << " passed." << std::endl;
      }
   }

 private:
   std::vector<bool (*)()> testFunctions;
   std::vector<std::string> testNames;
};

int
main(int argc, char** argv)
{
   TestExecutor t;
   t.execute();
   return 0;
}