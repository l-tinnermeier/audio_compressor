#include <iostream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <limits>
#include <algorithm>

using namespace std;

// Part 4
namespace Decoding24Bit {

  const int F = 8;

  // Read 24-bit records from binary file
  vector<pair<uint8_t, int16_t>> readRecords(
      const string& filename) {

    ifstream file(filename, ios::binary);

    if (!file) {
      throw runtime_error("Could not open compressed file");
    }

    vector<pair<uint8_t, int16_t>> records;

    while (true) {

      int zeroCount = file.get();

      if (zeroCount == EOF) {
        break;
      }

      int low = file.get();
      int high = file.get();

      if (low == EOF || high == EOF) {
        throw runtime_error("Incomplete 24-bit record");
      }

      uint16_t bits = static_cast<uint16_t>(
          low | (high << 8));

      int16_t coefficient = (bits < 0x8000)
          ? static_cast<int16_t>(bits)
          : static_cast<int16_t>(
              static_cast<int32_t>(bits) - 65536);

      records.push_back({
          static_cast<uint8_t>(zeroCount), coefficient
      });
    }

    return records;
  }

  // Decode 24-bit records
  vector<int16_t> decode(
      const vector<pair<uint8_t, int16_t>>& records) {

    vector<int16_t> quantized;

    for (const auto& record : records) {

      uint8_t zeroCount = record.first;
      int16_t coefficient = record.second;

      // Restore zeros
      for (int i = 0; i < zeroCount; i++) {
        quantized.push_back(0);
      }

      // Restore coefficient
      quantized.push_back(coefficient);
    }

    return quantized;
  }

  // Verify decoded coefficients
  bool verify(
      const vector<int16_t>& original,
      const vector<int16_t>& decoded) {

    if (original.size() != decoded.size()) {
      return false;
    }

    for (size_t i = 0; i < original.size(); i++) {

      if (original[i] != decoded[i]) {
        return false;
      }
    }

    return true;
  }

  // Dequantization
  vector<double> dequantize(
      const vector<int16_t>& quantized) {

    vector<double> coefficients;

    for (int16_t q : quantized) {

      double c = static_cast<double>(q) / (1 << F);

      coefficients.push_back(c);
    }

    return coefficients;
  }

  // Inverse Haar DWT
  vector<double> inverseHaar(
      vector<double> coefficients,
      size_t levels) {

    size_t n = coefficients.size();

    if (n == 0 || levels == 0) {
      return coefficients;
    }

    // Requires power-of-two coefficient count
    if ((n & (n - 1)) != 0) {
      throw runtime_error("Invalid coefficient count");
    }

    if (levels > static_cast<size_t>(log2(n))) {
      throw runtime_error("Invalid DWT level");
    }

    size_t length = n >> (levels - 1);

    if (length < 2) {
      throw runtime_error("Invalid DWT level");
    }

    while (length <= n) {

      vector<double> temp(length);
      size_t half = length / 2;

      for (size_t i = 0; i < half; i++) {

        double a = coefficients[i];
        double d = coefficients[half + i];

        temp[2 * i] = (a + d) / sqrt(2.0);
        temp[2 * i + 1] = (a - d) / sqrt(2.0);
      }

      for (size_t i = 0; i < length; i++) {
        coefficients[i] = temp[i];
      }

      if (length == n) {
        break;
      }

      length *= 2;
    }

    return coefficients;
  }

  // Calculate mean squared error
  double calculateMSE(
      const vector<double>& original,
      const vector<double>& reconstructed) {

    if (original.size() != reconstructed.size() ||
        original.empty()) {
      throw runtime_error("Invalid audio sample sizes");
    }

    double error = 0.0;

    for (size_t i = 0; i < original.size(); i++) {

      double difference = original[i] - reconstructed[i];
      error += difference * difference;
    }

    return error / original.size();
  }

  // Calculate signal-to-noise ratio
  double calculateSNR(
      const vector<double>& original,
      const vector<double>& reconstructed) {

    double signalPower = 0.0;

    for (double sample : original) {
      signalPower += sample * sample;
    }

    double mse = calculateMSE(original, reconstructed);

    if (mse == 0.0) {
      return numeric_limits<double>::infinity();
    }

    if (signalPower == 0.0) {
      return -numeric_limits<double>::infinity();
    }

    double averageSignalPower =
        signalPower / original.size();

    return 10.0 * log10(averageSignalPower / mse);
  }

  // Read metadata
  struct Metadata {
    int F;
    size_t coefficientCount;
  };

  Metadata readMetadata(const string& filename) {

    ifstream file(filename);

    if (!file) {
      throw runtime_error("Could not open metadata file");
    }

    Metadata metadata{};
    string line;

    if (!getline(file, line) ||
        line.rfind("F=", 0) != 0) {
      throw runtime_error("Invalid metadata F");
    }

    metadata.F = stoi(line.substr(2));

    if (metadata.F < 0 || metadata.F > 15) {
      throw runtime_error("Invalid fractional bits");
    }

    if (!getline(file, line) ||
        line.rfind("coefficientCount=", 0) != 0) {
      throw runtime_error("Invalid coefficient count");
    }

    metadata.coefficientCount = stoull(line.substr(17));

    return metadata;
  }

  // Read and decode compressed coefficients
  vector<double> decompress(
      const string& binaryFile,
      const string& metadataFile) {

    // Read metadata
    Metadata metadata = readMetadata(metadataFile);

    // Read 24-bit records
    auto records = readRecords(binaryFile);

    // Decode records into 16-bit integers
    auto quantized = decode(records);

    // Verify coefficient count
    if (quantized.size() != metadata.coefficientCount) {
      throw runtime_error("Coefficient count mismatch");
    }

    // Convert integers back to floating-point
    vector<double> coefficients;

    double scale = static_cast<double>(1 << metadata.F);

    for (int16_t q : quantized) {
      coefficients.push_back(q / scale);
    }

    return coefficients;
  }

  // Reconstruct audio samples
  vector<double> reconstructAudio(
      const string& binaryFile,
      const string& metadataFile,
      size_t levels) {

    // Read and decompress coefficients
    vector<double> coefficients =
        decompress(binaryFile, metadataFile);

    // Apply inverse Haar DWT
    vector<double> reconstructed =
        inverseHaar(coefficients, levels);

    return reconstructed;
  }

  // Save reconstructed audio as a WAV file
  void saveWav(
      const vector<double>& samples,
      const string& filename,
      uint32_t sampleRate) {

    ofstream file(filename, ios::binary);

    if (!file) {
      throw runtime_error("Could not create WAV file");
    }

    uint16_t channels = 1;
    uint16_t bitDepth = 16;
    uint16_t blockAlign = channels * (bitDepth / 8);
    uint32_t byteRate = sampleRate * blockAlign;
    uint32_t dataSize = static_cast<uint32_t>(
        samples.size() * blockAlign);

    // Write little-endian values
    auto write16 = [&](uint16_t value) {
      file.put(static_cast<char>(value & 0xFF));
      file.put(static_cast<char>((value >> 8) & 0xFF));
    };

    auto write32 = [&](uint32_t value) {
      for (int i = 0; i < 4; i++) {
        file.put(static_cast<char>((value >> (8 * i)) & 0xFF));
      }
    };

    // WAV header
    file.write("RIFF", 4);
    write32(36 + dataSize);
    file.write("WAVE", 4);

    file.write("fmt ", 4);
    write32(16);
    write16(1);
    write16(channels);
    write32(sampleRate);
    write32(byteRate);
    write16(blockAlign);
    write16(bitDepth);

    file.write("data", 4);
    write32(dataSize);

    // Write reconstructed samples
    for (double sample : samples) {

      // Assumes samples are normalized to [-1, 1]
      sample = clamp(sample, -1.0, 1.0);

      int32_t value = static_cast<int32_t>(
          round(sample * 32767.0));

      write16(static_cast<uint16_t>(value));
    }

    if (!file) {
      throw runtime_error("Error writing WAV file");
    }
  }

  // Complete decompression process
  void runDecompression(
      const string& binaryFile,
      const string& metadataFile,
      const string& outputFile,
      size_t levels,
      uint32_t sampleRate) {

    // Reconstruct audio samples
    vector<double> reconstructed =
        reconstructAudio(binaryFile, metadataFile, levels);

    // Save reconstructed WAV file
    saveWav(reconstructed, outputFile, sampleRate);

    cout << "Decompression complete!" << endl;
    cout << "Output file: " << outputFile << endl;
  }

}

