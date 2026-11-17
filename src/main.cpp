#include "wav.hpp"
#include "compression.hpp"
#include "decompression.hpp"

#include <iostream>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <string>
#include <algorithm>
#include <cmath>
#include <filesystem>

using namespace std;
  using std::string;
  using std::to_string;
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
  using std::max;
  using std::abs;
  using std::size_t;

  void runTUI();

int main() {

  runTUI();

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

    const auto monoBytes = audio::stereoToMono(file, dataSize, channels, bitDepth);


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
      cout << "Do you wish to save " << filename << " as [MONO]? (y/n): ";
      getline(cin, userResponse);

      if (userResponse == "y" || userResponse == "Y") {
        string newFileName;
        cout << "Enter a new filename: ";
        getline(cin, newFileName);

        audio::saveMonoWav(newFileName + ".wav", monoBytes, sampleRate, bitDepth);
      }
    }

    const auto referenceSamples = audio::decodeMonoPCM(monoBytes, bitDepth);


    if (referenceSamples.empty()) { throw runtime_error("The recording contains no audio samples"); }

    const double durationSeconds = static_cast<double>(referenceSamples.size()) / sampleRate;
    
    cout << "=== Compressing to Mono === " << endl;
    cout << "Sample rate: " <<  sampleRate << " Hz" << endl;
    cout << "Channels: 1" << endl;
    cout << "Bit depth: " << bitDepth << endl;
    cout << "Sample count: " << referenceSamples.size() << endl;
    cout << "Duration: " << durationSeconds << " seconds" << endl;
    cout << "Audio data size: " << monoBytes.size() << " bytes" << endl << endl;


    // Haar experiment
    cout << "=== Haar ===" << endl;
    
    for (unsigned int levels : {1u, 3u, 5u}) {
      auto workingSamples = referenceSamples;

      const size_t multiple = size_t{1} << levels;
      size_t padding = (multiple - workingSamples.size() % multiple) % multiple;
      workingSamples.resize(workingSamples.size() + padding, 0.0);

      const auto coefficients = dwt::haarEncode(workingSamples, levels);

      cout << "Levels: " << levels << " level(s) ===" << endl;
      cout << "Original samples: " << referenceSamples.size() << endl;
      cout << "Padding samples: " << padding << endl;
      cout << "Coefficient count: " << coefficients.size() << endl;
      cout << "Extension: zero padding at the end" << endl;

      size_t bandSize = coefficients.size() / multiple;
      size_t offset = 0;

      cout << " ==Subbands (offset, size)==" << endl;
      cout << " A" << levels << ": " << offset << ", " << bandSize << endl;

      offset += bandSize;

      for (unsigned int level = levels; level > 0; level--) {
        cout << " D" << level << ": " << offset << ", " << bandSize << endl;
        offset += bandSize;
        bandSize *= 2;
      }

      for (double threshold : {0.0, 0.001, 0.005, 0.01}) {
        auto thresholded = coefficients;
        dwt::hardThreshold(thresholded, threshold);

        size_t zeros = 0;
        for (double c : thresholded) { if (c == 0.0) zeros++; }

        auto restored = dwt::haarDecode(thresholded, levels);
        restored.resize(referenceSamples.size());


        const std::filesystem::path directory = std::filesystem::path("samples") / ("haar_L" + to_string(levels));
        std::filesystem::create_directories(directory);
        const auto outputPath = directory / ("T" + to_string(threshold) + ".wav");
        Decoding24Bit::saveWav(restored, outputPath.string(), sampleRate);
        cout << "Saved " << outputPath.string() << endl;
        
        double squaredErr = 0.0;
        double maxErr = 0.0;

        for (size_t i = 0; i < referenceSamples.size(); i++) { 
          const double difference = referenceSamples[i] - restored[i]; 
          squaredErr += difference * difference;
          maxErr = max(maxErr, abs(difference));
        }

        const double mse = squaredErr / referenceSamples.size();
        const double zeroPercent = 100.0 * static_cast<double>(zeros) / thresholded.size();

        cout << " T =" << threshold << ", zeroes = " << zeroPercent << "%" << ", MSE: " << mse << ", Max Error: " << maxErr << endl;

      }
    }


    // Haar experiment
    cout << "=== Daubechies ===" << endl;
 
    for (unsigned int levels : {1u, 3u, 5u}) {
      auto workingSamples = referenceSamples;

      const size_t multiple = size_t{1} << levels;
      size_t padding = (multiple - workingSamples.size() % multiple) % multiple;
      workingSamples.resize(workingSamples.size() + padding, 0.0);

      const auto coefficients = dwt::daubechies4EncodeLevels(workingSamples, levels);

      cout << "Levels: " << levels << " level(s) ===" << endl;
      cout << "Original samples: " << referenceSamples.size() << endl;
      cout << "Padding samples: " << padding << endl;
      cout << "Coefficient count: " << coefficients.size() << endl;
      cout << "Extension: zero padding at the end" << endl;

      size_t bandSize = coefficients.size() / multiple;
      size_t offset = 0;

      cout << " ==Subbands (offset, size)==" << endl;
      cout << " A" << levels << ": " << offset << ", " << bandSize << endl;

      offset += bandSize;

      for (unsigned int level = levels; level > 0; level--) {
        cout << " D" << level << ": " << offset << ", " << bandSize << endl;
        offset += bandSize;
        bandSize *= 2;
      }

      for (double threshold : {0.0, 0.001, 0.005, 0.01}) {
        auto thresholded = coefficients;
        dwt::hardThreshold(thresholded, threshold);

        size_t zeros = 0;
        for (double c : thresholded) { if (c == 0.0) zeros++; }

        auto restored = dwt::daubechies4DecodeLevels(thresholded, levels);
        restored.resize(referenceSamples.size());


        const std::filesystem::path directory = std::filesystem::path("samples") / ("d4_L" + to_string(levels));
        std::filesystem::create_directories(directory);
        const auto outputPath = directory / ("T" + to_string(threshold) + ".wav");
        Decoding24Bit::saveWav(restored, outputPath.string(), sampleRate);
        cout << "Saved " << outputPath.string() << endl;
        
        double squaredErr = 0.0;
        double maxErr = 0.0;

        for (size_t i = 0; i < referenceSamples.size(); i++) { 
          const double difference = referenceSamples[i] - restored[i]; 
          squaredErr += difference * difference;
          maxErr = max(maxErr, abs(difference));
        }

        const double mse = squaredErr / referenceSamples.size();
        const double zeroPercent = 100.0 * static_cast<double>(zeros) / thresholded.size();

        cout << " T =" << threshold << ", zeroes = " << zeroPercent << "%" << ", MSE: " << mse << ", Max Error: " << maxErr << endl;

      }
    }



  } catch(const exception &e) {
    cerr << e.what() << endl;
    return 1;
  }

  return 0;
}
