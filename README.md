Term project: Implementation of given compression and decompression schemes

Compression schemes with their format are as below:

Format of the Original Binaries
|000|Original Binary (32 bits)|

Format of the Run Length Encoding (RLE)
|001|Run Length Encoding (3 bits)|

Format of bitmask-based compression – starting location is counted from left/MSB
|010|Starting Location (5 bits)|Bitmask (4 bits)|Dictionary Index (4 bits)|

Format of the 1-bit Mismatch – mismatch location is counted from left/MSB
|011|Mismatch Location (5 bits)|Dictionary Index (4 bits)|

Format of the 2-bit consecutive mismatches – starting location is counted from left/MSB
|100|Starting Location (5 bits)|Dictionary Index (4 bits)|

Format of the 4-bit consecutive mismatches – starting location is counted from left/MSB
|101|Starting Location (5 bits)|Dictionary Index (4 bits)|

Format of the 2-bit mismatches anywhere – Mismatch locations (ML) are counted from left/MSB
|110|1st ML from left (5 bits)|2nd ML from left (5 bits)|Dictionary Index (4 bits)|

Format of the Direct Matching
|111|Dictionary Index (4 bits)|

Command Line and Input/Output Formats:
The simulator should be executed with the following command line.
Please use parameters “1” and “2” to indicate compression, and decompression, respectively.
./SIM 1 for compression
./SIM 2 decompression

Please hardcode the input and output files as follows:
1. Input file for your compression function: original.txt
2. Output produced by your compression function: cout.txt
3. Input file for your decompression function: compressed.txt
4. Output produced by your decompression function: dout.txt
