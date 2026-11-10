#include <iostream>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <string>

#include "wav.hpp"

using namespace std;
  using std::string;
  using std::ifstream;
  using std::istream;
  using std::ios;
  using std::runtime_error;
  using std::exception;
  using std::streampos;
  using std::memcmp;
  using std::streamoff;
  using std::vector;
  using std::streamsize;

int main() {
  string filename;
  cout << "Enter WAV path: ";
  getline(cin, filename);

  try {

    ifstream file(filename, ios::binary);
    if (!file) { throw runtime_error("Cannot open file"); }

    streampos fileSize = audio::getFileSize(filename);

    uint32_t sampleRate = audio::getFMTInfo(filename, 24, 4);
    uint16_t channels = audio::getFMTInfo(filename, 22, 2);
    uint16_t bitDepth = audio::getFMTInfo(filename, 34, 2);
    uint32_t dataSize = audio::getDataSize(file);
    uint16_t headerSize = static_cast<uint64_t>(fileSize) - dataSize;


    cout << "Sample rate: " << sampleRate << " Hz" << endl;
    cout << "Channels: " << channels << endl;
    cout << "Bit Depth: " << bitDepth << endl;

    cout << "Calculated Byte Rate: " << audio::calculateByteRate(sampleRate, channels, bitDepth) << endl;

    cout << "File size: " << fileSize << " bytes" << endl;
    cout << "Data size: " << dataSize << " bytes" << endl;
    cout << "Header size: " << headerSize << " bytes" << endl;

    string userResponse;

    if (channels == 2) { 
      cout << "[STEREO] file detected" << endl;
      cout << "Do you wish to convert " << filename << " to [MONO]? (y/n): ";
      getline(cin, userResponse);

      if (userResponse == "y" || userResponse == "Y") {
        auto monoData = audio::stereoToMono(file, dataSize, channels, bitDepth);
        string newFileName;
        cout << "Enter a new filename: ";
        getline(cin, newFileName);

        audio::saveMonoWav(newFileName + ".wav", monoData, sampleRate, bitDepth);
      }
    }


  } catch(const exception &e) {
    cerr << e.what() << endl;
    return 1;
  }

  return 0;
}
