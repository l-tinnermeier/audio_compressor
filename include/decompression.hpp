#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <utility>

namespace dwt {
  std::vector<double> haarDecode(std::vector<double> coefficients, unsigned int levels);
  std::vector<double> daubechies4Decode(const std::vector<std::pair<double, double>>& coefficients);
  std::vector<double> daubechies4DecodeLevels(std::vector<double> coefficients, unsigned int levels);
}

namespace Decoding24Bit {
  struct Metadata {
    int F;
    std::size_t coefficientCount;
    std::size_t originalSampleCount;
    std::uint32_t sampleRate;
    std::uint16_t bitDepth;
    std::string wavelet;
    unsigned int levels;
  };
  void saveWav(const std::vector<double>& samples, const std::string& filename, std::uint32_t sampleRate);
  double calculateSNR(const std::vector<double>& original, const std::vector<double>& reconstructed);

  std::vector<double> reconstructAudio(
      const std::string& binaryFile,
      const std::string& metadataFile);

  void runDecompression(
      const std::string& binaryFile,
      const std::string& metadataFile,
      const std::string& outputFile);

}
