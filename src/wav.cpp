#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>

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

  using std::uint16_t
  using std::uint32_t
  using std::uint64_t
// FUNCTIONS

namespace audio {

// *=== PARSING .WAV HELPERS ===* // 

// Reading "fmt" information
uint32_t getFMTInfo(const string &filename, const int offset, const int length) {
  ifstream file(filename, ios::binary);
  if (!file) {
    throw runtime_error("Error: Cannot open file " + filename);
  }

  file.seekg(offset, ios::beg);

  uint32_t info = 0;
  file.read(reinterpret_cast<char*>(&info), length);

  if (!file) { throw runtime_error("Error: Failed to read info from file"); };
  return info;
}

// Takes the sample rate, channels, and bits per sample, and finds the total byte rate
uint64_t calculateByteRate(const uint32_t rate, const uint16_t channels, const uint16_t bitDepth) {
  uint64_t byteRate = uint64_t{rate} * channels * bitDepth / 8;
  return byteRate;
}

// Reads the entire file size & returns it (in bytes)
streampos getFileSize(const string &filename) {
  ifstream file(filename, ios::binary | ios::ate);
  return file.tellg();
}

// Helper function to read individual samples
uint32_t readLE32(istream& file) {
  unsigned char b[4];

  if (!file.read(reinterpret_cast<char*>(b), 4)) {
    throw runtime_error("Unexpected end of .WAV file");
  }

  return uint32_t(b[0])
      | (uint32_t(b[1]) << 8)
      | (uint32_t(b[2]) << 16)
      | (uint32_t(b[3]) << 24);
}

// *=== STEREO TO MONO HELPERS ===* //

// Reads data chunk until it hits EOF
uint32_t getDataSize(istream& file) {
  char header[12];
  if (!file.read(header, sizeof(header)) || memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
    throw runtime_error("Non-standard RIFF/WAV file detected");
  }

  char id[4];
  while (file.read(id, sizeof(id))) {
    uint32_t size = readLE32(file);

    if (memcmp(id, "data", 4) == 0) {
      // Stream has moved to the start of the data samples
      return size;
    }

    // Skipping data chunk w/padding if chunk is odd
    file.seekg(static_cast<streamoff>(size) + (size % 2), ios::cur);
  }

  throw runtime_error("No 'data' chunk found");
}

vector<unsigned char> stereoToMono(istream& file, uint32_t dataSize, uint16_t channels, uint16_t bitDepth) {
  if (channels != 1 && channels != 2) { throw runtime_error("Expected stereo or mono audio"); }
  if (bitDepth != 8 && bitDepth != 16) { throw runtime_error("Only 8-bit and 16-bit PCM supported"); }

  const uint32_t bytesPerSample = bitDepth / 8;
  const uint32_t bytesPerFrame = channels * bytesPerSample;

  if (dataSize % bytesPerFrame != 0) { throw runtime_error("Incomplete audio frame"); }

  vector<unsigned char> data(dataSize);
  if (dataSize != 0 && !file.read(reinterpret_cast<char*>(data.data()), dataSize)) { throw runtime_error("Failed to read audio data"); }
  // Audio already mono
  if (channels == 1) { return data; }

  vector<unsigned char> mono(dataSize / 2);

  // Decoding a signed, little-endian 16-bit sample
  auto readSigned_16 = [](const unsigned char* p) -> int32_t {
    uint32_t value = uint32_t(p[0]) | (uint32_t(p[1]) << 8);
    return value >= 32768 ? static_cast<int32_t>(value) - 65536 : static_cast<int32_t>(value);
  };

  // Converting stereo audio to mono by averaging channels
  for (size_t input = 0, output = 0; input < data.size(); input += bytesPerFrame, output += bytesPerSample) {
    // 8-bit PCM samples are unsigned
    if (bitDepth == 8) {
      mono[output] = static_cast<unsigned char>((uint32_t(data[input]) + data[input + 1]) / 2);
    } else {
      int32_t left = readSigned_16(&data[input]);
      int32_t right = readSigned_16(&data[input + 2]);
      int32_t average = (left + right) / 2;
      
      // Encoding result back to two little-endian bytes
      uint32_t sample = static_cast<uint32_t>(average);
      mono[output] = static_cast<unsigned char>(sample & 0xFF);
      mono[output + 1] = static_cast<unsigned char>(sample >> 8);
    }
  }
  return mono;
}


void saveMonoWav(const string& filename, const vector<unsigned char>& monoData, uint32_t sampleRate, uint16_t bitDepth) {
  if (bitDepth != 8 && bitDepth != 16) { throw runtime_error("Expected 8 or 16-bit PCM"); }

  // Mono is one channel
  uint16_t blockAlign = bitDepth / 8;
  if (monoData.size() % blockAlign != 0) { throw runtime_error("Incomplete mono sample"); }

  // Ensuring there's room for varying header sizes + padding
  if (monoData.size() > UINT32_MAX - 37u) { throw runtime_error("Audio data too large"); }

  uint32_t dataSize = static_cast<uint32_t>(monoData.size());
  uint32_t padding = dataSize % 2;
  uint64_t byteRate = uint64_t{sampleRate} * blockAlign;

  // Ensuring byte rate doesn't exceed writing capacity
  if (byteRate > UINT32_MAX) { throw runtime_error("Byte rate too large"); }

  // Creating file
  ofstream out(filename, ios::binary);
  if (!out) { throw runtime_error("Unable to create file"); }

  // Per StackOverflow, a typical WAV file follows this format:
  // RIFF chunk
  //    ChunkID     (4 bytes)
  //    ChunkSize   (4 bytes)
  //    Format      (4 bytes)
  // "fmt" subchunk
  //    Subchunk1ID   (4 bytes)
  //    Subchunk1Size (4 bytes)
  //    AudioFormat   (2 bytes)
  //    NumChannels   (2 bytes)
  //    SampleRate    (4 bytes)
  //    ByteRate      (4 bytes)
  //    BlockAlign    (2 bytes)
  //    BitsPerSample (2 bytes)
  // "data" subchunk
  //    Subchunk2ID   (4 bytes)
  //    Subchunk2ID   (4 bytes)
  //    data          (n bytes)


  // Writing RIFF chunk
  out.write("RIFF", 4);
  writeLE32(out, 36 + dataSize + padding);

  // Writing WAV chunk
  out.write("WAVE", 4);

  // "fmt" subsection
  out.write("fmt ", 4);
  writeLE32(out, 16);
  writeLE16(out, 1);
  writeLE16(out, 1);
  writeLE32(out, sampleRate);
  writeLE32(out, static_cast<uint32_t>(byteRate));
  writeLE16(out, blockAlign);
  writeLE16(out, bitDepth);

  // "data" subsection
  out.write("data", 4);
  writeLE32(out, dataSize);
  if (!monoData.empty()) {
    out.write(
        reinterpret_cast<const char*>(monoData.data()), 
        static_cast<streamsize>(monoData.size())
        );
  }

  if (padding) {
    // Padding isn't included in the data chunk's size
    out.put('\0');
  }

  out.close();
  if (!out) { throw runtime_error("Failed to save .WAV file"); }
}

}

namespace {

// *=== SAVING FILE HELPERS ===* //

// Writes a given 16-bit little-endian value to a given stream
void writeLE16(ostream& out, uint16_t value) {
  unsigned char bytes[2] = {
    static_cast<unsigned char>(value),
    static_cast<unsigned char>(value >> 8)
  };

  out.write(reinterpret_cast<const char*>(bytes), 2);
}

// Writes a given 32-bit little-endian value to a given stream
void writeLE32(ostream& out, uint32_t value) {
  unsigned char bytes[4] = {
    static_cast<unsigned char>(value),
    static_cast<unsigned char>(value >> 8),
    static_cast<unsigned char>(value >> 16),
    static_cast<unsigned char>(value >> 24)
  };

  out.write(reinterpret_cast<const char*>(bytes), 4);
}

}
