# HuffmanTransducer
Compression methods project repository

## Description
<b>The implementation contains:</b>
1. a Huffman encoder based on transducers
2. a precompressor using Markov chains

The main idea of this project is a precompressor based Markov chains. I originally implemented a different algorithm, but due to programming errors, I didn't realize it was incorrect. The original algorithm was aiming to cluster binary values (0, 1) in an algorithmic way, so that the different values form larger clusters. However, certain steps of the algorithm caused information loss.

The precompressor uses a probabilistic model to reduce the entropy of the data.

<b>Pseudocode:</b>
  1. measure the frequency of each symbol transition (for consequent symbols)
  2. for each symbol, select the next most probable symbol based on the frequencies
  3. based on a frequency threshold, create an encoding map with two columns: if column1=symbolA, column2=symbolB then the consequent symbol of symbolA should be XORed with symbolB
  4. execute the encoding on the input data using the encoding table (starting from the first symbol)

The algorithm performs well during text compression since natural language can be well described with Markov models. I have also tested network traffic data (see also: samples).

Please note, that the current implementation does not support exporting the compressed data but information is displayed in the console (see also: Example output).

## How to run

  <i>cd src</i>
  
  <i>make</i>
  
   <i>./HuffmanTransducer ../samples/sip_flow.pcap</i>


Boost libraries are required to compile the code.

## Example output

![kép](https://user-images.githubusercontent.com/28252625/119221172-58d0e500-baee-11eb-9b95-c40ddedc2520.png)


