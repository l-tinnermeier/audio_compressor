#include <iostream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;
  using std::vector;

  using std::uint8_t;
  using std::uint16_t;
  using std::uint32_t;
  
  
namespace dwt {

   vector<unsigned char> rearrange(vector<unsigned char> input, uint16_t length);

  vector<unsigned char> haar(vector<unsigned char> input, uint16_t length, uint8_t level) {
    while (level--) {
      float fTemp01, fTemp02;
      for (int i = 0; i < length; i += 2) {
        fTemp01 = static_cast<uint32_t>(input.at(i));
        fTemp02 = static_cast<uint32_t>(input.at(i + 1));
        input.at(i) = (fTemp01 + fTemp02) / 1.414214f;
        input.at(i + 1) = (fTemp01 - fTemp02) / 1.414214f;
      }

      rearrange(input, length);
      length /= 2;
    }

    return input;
  }


}


// change main decoder -> decoder -> datapass -> riscvsig

// Part 3
namespace Encoding24Bit {
  const int F = 8;

  // Convert to 16-bit integer
  std::vector<int16_t> quantize(
    const std::vector<double>& coefficients) {
    std::vector<int16_t> quantized;

    for (double c : coefficients) {

      if (!std::isfinite(c)) {
        throw std::runtime_error("Invalid coefficient");
      }

      double q = std::round(c * (1 << F));

      // Prevent overflow
      q = std::clamp(q, -32768.0, 32767.0);

      quantized.push_back(static_cast<int16_t> (q));
    }
      return quantized;
    }
  
    // Encode coefficients into 24-bit records
    vector<pair<uint8_t, int16_t>> encode(
      const vector<int16_t>& quantized) {
      
    vector<pair<uint8_t, int16_t>> records;
    int zeroCount = 0;

    for (int16_t q : quantized) {

      if (q == 0) {
        zeroCount++;

        if (zeroCount == 256) {
          records.push_back({255, 0});
          zeroCount = 0;
        }
      }
      else {
        records.push_back({
          static_cast<uint8_t>(zeroCount), q
        });

        zeroCount = 0;
      }
    }

    // Handle remaining zeros
    if (zeroCount > 0) {
      records.push_back({
        static_cast<uint8_t>(zeroCount - 1), 0
      });
    }

    return records;
  }

  // Save 24-bit records to a binary file
  void saveRecords(
    const vector<pair<uint8_t, int16_t>>& records, const string& filename){

    ofstream file(filename, ios::binary);

    if(!file){
      throw runtime_error("File could not be opened");
    }

    for(const auto& record : records){
      uint8_t zeroCount = record.first;
      uint16_t coefficient = static_cast<uint16_t>(record.second);

      // 8-bit zero count
      file.put(static_cast<char>(zeroCount));

      // 16-bit (little endian)
      file.put(static_cast<char>(coefficient & 0xFF));
      file.put(static_cast<char>((coefficient >> 8) & 0xFF));
    }

    if(!file){
      throw runtime_error("Error writing compressed file");
    }
  }
    void saveMetadata(
        size_t coefficientCount, const string& filename){
      
      ofstream file(filename);

      if(!file){
        throw runtime_error("Could not open metadata file");
      }
    
      file << "F=" << F << '\n';
      file << "coefficientCount=" << coefficientCount << '\n';

      if(!file){
        throw runtime_error("Error writing metadata");
    }
  }
      
  // Calculate percentage of zero coefficients
  double zeroPercentage(const vector<int16_t>& quantized) {

    if (quantized.empty()) {
      return 0.0;
    }

    int zeros = 0;

    for (int16_t q : quantized) {
      if (q == 0) {
        zeros++;
      }
    }

    return (100.0 * zeros) / quantized.size();
  }

  // Calculate compressed payload size in bits
  size_t payloadBits(
      const vector<pair<uint8_t, int16_t>>& records) {

    return records.size() * 24;
  }

  // Calculate compression ratio
  double compressionRatio(
      const vector<int16_t>& quantized,
      const vector<pair<uint8_t, int16_t>>& records) {

    if (records.empty()) {
      return 0.0;
    }

    double originalBits = quantized.size() * 16.0;
    double compressedBits = payloadBits(records);

    return originalBits / compressedBits;
  }
}
