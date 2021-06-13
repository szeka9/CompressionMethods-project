#include "BinaryUtils.hh"
#include "EncoderChain.hh"
#include "HuffmanTransducer.hh"
#include "MarkovEncoder.hh"
#include "Padder.hh"

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

using namespace BinaryUtils;

#define DEF_SYMBOLSIZE 16             // Note: does not work for odd byte sizes
#define DEF_PROBABILITY_THRESHOLD 0.4 // State transitions with >40% probability
#define DEF_NUM_SLICES 8
#define DEF_HUFF_THREADS 8

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
demo(const std::string& inputName, const std::string& outputName)
{
   // Generate or read binary #####################################
   auto t1 = std::chrono::high_resolution_clock::now();
   bitSet inputData = readBinary(inputName, 0);
   // bitSet inputData = getExpRandomBitStream(10000000);
   auto t2 = std::chrono::high_resolution_clock::now();
   printDurationMessage("Reading/generating the input", t1, t2);

   // Statistics & tree ##########################################
   t1 = std::chrono::high_resolution_clock::now();
   HuffmanTransducer h(inputData, DEF_SYMBOLSIZE);
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

   MarkovEncoder m(inputData, DEF_SYMBOLSIZE, DEF_PROBABILITY_THRESHOLD);

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
   HuffmanTransducer h2(markovEncoded, DEF_SYMBOLSIZE);
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

   float normalSize = h.getTableSize() + encoded.size();
   float precompressedSize = h2.getTableSize() + encoded2.size() + m.getTableSize();

   printConsoleLine("File size (without serialization)");
   std::cout << "File size: " << inputData.size() / 8000.0 << " KB" << std::endl
             << "Compressed size (original) with encoding table: " << (normalSize) / 8000.0 << " KB"
             << std::endl
             << "Compressed size (precompressed) with encoding table: "
             << (precompressedSize) / 8000.0 << " KB" << std::endl;

   t1 = std::chrono::high_resolution_clock::now();
   writeBinary(outputName, markovDecoded);
   t2 = std::chrono::high_resolution_clock::now();
   printDurationMessage("Writing into file", t1, t2);

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

   // Markov
   MarkovEncoder m(inputData, DEF_SYMBOLSIZE, DEF_PROBABILITY_THRESHOLD);
   auto markovEncoded = m.encode(inputData);
   auto serializedMarkov = m.serialize();

   // Huffman
   HuffmanTransducer h(markovEncoded, DEF_SYMBOLSIZE, DEF_HUFF_THREADS);
   auto encoded = h.encode(markovEncoded);
   auto serializedHuffman = h.serialize();

   // Serialize
   std::vector<bitSet> b;
   b.push_back(serializedHuffman);
   b.push_back(serializedMarkov);
   b.push_back(encoded);

   bitSet serialized_all = serialize(b, 4);
   writeBinary(outputName, serialized_all);
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

   HuffmanTransducer* h = HuffmanTransducer::deserializerFactory(serialized[0]);
   MarkovEncoder* m = MarkovEncoder::deserializerFactory(serialized[1]);

   auto huffmanDecoded = h->decode(serialized[2]);
   auto markovDecoded = m->decode(huffmanDecoded);

   delete h;
   delete m;

   writeBinary(outputName, markovDecoded);
}

///////////////////////////////////////////////////////////////////////////////
// chainEncode
///////////////////////////////////////////////////////////////////////////////

void
chainEncode(const std::string& inputName, const std::string& outputName)
{

   bitSet inputData = readBinary(inputName, 0);

   auto m = std::make_unique<MarkovEncoder>(DEF_SYMBOLSIZE, DEF_PROBABILITY_THRESHOLD);
   auto h = std::make_unique<HuffmanTransducer>(DEF_SYMBOLSIZE, DEF_HUFF_THREADS);
   auto p = std::make_unique<Padder>(Padder::PaddingType::WholeBytes);

   // EnocderChain
   EncoderChain c;
   c.addEncoder(std::move(m));
   c.addEncoder(std::move(h));
   c.addEncoder(std::move(p));

   auto encoded = c.encode(inputData);

   std::vector<bitSet> b = { c.serialize(), encoded };
   writeBinary(outputName, serialize(b, 4));
}

///////////////////////////////////////////////////////////////////////////////
// chainDecode
///////////////////////////////////////////////////////////////////////////////

void
chainDecode(const std::string& inputName, const std::string& outputName)
{

   bitSet inputData = readBinary(inputName, 0);

   std::vector<bitSet> serialized = deserialize(inputData, 4);
   if (serialized.size() != 2) {
      throw std::runtime_error("Cannot deserialize!");
   }

   auto d = std::unique_ptr<EncoderChain>(EncoderChain::deserializerFactory(serialized[0]));
   if (d && d->isValid()) {
      auto decoded = d->decode(serialized[1]);
      writeBinary(outputName, decoded);
   } else {
      throw std::runtime_error("Could not create the deserializer.");
   }
}

///////////////////////////////////////////////////////////////////////////////
// chainSlicedEncode
///////////////////////////////////////////////////////////////////////////////

void
chainSlicedEncode(const std::string& inputName, const std::string& outputName)
{

   bitSet inputData = readBinary(inputName, 0);
   std::vector<bitSet> slices;
   for (size_t i = 0; i < DEF_NUM_SLICES; ++i) {
      slices.push_back(slice(
        inputData, (inputData.size() / DEF_NUM_SLICES) * i, inputData.size() / DEF_NUM_SLICES));
   }

   std::vector<bitSet> serialized(DEF_NUM_SLICES);
   std::vector<bitSet> encoded(DEF_NUM_SLICES);

#pragma omp parallel for
   for (size_t i = 0; i < DEF_NUM_SLICES; ++i) {
      auto n = std::make_unique<Padder>(Padder::PaddingType::EvenBytes);
      auto m = std::make_unique<MarkovEncoder>(DEF_SYMBOLSIZE, DEF_PROBABILITY_THRESHOLD);
      auto h = std::make_unique<HuffmanTransducer>(DEF_SYMBOLSIZE, DEF_HUFF_THREADS);

      // EnocderChain
      EncoderChain c;
      c.addEncoder(std::move(n));
      c.addEncoder(std::move(m));
      c.addEncoder(std::move(h));

      encoded[i] = c.encode(slices[i]);
      serialized[i] = c.serialize();
   }

   auto mergedSerialized = serialize(serialized, 4);
   auto mergedSlices = serialize(encoded, 4);
   auto merged = serialize(std::vector<bitSet>{ mergedSerialized, mergedSlices }, 4);

   writeBinary(outputName, merged);
}

///////////////////////////////////////////////////////////////////////////////
// chainSlicedDecode
///////////////////////////////////////////////////////////////////////////////

void
chainSlicedDecode(const std::string& inputName, const std::string& outputName)
{
   bitSet inputData = readBinary(inputName, 0);

   std::vector<bitSet> serialized = deserialize(inputData, 4);
   if (serialized.size() != 2) {
      throw std::runtime_error("Cannot deserialize!");
   }

   auto serializedEncoder = deserialize(serialized[0], 4);
   auto slices = deserialize(serialized[1], 4);

   if (serializedEncoder.size() != DEF_NUM_SLICES || slices.size() != DEF_NUM_SLICES) {
      throw std::runtime_error("Cannot deserialize!");
   }

   std::vector<bitSet> decodedSlices(DEF_NUM_SLICES);
#pragma omp parallel for
   for (size_t i = 0; i < DEF_NUM_SLICES; ++i) {
      auto d =
        std::unique_ptr<EncoderChain>(EncoderChain::deserializerFactory(serializedEncoder[i]));
      if (d && d->isValid()) {
         auto b = d->decode(slices[i]);
         decodedSlices[i] = b;
      } else {
         throw std::runtime_error("Could not create the deserializer.");
      }
   }

   bitSet merged;
   for (auto b : decodedSlices) {
      append(merged, b);
   }
   writeBinary(outputName, merged);
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
         demo(inputName, "demo_decoded");
      } else if (mode == "--encode") {
         auto t1 = std::chrono::high_resolution_clock::now();
         chainSlicedEncode(inputName, outputName);
         auto t2 = std::chrono::high_resolution_clock::now();
         printDurationMessage("Encoding", t1, t2);
      } else if (mode == "--decode") {
         auto t1 = std::chrono::high_resolution_clock::now();
         chainSlicedDecode(inputName, outputName);
         auto t2 = std::chrono::high_resolution_clock::now();
         printDurationMessage("Decoding", t1, t2);
      } else {
         std::cout << "Unrecognized option: " << mode << std::endl;
      }
   } catch (std::exception& E) {
      std::cout << E.what();
   }

   return 0;
}