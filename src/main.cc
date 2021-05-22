#include "BinaryUtils.hh"
#include "HuffmanTransducer.hh"

#include <chrono>
#include <exception>
#include <iostream>
#include <string>

#define DEF_SYMBOLSIZE 32             // Note: does not work for odd byte sizes
#define DEF_PROBABILITY_THRESHOLD 0.1 // State transitions with >10% probability

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
      BinaryUtils::bitSet inputData = BinaryUtils::readBinary(inputName, 0);
      // BinaryUtils::getExpRandomBitStream(100000000);
      auto t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Reading/generating the input", t1, t2);

      // Statistics & tree ##########################################
      t1 = std::chrono::high_resolution_clock::now();
      auto stats = BinaryUtils::getStatistics(inputData, DEF_SYMBOLSIZE);
      HuffmanTransducer h(stats);
      t2 = std::chrono::high_resolution_clock::now();

      printConsoleLine("Original data");
      std::cout << "The entropy is: " << h.getEntropy() << std::endl;
      std::cout << "The average code length is: " << h.getAvgCodeLength() << std::endl;
      std::cout << "The size of the encoding table is: " << h.getRepresentationSize() / 8000.0
                << " KB" << std::endl;
      printDurationMessage("Statistics and tree creation", t1, t2);
      std::cout << "Symbolsize (bits): " << DEF_SYMBOLSIZE << std::endl;

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
      t2 = std::chrono::high_resolution_clock::now();

      printConsoleLine("Precompressed data");
      std::cout << "The entropy is: " << h2.getEntropy() << std::endl;
      std::cout << "The average code length is: " << h2.getAvgCodeLength() << std::endl;
      std::cout << "The size of the encoding table is: " << h2.getRepresentationSize() / 8000.0
                << " KB" << std::endl;
      printDurationMessage("Statistics and tree creation", t1, t2);
      std::cout << "Symbolsize (bits): " << DEF_SYMBOLSIZE << std::endl;

      // Encoding ###################################################
      printConsoleLine("Encoding");
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

      printConsoleLine("File size");
      std::cout << "File size: " << inputData.size() / 8000.0 << " KB" << std::endl;
      std::cout << "Compressed size (original) with encoding table: "
                << (h.getRepresentationSize() + encoded.size()) / 8000.0 << " KB" << std::endl;
      std::cout << "Compressed size (precompressed) with encoding table: "
                << (h2.getRepresentationSize() + encoded2.size()) / 8000.0 << " KB" << std::endl;

      printConsoleLine("Compression ratio");
      float originalRate =
        float(inputData.size()) / (float(encoded.size()) + h.getRepresentationSize());
      float precompressedRate =
        float(markovEncoded.size()) / (float(encoded2.size()) + h2.getRepresentationSize());
      std::cout << "Compression ratio (original): " << originalRate << std::endl;
      std::cout << "Compression ratio (precompressed): " << precompressedRate << std::endl;

      std::cout << "Compression improvement using precompression: "
                << ((precompressedRate / originalRate) - 1) * 100 << "%" << std::endl;

      // Compare encoded and decoded data ###########################
      printConsoleLine();
      auto markovDecoded_ = BinaryUtils::markovDecode(encodingMap, decoded2, DEF_SYMBOLSIZE);
      if (inputData != decoded || inputData != markovDecoded_) {
         std::cout << "Decoding is not successful!" << std::endl;
      } else {
         std::cout << "Decoding is successful!" << std::endl;
      }
      std::cout << "Hash (original data): " << BinaryUtils::hashValue(inputData) << std::endl;
      std::cout << "Hash (decoded data): " << BinaryUtils::hashValue(decoded) << std::endl;
      std::cout << "Hash (decoded data, precompressed): " << BinaryUtils::hashValue(markovDecoded_)
                << std::endl;

   } catch (std::exception& E) {
      std::cout << E.what();
   }

   return 0;
}