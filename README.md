# Compression methods project repository

## Description
<b>The implementation contains:</b>
1. a Huffman encoder based on transducers
2. a precompressor using Markov chains
3. custom serialization format (for Huffman encoding)
4. a general framework to chain different encoders inheriting from a common interface

The idea of this project is precompressor based on Markov chains, combined with a Huffman encoder. The project aims to reduce the entropy of an arbitrary binary data source (before compression) so that an arbitrary entropy based compression algorithm can achieve better compression ratio. In orderd to achieve this, I implemented a general framework that can chain different steps of the compression sequentially. This way, it is simple to develop and test new algorithms as well. 

<b>Precompressor pseudocode:</b>
  1. measure the frequency of each symbol transition (for consequent symbols)
  2. for each symbol, select the next most probable symbol based on the frequencies
  3. based on a frequency threshold, create an encoding map with two columns: if column 1 = symbol A, column 2 = symbol B then the consequent symbol of symbol A should be XORed with symbol B. If not all symbols are used in the source data, then select one of them as a substituting symbol instead: at each transition this symbol will encode the consequtive symbol if the consequtive symbol matches the assumed symbol. This can further improve the compression ratio. Otherwise all consequtive symbols are encoded regardless if it matches the assumption.
  5. execute the encoding on the input data using the encoding table (starting from the first symbol)
  6. apply Huffman encoding (or other arbitrary compression algorithm)

The algorithm was mostly tested on network traffic data (.pcap), and in most cases there is noticable difference in the compression (15-20%). However, further testing and evaluation is needed since certain steps of the algorithm use predefined constants.

The implementation does support serializing the compressed data, however the file size should not exceed 6MB, since this feature is experimental.

## How to run

  <i>cd src</i>
  
  <i>make</i>
  
   <i>./HuffmanTransducer <--demo | --encode | --decode> <input path> <output path> (e.g. ./HuffmanTransducer --encode ../samples/text_data.txt output.bin)  </i>
  
  Run "<i>./HuffmanTransducer --demo</i>" to display information in the console.


Boost libraries are required to compile the code.

## Example output

![kép](https://user-images.githubusercontent.com/28252625/120709645-635f8700-c4bd-11eb-87d2-5a0c567fccaa.png)








