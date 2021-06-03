#include "BinaryUtils.hh"
#include "HuffmanTransducer.hh"
#include "MarkovEncoder.hh"

#include <chrono>
#include <exception>
#include <iostream>
#include <string>

using namespace BinaryUtils;

#define DEF_SYMBOLSIZE 16             // Note: does not work for odd byte sizes
#define DEF_PROBABILITY_THRESHOLD 0.4 // State transitions with >40% probability

///////////////////////////////////////////////////////////////////////////////
// utility functions
///////////////////////////////////////////////////////////////////////////////

void
printDurationMessage(const std::string& what,
                     std::chrono::_V2::system_clock::time_point t1,
                     std::chrono::_V2::system_clock::time_point t2)
{
   std::cout << what << " took "
             << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
             << " milliseconds" << std::endl;
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

///////////////////////////////////////////////////////////////////////////////
// demo
///////////////////////////////////////////////////////////////////////////////

void
demo(const std::string& inputName)
{
   // Generate or read binary #####################################
   auto t1 = std::chrono::high_resolution_clock::now();
   bitSet inputData = readBinary(inputName, 0);
   // bitSet inputData = getExpRandomBitStream(10000000);
   auto t2 = std::chrono::high_resolution_clock::now();
   printDurationMessage("Reading/generating the input", t1, t2);

   // Statistics & tree ##########################################
   t1 = std::chrono::high_resolution_clock::now();
   auto stats = getStatistics(inputData, DEF_SYMBOLSIZE);
   HuffmanTransducer h(stats, DEF_SYMBOLSIZE);
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

   bitSet unusedSymbol;
   findUnusedSymbol(h.getEncodingMap(), unusedSymbol, DEF_SYMBOLSIZE);

   MarkovEncoder m(inputData, DEF_SYMBOLSIZE, DEF_PROBABILITY_THRESHOLD, unusedSymbol);

   t1 = std::chrono::high_resolution_clock::now();
   auto markovEncoded = m.encode(inputData);
   t2 = std::chrono::high_resolution_clock::now();
   printDurationMessage("Precompression using Markov chains", t1, t2);

   t1 = std::chrono::high_resolution_clock::now();
   auto markovDecoded = m.decode(markovEncoded);
   t2 = std::chrono::high_resolution_clock::now();
   printDurationMessage("Decompression using Markov chains", t1, t2);

   if (inputData != markovDecoded) {
      std::cout << "Precompression failed!" << std::endl;
   }

   t1 = std::chrono::high_resolution_clock::now();
   auto stats2 = getStatistics(markovEncoded, DEF_SYMBOLSIZE);
   HuffmanTransducer h2(stats2, DEF_SYMBOLSIZE);
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
   bitSet encoded = h.encode(inputData);
   t2 = std::chrono::high_resolution_clock::now();
   printDurationMessage("Huffman encoding (original)", t1, t2);

   t1 = std::chrono::high_resolution_clock::now();
   bitSet encoded2 = h2.encode(markovEncoded);
   t2 = std::chrono::high_resolution_clock::now();
   printDurationMessage("Huffman encoding (precompressed)", t1, t2);

   // Decoding ###################################################
   t1 = std::chrono::high_resolution_clock::now();
   bitSet decoded = h.decode(encoded);
   t2 = std::chrono::high_resolution_clock::now();
   printDurationMessage("Huffman decoding (original)", t1, t2);

   t1 = std::chrono::high_resolution_clock::now();
   bitSet decoded2 = h2.decode(encoded2);
   t2 = std::chrono::high_resolution_clock::now();
   printDurationMessage("Huffman decoding (precompressed)", t1, t2);

   // ############################################################
   // writeBinary(outputName, markovEncoded, true);

   float normalSize = h.getTableSize() + encoded.size();
   float precompressedSize = h2.getTableSize() + encoded2.size() + m.getTableSize();

   printConsoleLine("File size (without serialization)");
   std::cout << "File size: " << inputData.size() / 8000.0 << " KB" << std::endl
             << "Compressed size (original) with encoding table: " << (normalSize) / 8000.0 << " KB"
             << std::endl
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
   auto markovDecoded_ = m.decode(decoded2);
   if (inputData != decoded || inputData != markovDecoded_) {
      std::cout << "Decoding is not successful!" << std::endl;
   } else {
      std::cout << "Decoding is successful!" << std::endl;
   }
   std::cout << "Hash (original data): " << hashValue(inputData) << std::endl
             << "Hash (decoded data): " << hashValue(decoded) << std::endl
             << "Hash (decoded data, precompressed): " << hashValue(markovDecoded_) << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
// encode
///////////////////////////////////////////////////////////////////////////////

void
encode(const std::string& inputName, const std::string& outputName)
{

   bitSet inputData = readBinary(inputName, 0);

   auto stats = getStatistics(inputData, DEF_SYMBOLSIZE);
   HuffmanTransducer h(stats, DEF_SYMBOLSIZE);

   // Markov
   bitSet unusedSymbol;
   findUnusedSymbol(h.getEncodingMap(), unusedSymbol, DEF_SYMBOLSIZE);
   MarkovEncoder m(inputData, DEF_SYMBOLSIZE, DEF_PROBABILITY_THRESHOLD, unusedSymbol);
   bitSet markovEncoded = m.encode(inputData);
   bitSet serializedMarkov = m.serialize();

   // Huffman
   stats = getStatistics(markovEncoded, DEF_SYMBOLSIZE);
   HuffmanTransducer h2(stats, DEF_SYMBOLSIZE);
   bitSet encoded = h2.encode(markovEncoded);
   bitSet serializedHuffman = h2.serialize();

   // Serialize
   std::vector<bitSet> b;
   b.push_back(serializedHuffman);
   b.push_back(serializedMarkov);
   b.push_back(encoded);

   bitSet serialized_all = serialize(b, 4);
   writeBinary(outputName, serialized_all, true);
}

///////////////////////////////////////////////////////////////////////////////
// decode
///////////////////////////////////////////////////////////////////////////////

void
decode(const std::string& inputName, const std::string& outputName)
{

   bitSet inputData = readBinary(inputName, 0);

   std::vector<bitSet> serialized = deserialize(inputData, 4);
   if (serialized.size() != 3) {
      throw std::runtime_error("Cannot deserialize!");
   }

   HuffmanTransducer h = HuffmanTransducer::deserialize(serialized[0]);
   MarkovEncoder m = MarkovEncoder::deserialize(serialized[1]);

   bitSet huffmanDecoded = h.decode(serialized[2]);
   bitSet markovDecoded = m.decode(huffmanDecoded);

   writeBinary(outputName, markovDecoded, true);
}

///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////

int
main(int argc, char** argv)
{
   std::string inputName = "../samples/text_data.txt";
   std::string outputName = "encoded_output";
   std::string mode = "--demo";

   if (argc < 2) {
      std::cout << "Missing parameters!" << std::endl;
      return 0;
   }

   mode = std::string(argv[1]);
   if (argc > 2) {
      inputName = std::string(argv[2]);
   }
   if (argc > 3) {
      outputName = std::string(argv[3]);
   }

   try {
      if (mode == "--demo") {
         demo(inputName);
      } else if (mode == "--encode") {
         encode(inputName, outputName);
      } else if (mode == "--decode") {
         decode(inputName, outputName);
      } else {
         std::cout << "Unrecognized option: " << mode << std::endl;
      }
   } catch (std::exception& E) {
      std::cout << E.what();
   }

   return 0;
}