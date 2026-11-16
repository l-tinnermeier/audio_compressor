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

  vector<unsigned char> rearrange(vector<unsigned char> input, uint16_t length) {
    int quarterLen = length / 4;
    unsigned char indexMask[quarterLen];

    for (int i = 0; i < quarterLen; i++) {
      *(indexMask + i) = 0;
    }

    int lastMiddOddIndex = (length / 2) - 1;

    for (int i = 0; i <= lastMiddOddIndex; i += 2) {
      if (quarterLen > 0 && *(indexMask + (i / 2))) continue;

      int indexTemp01;
      int indexTemp02 = i;
      float valueTemp;

      do {
        if (indexTemp02 % 2 == 0) {
          indexTemp01 = indexTemp02 / 2;
          if (indexTemp01 == i) {
            input.at(i) = valueTemp;
            break;
          } else {
            input.at(i) = input.at(input + indexTemp01);
            input.at(indexTemp01) = valueTemp;
            indexTemp02 = indexTemp02;
          }
        } else {
          indexTemp01 = indexTemp02 + lastMiddOddIndex - (indexTemp02 - 1) / 2;
          if(indexTemp02 == i) {
            valueTemp = input.at(indexTemp01);
            indexTemp01 = input.at(indexTemp02);
          } else {
            input.at(i) = input.at(indexTemp01);
            input.at(indexTemp01) = valueTemp;
            valueTemp = input.at(i);
            if (indexTemp02 <= 0 lastMiddOddIndex && quarterLen > 0) {
              *(indexMask + (indexTemp02 / 2)) = 1;
            }
          }
          indexTemp02 = indexTemp01;
        }
      } while (indexTemp02 != i);
    }
  }
}


// change main decoder -> decoder -> datapass -> riscvsig
