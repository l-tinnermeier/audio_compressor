#include "compression.hpp"


#include <iostream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <ranges>

using namespace std;
  using std::vector;
  using std::pair;

  using std::uint8_t;
  using std::uint16_t;
  using std::uint32_t;
  
  
namespace dwt {

  // Little description of Haar here
  vector<double> haarEncode(vector<double>& input, uint16_t levels) {
    size_t active = input.size();

    for (unsigned int level = 0; level < levels; level++) {
      if (active < 2 || active % 2 != 0) { 
        throw invalid_argument("Input length must be divisible by 2");
      }

      active /= 2;
    }

    vector<double> temp(input.size());
    const double scale = 1.0 / sqrt(2.0);
    active = input.size();

    for (unsigned int level = 0; level < levels; level++) {
      size_t half = active / 2;

      for (size_t i = 0; i < half; i++) {
        const double left = input[i * 2];
        const double right = input[(i * 2) + 1];

        temp[i] = (left + right) * scale;
        temp[half + i] = (left - right) * scale;
      }

      for (size_t i = 0; i < active; i++) { input[i] = temp[i]; } 
      active = half;
    }

    return input;
  }

  // Daubechies apparently uses a set of pre-defined coefficients to quantize data
  //    This inplementation, D4, uses [1 + sqrt(3), 3 + sqrt(3), 3 - sqrt(3), 1 - sqrt(3)] / 4
  vector<pair<double, double>> daubechies4Encode(const vector<double>& input) {
    if (input.size() < 2 || input.size() % 2 != 0) {
      throw invalid_argument("Input must contain a positive, even number of samples");
    }

    const size_t half = input.size() / 2;
    const double sqrt3 = sqrt(3.0);
    const double scale = 1.0 / (4.0 * sqrt(2.0));

    const double a = (sqrt3 + 1.0) * scale;
    const double b = (sqrt3 + 3.0) * scale;
    const double c = (3.0 - sqrt3) * scale;
    const double e = (1.0 - sqrt3) * scale;

    vector<pair<double, double>> result(half);

    for (size_t i = 0; i < half; i++) {
      size_t next = (i + 1) % half;
      size_t previous = (i == 0) ? half - 1 : i - 1;

      const double approx = 
        a * input[2 * i] +
        b * input[2 * i + 1] +
        c * input[2 * next] +
        e * input[2 * next + 1];

      const double detail = 
        e * input[2 * previous] -
        c * input[2 * previous + 1] +
        b * input[2 * i] -
        a * input[2 * i + 1];

      result[i] = {approx, detail};
    }
    return result;
  }

  vector<double> daubechies4EncodeLevels(vector<double> input, unsigned int levels) {
    size_t active = input.size();

    for (unsigned int level = 0; level < levels; level++) {
      if (active < 2 || active % 2 != 0) { throw invalid_argument("Invalid length argument"); }
      active /= 2;
    }

    active = input.size();

    for (unsigned int level = 0; level < levels; level++) {
      vector<double> approx(input.begin(), input.begin() + active);
      const auto pairs = daubechies4Encode(approx);
      const size_t half = pairs.size();

      for (size_t i = 0; i < half; i++) {
        input[i] = pairs[i].first;
        input[half + i] = pairs[i].second;
      }

      active = half;
    }

    return input;
  }

  void hardThreshold(vector<double>& coefficients, double threshold) {
    if (!isfinite(threshold) || threshold < 0) { throw invalid_argument("Threshold must be finite and non-negative"); }

    for (double& c : coefficients) {if (abs(c) <= threshold) { c = 0.0; }}
  }
}


// change main decoder -> decoder -> datapass -> riscvsig

// Part 3
namespace Encoding24Bit {

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
    size_t coefficientCount,
    size_t originalSampleCount,
    uint32_t sampleRate,
    uint16_t bitDepth,
    const string& wavelet,
    unsigned int levels,
    const string& filename) {      
      ofstream file(filename);

      if(!file){
        throw runtime_error("Could not open metadata file");
      }
    
file << "F=" << F << '\n';
file << "coefficientCount=" << coefficientCount << '\n';
file << "originalSampleCount=" << originalSampleCount << '\n';
file << "sampleRate=" << sampleRate << '\n';
file << "bitDepth=" << bitDepth << '\n';
file << "wavelet=" << wavelet << '\n';
file << "levels=" << levels << '\n';

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
