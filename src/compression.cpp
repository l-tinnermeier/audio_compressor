#include <iostream>
#include <vector>

using namespace std;
  using std::vector;

  using std::uint8_t;
  using std::uint16_t;
  using std::uint32_t;
  

namespace dwt {

  vector<unsigned char> haar(vector<unsigned char> input, uint16_t length, uint8_t level) {
    while (level--) {
      float ftemp01, ftemp02;
      for (int i = 0; i < length; i += 2) {
        fTemp01 = static_cast<uint32_t>(input.at(i));
        fTemp02 = static_cast<uint32_t>(input.at(i + 1));
        input.at(i) = (fTemp01 + fTemp02) / 1.414214f;
        input.at(i + 1) = (fTemp01 - fTemp02) / 1.414214f;
      }

      rearrange(input, length);
      length /= 2;
    }
  }

  vector<unsigned char> rearrange(vector<unsigned char>, uint16_t length);
}


// change main decoder -> decoder -> datapass -> riscvsig
