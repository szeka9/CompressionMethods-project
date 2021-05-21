#include "BinaryUtils.hh"
#include "HuffmanTransducer.hh"

#include <chrono>
#include <exception>
#include <iostream>
#include <string>

#define DEF_SYMBOLSIZE 16 // Experimental: does not work for odd byte sizes
#define DEF_PROBABILITY_THRESHOLD 0.1

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
printConsoleLine()
{
   std::cout << "--------------------------------------------------------------" << std::endl;
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
      BinaryUtils::bitSet inputData = BinaryUtils::readBinary(inputName, 0);
      // BinaryUtils::bitSet inputData =
      // BinaryUtils::getExpRandomBitStream(100000000);
      auto t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Reading/generating the input", t1, t2);

      // Statistics & tree ##########################################
      t1 = std::chrono::high_resolution_clock::now();
      std::map<unsigned int, std::tuple<BinaryUtils::bitSet, double>> stats =
        BinaryUtils::getStatistics(inputData, DEF_SYMBOLSIZE);
      HuffmanTransducer h(stats);

      printConsoleLine();
      std::cout << "The entropy of the original data is: " << h.getEntropy() << std::endl;
      std::cout << "The average code length of the original data is: " << h.getAvgCodeLength()
                << std::endl;
      printConsoleLine();
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Statistics and tree creation of the original data", t1, t2);

      // Precompression with Markov chain ###########################
      auto markovChain = BinaryUtils::computeMarkovChain(inputData, DEF_SYMBOLSIZE);
      auto encodingMap = BinaryUtils::getMarkovEncodingMap(markovChain, DEF_PROBABILITY_THRESHOLD);
      auto markovEncoded = BinaryUtils::markovEncode(encodingMap, inputData, DEF_SYMBOLSIZE);
      auto markovDecoded = BinaryUtils::markovDecode(encodingMap, markovEncoded, DEF_SYMBOLSIZE);

      if (inputData != markovDecoded) {
         std::cout << "Precompression failed!" << std::endl;
      }

      t1 = std::chrono::high_resolution_clock::now();
      auto stats2 = BinaryUtils::getStatistics(markovEncoded, DEF_SYMBOLSIZE);
      HuffmanTransducer h2(stats2);

      printConsoleLine();
      std::cout << "The entropy of the precompressed data is: " << h2.getEntropy() << std::endl;
      std::cout << "The average code length of the precompressed data is: " << h2.getAvgCodeLength()
                << std::endl;
      printConsoleLine();
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Statistics and tree creation of the precompressed data", t1, t2);

      // Encoding ###################################################
      printConsoleLine();
      t1 = std::chrono::high_resolution_clock::now();
      BinaryUtils::bitSet encoded = h.encode(inputData, DEF_SYMBOLSIZE);
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Huffman encoding (original)", t1, t2);

      t1 = std::chrono::high_resolution_clock::now();
      BinaryUtils::bitSet encoded2 = h2.encode(markovEncoded, DEF_SYMBOLSIZE);
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Huffman encoding (precompressed)", t1, t2);

      // Decoding ###################################################
      t1 = std::chrono::high_resolution_clock::now();
      h.decode(encoded);
      BinaryUtils::bitSet decoded;
      h.moveBuffer(decoded);
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Huffman decoding (original)", t1, t2);

      t1 = std::chrono::high_resolution_clock::now();
      h2.decode(encoded2);
      BinaryUtils::bitSet decoded2;
      h2.moveBuffer(decoded2);
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Huffman decoding (precompressed)", t1, t2);

      // ############################################################
      // BinaryUtils::writeBinary(outputName, encoded, true);

      printConsoleLine();
      float originalRate = float(inputData.size()) / float(encoded.size());
      std::cout << "File size: " << inputData.size() << std::endl;
      std::cout << "Compressed size (original): " << encoded.size() << std::endl;
      std::cout << "Compression ratio (original): " << originalRate << std::endl;

      float precompressedRate = float(markovEncoded.size()) / float(encoded2.size());
      std::cout << "Compressed size (precompressed): " << encoded2.size() << std::endl;
      std::cout << "Compression ratio (precompressed): " << precompressedRate << std::endl;

      std::cout << "Compression improvement using precompression: "
                << ((precompressedRate / originalRate) - 1) * 100 << "%" << std::endl;

      // Compare encoded and decoded data ###########################
      if (inputData != decoded ||
          inputData != BinaryUtils::markovDecode(encodingMap, decoded2, DEF_SYMBOLSIZE)) {
         std::cout << "Decoding is not successful!" << std::endl;
      } else {
         std::cout << "Decoding is successful!" << std::endl;
      }

   } catch (std::exception& E) {
      std::cout << E.what();
   }

   return 0;
}