#include <vector>
#include <string>
#include <vector>
#include<cstdint>
#include <cstddef>
#include <utility>

namespace dwt {
  std::vector<double> haarDecode(std::vector<double> coefficients, unsigned int levels);
  std::vector<double> daubechies4Decode(const std::vector<std::pair<double, double>>& coefficients);
}

namespace Decoding24Bit {
  
  struct Metadata {
    int F;
    size_t coefficientCount;
    size_t originalSampleCount;
    std::uint32_t sampleRate;
    std::uint16_t bitDepth;
    std::string wavelet;
    unsigned int levels;
  };

  Metadata readMetadata(const std::string& filename);


  void saveWav(const std::vector<double>& samples, const std::string& filename, std::uint32_t sampleRate);
  std::vector<std::pair<uint8_t, int16_t>> readRecords(
      const std::string& filename);

  std::vector<int16_t> decode(
      const std::vector<std::pair<uint8_t, int16_t>>& records);

  bool verify(
      const std::vector<int16_t>& original,
      const std::vector<int16_t>& decoded);

  std::vector<double> dequantize(
      const std::vector<int16_t>& quantized);

  std::vector<double> inverseHaar(
      std::vector<double> coefficients,
      size_t levels);

  double calculateMSE(
      const std::vector<double>& original,
      const std::vector<double>& reconstructed);

  double calculateSNR(
      const std::vector<double>& original,
      const std::vector<double>& reconstructed);

  std::vector<double> decompress(
      const std::string& binaryFile,
      const std::string& metadataFile);

  std::vector<double> reconstructAudio(
      const std::string& binaryFile,
      const std::string& metadataFile,
      size_t levels);

  void runDecompression(
      const std::string& binaryFile,
      const std::string& metadataFile,
      const std::string& outputFile,
      size_t levels,
      uint32_t sampleRate);

}
