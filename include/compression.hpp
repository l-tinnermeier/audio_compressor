#include <vector>
#include <utility>
#include <cstdint>
#include <cstddef>
#include <string>

namespace dwt {
  std::vector<double> haarEncode(std::vector<double>& input, std::uint16_t levels);
  std::vector<std::pair<double, double>> daubechies4Encode(const std::vector<double>& input);
  std::vector<double> daubechies4EncodeLevels(std::vector<double> input, unsigned int levels);
  void hardThreshold(std::vector<double>& coefficients, double threshold);
}



namespace Encoding24Bit {

    const int F = 8;

    std::vector<int16_t> quantize(
        const std::vector<double>& coefficients);

    std::vector<std::pair<uint8_t, int16_t>> encode(
        const std::vector<int16_t>& quantized);

    void saveRecords(
        const std::vector<std::pair<uint8_t, int16_t>>& records,
        const std::string& filename);

    void saveMetadata(
        size_t coefficientCount,
        size_t originalSampleCount,
        uint32_t sampleRate,
        uint16_t bitDepth,
        const std::string& wavelet,
        unsigned int levels,
        const std::string& filename);

    double zeroPercentage(
        const std::vector<int16_t>& quantized);

    size_t payloadBits(
        const std::vector<std::pair<uint8_t, int16_t>>& records);

    double compressionRatio(
        const std::vector<int16_t>& quantized,
        const std::vector<std::pair<uint8_t, int16_t>>& records);
}
