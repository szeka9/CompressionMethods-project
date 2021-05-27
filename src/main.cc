#include "BinaryUtils.hh"
#include "HuffmanTransducer.hh"

#include <chrono>
#include <exception>
#include <iostream>
#include <string>

using namespace BinaryUtils;

#define DEF_SYMBOLSIZE 16             // Note: does not work for odd byte sizes
#define DEF_PROBABILITY_THRESHOLD 0.4 // State transitions with >10% probability

void
printDurationMessage(const std::string& what,
                     std::chrono::_V2::system_clock::time_point t1,
                     std::chrono::_V2::system_clock::time_point t2)
{
   std::cout << what << " took "
             << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
             << " milliseconds\n";
}

void
printConsoleLine(const std::string header = "")
{
   size_t maxSize = 80;
   std::cout << std::endl;
   for (size_t i = 0; i < (maxSize - header.size()) / 2; ++i)
      std::cout << "-";
   std::cout << header;
   for (size_t i = 0; i < (maxSize - header.size()) / 2; ++i)
      std::cout << "-";
   std::cout << std::endl;
}

int
main(int argc, char** argv)
{
   std::string inputName;
   std::string outputName = "binary_output";

   if (argc < 2) {
      std::cout << "Missing parameters!" << std::endl;
      return 0;
   }
   if (argc > 2)
      outputName = std::string(argv[2]);
   inputName = std::string(argv[1]);

   try {
      // Generate or read binary #####################################
      auto t1 = std::chrono::high_resolution_clock::now();
      bitSet inputData = readBinary(inputName, 0);
      // bitSet inputData = getExpRandomBitStream(10000000);
      auto t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Reading/generating the input", t1, t2);

      // Statistics & tree ##########################################
      t1 = std::chrono::high_resolution_clock::now();
      auto stats = getStatistics(inputData, DEF_SYMBOLSIZE);
      HuffmanTransducer h(stats);
      t2 = std::chrono::high_resolution_clock::now();

      printConsoleLine("Original data");
      std::cout << "The entropy is: " << h.getEntropy() << std::endl
                << "The average code length is: " << h.getAvgCodeLength() << std::endl
                << "The size of the encoding table is: " << h.getTableSize() / 8000.0 << " KB"
                << std::endl
                << "Symbolsize (bits): " << DEF_SYMBOLSIZE << std::endl;

      printDurationMessage("Statistics and tree creation", t1, t2);

      // Precompression with Markov chain ###########################

      printConsoleLine("Precompression");
      auto markovChain = computeMarkovChain(inputData, DEF_SYMBOLSIZE);
      auto markovMap = getMarkovEncodingMap(markovChain, DEF_PROBABILITY_THRESHOLD);

      t1 = std::chrono::high_resolution_clock::now();
      auto markovEncoded = markovEncode(markovMap, inputData, DEF_SYMBOLSIZE);
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Precompression using Markov chains", t1, t2);

      t1 = std::chrono::high_resolution_clock::now();
      auto markovDecoded = markovDecode(markovMap, markovEncoded, DEF_SYMBOLSIZE);
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Decompression using Markov chains", t1, t2);

      if (inputData != markovDecoded) {
         std::cout << "Precompression failed!" << std::endl;
      }

      t1 = std::chrono::high_resolution_clock::now();
      auto stats2 = getStatistics(markovEncoded, DEF_SYMBOLSIZE);
      HuffmanTransducer h2(stats2);
      t2 = std::chrono::high_resolution_clock::now();

      printConsoleLine("Precompressed data");
      std::cout << "The entropy is: " << h2.getEntropy() << std::endl
                << "The average code length is: " << h2.getAvgCodeLength() << std::endl
                << "The size of the encoding table is: " << h2.getTableSize() / 8000.0 << " KB"
                << std::endl
                << "Symbolsize (bits): " << DEF_SYMBOLSIZE << std::endl;

      printDurationMessage("Statistics and tree creation", t1, t2);

      // Encoding ###################################################
      printConsoleLine("Encoding & Decoding");
      t1 = std::chrono::high_resolution_clock::now();
      bitSet encoded = h.encode(inputData, DEF_SYMBOLSIZE);
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Huffman encoding (original)", t1, t2);

      t1 = std::chrono::high_resolution_clock::now();
      bitSet encoded2 = h2.encode(markovEncoded, DEF_SYMBOLSIZE);
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Huffman encoding (precompressed)", t1, t2);

      // Decoding ###################################################
      t1 = std::chrono::high_resolution_clock::now();
      h.decode(encoded);
      t2 = std::chrono::high_resolution_clock::now();
      bitSet decoded;
      h.moveBuffer(decoded);
      printDurationMessage("Huffman decoding (original)", t1, t2);

      t1 = std::chrono::high_resolution_clock::now();
      h2.decode(encoded2);
      t2 = std::chrono::high_resolution_clock::now();
      bitSet decoded2;
      h2.moveBuffer(decoded2);
      printDurationMessage("Huffman decoding (precompressed)", t1, t2);

      // ############################################################
      // writeBinary(outputName, encoded, true);

      float normalSize = h.getTableSize() + encoded.size();
      float precompressedSize =
        h2.getTableSize() + encoded2.size() + markovMap.size() * 2 * DEF_SYMBOLSIZE;

      printConsoleLine("File size (without serialization)");
      std::cout << "File size: " << inputData.size() / 8000.0 << " KB" << std::endl
                << "Compressed size (original) with encoding table: " << (normalSize) / 8000.0
                << " KB" << std::endl
                << "Compressed size (precompressed) with encoding table: "
                << (precompressedSize) / 8000.0 << " KB" << std::endl;

      float originalRate = float(inputData.size()) / normalSize;
      float precompressedRate = float(inputData.size()) / precompressedSize;

      printConsoleLine("Compression ratio");
      std::cout << "Compression ratio (original): " << originalRate << std::endl
                << "Compression ratio (precompressed): " << precompressedRate << std::endl
                << "Compression improvement using precompression: "
                << ((precompressedRate / originalRate) - 1) * 100 << "%" << std::endl;

      // Compare encoded and decoded data ###########################
      printConsoleLine();
      auto markovDecoded_ = markovDecode(markovMap, decoded2, DEF_SYMBOLSIZE);
      if (inputData != decoded || inputData != markovDecoded_) {
         std::cout << "Decoding is not successful!" << std::endl;
      } else {
         std::cout << "Decoding is successful!" << std::endl;
      }
      std::cout << "Hash (original data): " << hashValue(inputData) << std::endl
                << "Hash (decoded data): " << hashValue(decoded) << std::endl
                << "Hash (decoded data, precompressed): " << hashValue(markovDecoded_) << std::endl;

   } catch (std::exception& E) {
      std::cout << E.what();
   }

   return 0;
}