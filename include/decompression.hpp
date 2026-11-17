#include <vector>
#include <string>
#include <vector>

namespace dwt {
  std::vector<double> haarDecode(std::vector<double> coefficients, unsigned int levels);
  std::vector<double> daubechies4Decode(const std::vector<std::pair<double, double>>& coefficients);
}

namespace Decoding24Bit {
  void saveWav(const std::vector<double>& samples, const std::string& filename, std::uint32_t sampleRate);
}
