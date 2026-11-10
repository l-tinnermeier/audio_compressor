#ifndef AUDIO_COMPRESSOR_WAV_HPP
#define AUDIO_COMPRESSOR_WAV_HPP

#include <cstdint>
#include <istream>
#include <string>
#include <vector>

namespace audio {
  std::uint32_t getDataSize(std::istream& file);
  std::vector<unsigned char> stereoToMono(
      std::istream& file,
      std::uint32_t dataSize,
      std::uint16_t channels,
      std::uint16_t bitDepth);

  void saveMonoWav(
      const std::string& filename,
      const std::vector<unsigned char>& monoData,
      std::uint32_t sampleRate,
      std::uint16_t bitDepth);

  std::streampos getFileSize(const std::string& filename);

  std::uint32_t getFMTInfo(const std::string& filename,
                           int offset, 
                           int length);

  std::uint64_t calculateByteRate(std::uint32_t rate,
                                  std::uint16_t channels,
                                  std::uint16_t bitDepth);
} // namespace audio

#endif
