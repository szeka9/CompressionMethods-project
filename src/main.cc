#include "BinaryUtils.hh"
#include "HuffmanTransducer.hh"

#include <chrono>
#include <iostream>
#include <string>

void printDurationMessage(const std::string& what,
    std::chrono::_V2::system_clock::time_point t1,
    std::chrono::_V2::system_clock::time_point t2)
{
   std::cout << what << " took "
             << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                    .count()
             << " milliseconds\n";
}

int main(int argc, char** argv)
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
      // Generate ###################################
      auto t1 = std::chrono::high_resolution_clock::now();
      BinaryUtils::bitSet generated = BinaryUtils::readBinary(inputName, 0);
      //BinaryUtils::bitSet generated = BinaryUtils::getExpRandomBitStream(100000000);
      auto t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Reading the input", t1, t2);

      // Statistics & tree ##########################
      t1 = std::chrono::high_resolution_clock::now();
      std::map<unsigned int, std::tuple<BinaryUtils::bitSet, double>> stats = BinaryUtils::getStatistics(generated, 8);
      HuffmanTransducer h(stats);
      std::cout << "----------------------------------------------------------------------------" << std::endl;
      std::cout << "The entropy is: " << h.getEntropy() << std::endl;
      std::cout << "The average code length is: " << h.getAvgCodeLength() << std::endl;
      std::cout << "----------------------------------------------------------------------------" << std::endl;
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Statistics and tree creation", t1, t2);

      // Encoding ###################################
      t1 = std::chrono::high_resolution_clock::now();
      BinaryUtils::bitSet encoded = h.encodeChunk(generated, 8);
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Encoding", t1, t2);

      // Decoding ###################################
      t1 = std::chrono::high_resolution_clock::now();
      h.decodeChunk(encoded);
      BinaryUtils::bitSet decoded;
      h.flushBuffer(decoded);
      t2 = std::chrono::high_resolution_clock::now();
      printDurationMessage("Decoding", t1, t2);

      // ############################################
      BinaryUtils::writeBinary(outputName, encoded, true);

      std::cout << "----------------------------------------------------------------------------" << std::endl;
      std::cout << "File size: " << generated.size() << std::endl;
      std::cout << "Compressed size: " << encoded.size() << std::endl;
      std::cout << "Compression ratio: " << float(generated.size()) / float(encoded.size()) << std::endl;

      if (generated != decoded) {
         std::cout << "Decoding is not successful!" << std::endl;
      } else {
         std::cout << "Decoding is successful!" << std::endl;
      }

   } catch (int errorCode) {
      std::cout << "The program was interrupted due to an error.";
   }

   return 0;
}