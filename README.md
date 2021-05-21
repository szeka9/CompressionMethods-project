# HuffmanTransducer
Compression methods project repository

## Description
The implementation contains:
1. a Huffman encoder based on transducers
2. a precompressor using Markov chains

The main idea of this project is a precompressor based Markov chains. I originally implemented a different algorithm, but due to programming errors, I didn't realize it was incorrect. The original algorithm was aiming to cluster binary values (0, 1) in an algorithmic way, so that the different values form larger clusters. However, certain steps of the algorithm cased information loss.

The precompressor uses a probabilistic model to reduce the entropy of the data.

Pseudocode:
  1. measure the frequency of each symbol transition (for consequent symbols)
  2. for each symbol, select the next most probable symbol based on the frequencies
  3. based on a frequency threshold, create an encoding map with two columns: if column1=symbolA, column2=symbolB then the consequent symbol of symbolA should be XORed with symbolB
  4. execute the encoding on the input data using the encoding table (starting from the first symbol)

## How to run

  cd src

  make

  ./HuffmanTransducer ../samples/sip_flow.pcap


Boost libraries are required to compile the code.

## Example output

File size: 171782 bytes.
Reading/generating the input took 8 milliseconds
--------------------------------------------------------------
The entropy of the original data is: 11.5383
The average code length of the original data is: 11.5815
--------------------------------------------------------------
Statistics and tree creation of the original data took 48 milliseconds
--------------------------------------------------------------
The entropy of the precompressed data is: 8.4445
The average code length of the precompressed data is: 8.48594
--------------------------------------------------------------
Statistics and tree creation of the precompressed data took 45 milliseconds
--------------------------------------------------------------
Huffman encoding (original) took 20 milliseconds
Huffman encoding (precompressed) took 16 milliseconds
Huffman decoding (original) took 24 milliseconds
Huffman decoding (precompressed) took 24 milliseconds
--------------------------------------------------------------
File size: 1374256
Compressed size (original): 994748
Compression ratio (original): 1.38151
Compressed size (precompressed): 728866
Compression ratio (precompressed): 1.88547
Compression improvement using precompression: 36.4789%
Decoding is successful!
