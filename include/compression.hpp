#include <vector>
#include <cstdint>

namespace dwt {
  std::vector<unsigned char> haar(std::vector<unsigned char>,
                                  std::uint16_t length,
                                  std::uint8_t level);

  std::vector<unsigned char> rearrange(std::vector<unsigned char>,
                                       std::uint16_t length);
}
