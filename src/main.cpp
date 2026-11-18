#include "tui.hpp"
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
#include <iomanip>
#include<sstream>
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#else
#include <cstdlib>
#endif

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

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

void playAudio(const std::string& path) {
  #ifdef _WIN32
    PlaySoundA(
        path.c_str(),
        nullptr,
        SND_FILENAME | SND_ASYNC
    );
  #elif defined(__APPLE__)
    std::string command = "afplay \"" + path + "\" &";
    std::system(command.c_str());
  #endif
}

void stopAudio() {
#ifdef _WIN32
    PlaySoundA(nullptr, nullptr, 0);
#elif defined(__APPLE__)
    std::system("pkill -x afplay >/dev/null 2>&1");
#endif
}

void showPlayback(
    const std::string& originalPath,
    const std::string& thresholdedPath,
    const std::string& decompressedPath
) {
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();

    auto originalButton = Button("Play Original", [&] {
        playAudio(originalPath);
    });

    auto thresholdedButton = Button("Play Thresholded", [&] {
        playAudio(thresholdedPath);
    });

    auto decompressedButton = Button("Play Decompressed", [&] {
        playAudio(decompressedPath);
    });

    auto stopButton = Button("Stop Playback", [&] {
        stopAudio();
    });

    auto exitButton = Button("Exit", [&] {
        stopAudio();
        screen.ExitLoopClosure()();
    });

    auto layout = Container::Vertical({
        originalButton,
        thresholdedButton,
        decompressedButton,
        stopButton,
        exitButton
    });

    auto playbackInterface = Renderer(layout, [&] {
        return vbox({
            text("AUDIO PLAYBACK") | bold | center,
            separator(),
            text("Compare the three audio versions") | center,
            separator(),
            originalButton->Render() | center,
            thresholdedButton->Render() | center,
            decompressedButton->Render() | center,
            separator(),
            stopButton->Render() | center,
            exitButton->Render() | center
        }) | border | center;
    });

    screen.Loop(playbackInterface);
    stopAudio();
}
  
void showResults(
    const std::string& filename,
    const std::string& wavelet,
    unsigned int levels,
    double threshold,
    size_t padding,
    size_t coefficientCount,
    const std::string& extensionMethod,
    const std::string& subbandInfo,
    uint32_t sampleRate,
    uint16_t channels,
    uint16_t bitDepth,
    uint32_t byteRate,
    uint64_t fileSize,
    uint32_t dataSize,
    uint16_t headerSize,
    size_t sampleCount,
    double durationSeconds,
    size_t monoDataSize,
    double compressionRatio,
    double snr,
    double zeroPercent,
    double mse,
    double maxError,
    const std::string& binaryPath,
    const std::string& metadataPath,
    const std::string& decodedPath,
    const std::string& savedWavPath
) {
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();

    auto closeButton = Button("Close", [&] {
        screen.ExitLoopClosure()();
    });

    auto layout = Container::Vertical({
        closeButton
    });

    std::ostringstream mseFormatted;
    mseFormatted << std::scientific << std::setprecision(6) << mse;

    auto resultsInterface = Renderer(layout, [&] {
        return vbox({
            text("AUDIO COMPRESSION RESULTS") | bold | center,
            separator(),

            text("AUDIO FILE INFORMATION") | bold,
            separator(),

            text("Input File: " + filename),
            text("Sample Rate: " + std::to_string(sampleRate) + " Hz"),
            text("Original Channels: " + std::to_string(channels)),
            text("Bit Depth: " + std::to_string(bitDepth) + " bits"),
            text("Calculated Byte Rate: " + std::to_string(byteRate) + " bytes/s"),
            text("File Size: " + std::to_string(fileSize) + " bytes"),
            text("Data Size: " + std::to_string(dataSize) + " bytes"),
            text("Header Size: " + std::to_string(headerSize) + " bytes"),

            separator(),

            text("MONO AUDIO INFORMATION") | bold,
            separator(),

            text("Output Channels: 1"),
            text("Sample Count: " + std::to_string(sampleCount)),
            text("Duration: " + std::to_string(durationSeconds) + " seconds"),
            text("Audio Data Size: " + std::to_string(monoDataSize) + " bytes"),

            separator(),

            text("WAVELET CONFIGURATION") | bold,
            separator(),

            text("Wavelet: " + wavelet),
            text("Decomposition Level: " + std::to_string(levels)),
            text("Threshold: " + std::to_string(threshold)),
            

            separator(),

            text("WAVELET SUBBANDS") | bold,
            separator(),

            paragraph(subbandInfo),

            separator(),

            text("COMPRESSION PERFORMANCE") | bold,
            separator(),

            text("Compression Ratio: " + std::to_string(compressionRatio) + ":1"),
            text("Signal-to-Noise Ratio: " + std::to_string(snr) + " dB"),
            text("Zero Coefficients: " + std::to_string(zeroPercent) + "%"),
            text("Mean Squared Error: " + mseFormatted.str()),
            text("Maximum Error: " + std::to_string(maxError)),

            separator(),

            text("OUTPUT FILES") | bold,
            separator(),

            text("Compressed Binary:"),
            paragraph(binaryPath),

            text("Metadata:"),
            paragraph(metadataPath),

            text("Decoded WAV:"),
            paragraph(decodedPath),

            text("Saved WAV:"),
            paragraph(savedWavPath),

            separator(),

            text("Decompression Complete!") | bold | center,
            closeButton->Render() | center

        }) | border | frame;
    });

    screen.Loop(resultsInterface);
}
int main() {

  CompressionSettings settings = runTUI();

  string filename = settings.wavPath;

  if (filename.empty() || filename == "No WAV files found"){
    cerr << " No WAV file selected." << endl;
    return 1;
  }
 
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
    audio::saveMonoWav("samples/reference_mono.wav", monoBytes, sampleRate, bitDepth);

   
    string userResponse;

    if (channels == 2 &&
        std::filesystem::path(filename).filename() == "sample-5mb.wav" &&
        settings.saveMono) {

    if (settings.monoFilename.empty()) {
        throw runtime_error("Please enter a filename for the mono copy.");
    }

    audio::saveMonoWav(
        (std::filesystem::path("samples") /
          (settings.monoFilename + ".wav")).string(),
        monoBytes,
        sampleRate,
        bitDepth
    );
}
    const auto referenceSamples = audio::decodeMonoPCM(monoBytes, bitDepth);


    if (referenceSamples.empty()) { throw runtime_error("The recording contains no audio samples"); }

    const double durationSeconds = static_cast<double>(referenceSamples.size()) / sampleRate;
    
        ofstream results("results.csv");
    if (!results) { throw runtime_error("Could not create results.csv"); }
    results << setprecision(17);
    results
      << "reference,wavelet,levels,threshold,zero_percent,compression_ratio,mse,max_error,snr_db,"
      << "sample_rate,bit_depth,original_sample_count,padded_sample_count,"
      << "padding_samples,extension,coefficient_order,subband_sizes\n";

    // Haar experiment
    if(settings.wavelet == "Haar") {
    
    for (unsigned int levels : {settings.level}) {
      auto workingSamples = referenceSamples;

      const size_t multiple = size_t{1} << levels;
      size_t padding = (multiple - workingSamples.size() % multiple) % multiple;
      workingSamples.resize(workingSamples.size() + padding, 0.0);

      const auto coefficients = dwt::haarEncode(workingSamples, levels);

      string coefficientOrder = "A" + to_string(levels);
      size_t size = coefficients.size() / multiple;
      string subbandSizes = to_string(size);
      for (unsigned int level = levels; level > 0; level--) {
          coefficientOrder += ";D" + to_string(level);
          subbandSizes += ";" + to_string(size);
          size *= 2;
      }

      for (double threshold : {settings.threshold}) {
        auto thresholded = coefficients;
        dwt::hardThreshold(thresholded, threshold);

// Reconstruct audio after thresholding, before quantization
auto thresholdedAudio = dwt::haarDecode(thresholded, levels);
thresholdedAudio.resize(referenceSamples.size());

// Save thresholded audio
const std::filesystem::path thresholdedDirectory =
    std::filesystem::path("samples") /
    ("haar_L" + to_string(levels));

std::filesystem::create_directories(thresholdedDirectory);

const auto thresholdedPath =
    thresholdedDirectory /
    ("T" + to_string(threshold) + "_thresholded.wav");

Decoding24Bit::saveWav(
    thresholdedAudio,
    thresholdedPath.string(),
    sampleRate
);
      // Quantize wavelet coefficients to signed 16-bit integers
      auto quantized = Encoding24Bit::quantize(thresholded); 
      auto records = Encoding24Bit::encode(quantized);
      
      // Create a directory for the compressed files
      const std::filesystem::path compressedDirectory =
       std::filesystem::path("compressed") /
       ("haar_L" + to_string(levels));

      std::filesystem::create_directories(compressedDirectory);

      // Save the 24-bit records
      const auto binaryPath =
        compressedDirectory / ("T" + to_string(threshold) + ".bin");

      Encoding24Bit::saveRecords(records, binaryPath.string());

      // Save metadata needed for decompression
      const auto metadataPath =
        compressedDirectory / ("T" + to_string(threshold) + ".txt");

      Encoding24Bit::saveMetadata(
       quantized.size(),
       referenceSamples.size(),
       sampleRate,
       bitDepth,
       "haar",
       levels,
       metadataPath.string()
    );
      // Decompress the saved 24-bit records
      const auto decompressedPath =
       compressedDirectory / ("T" + to_string(threshold) + "_decoded.wav");

      Decoding24Bit::runDecompression(
       binaryPath.string(),
       metadataPath.string(),
       decompressedPath.string()
    );
        size_t zeros = 0;
        for (int16_t c : quantized) { 
          if (c == 0.0) zeros++; 
    }

      auto restored = Decoding24Bit::reconstructAudio(
       binaryPath.string(),
       metadataPath.string()
    );

        const double snr = Decoding24Bit::calculateSNR(referenceSamples, restored);

        const std::filesystem::path directory = std::filesystem::path("samples") / ("haar_L" + to_string(levels));
        std::filesystem::create_directories(directory);
        const auto outputPath = directory / ("T" + to_string(threshold) + ".wav");
        Decoding24Bit::saveWav(restored, outputPath.string(), sampleRate);
        
        double squaredErr = 0.0;
        double maxErr = 0.0;

        for (size_t i = 0; i < referenceSamples.size(); i++) { 
          const double difference = referenceSamples[i] - restored[i]; 
          squaredErr += difference * difference;
          maxErr = max(maxErr, abs(difference));
        }

        const double mse = squaredErr / referenceSamples.size();
        const double zeroPercent = 100.0 * static_cast<double>(zeros) / quantized.size();

        // Calculate compression ratio
        const double ratio = Encoding24Bit::compressionRatio(quantized, records);

// Prepare subband information
std::string subbandInfo = "";
size_t displayBandSize = coefficients.size() / multiple;
size_t displayOffset = 0;

subbandInfo += "A" + to_string(levels) +
    ": Offset " + to_string(displayOffset) +
    ", Size " + to_string(displayBandSize) + "\n";

displayOffset += displayBandSize;

for (unsigned int level = levels; level > 0; level--) {
    subbandInfo += "D" + to_string(level) +
        ": Offset " + to_string(displayOffset) +
        ", Size " + to_string(displayBandSize) + "\n";

    displayOffset += displayBandSize;
    displayBandSize *= 2;
}

// Display formatted results
showResults(
    filename,
    "Haar",
    levels,
    threshold,
    padding,
    coefficients.size(),
    "Zero padding at the end",
    subbandInfo,
    sampleRate,
    channels,
    bitDepth,
    audio::calculateByteRate(sampleRate, channels, bitDepth),
    static_cast<uint64_t>(fileSize),
    dataSize,
    headerSize,
    referenceSamples.size(),
    durationSeconds,
    monoBytes.size(),
    ratio,
    snr,
    zeroPercent,
    mse,
    maxErr,
    binaryPath.string(),
    metadataPath.string(),
    decompressedPath.string(),
    outputPath.string()
);
 showPlayback(
    "samples/reference_mono.wav",
    thresholdedPath.string(),
    decompressedPath.string()
);
// Save results to CSV
results
    << "samples/reference_mono.wav,haar,"
    << levels << ','
    << threshold << ','
    << zeroPercent << ','
    << ratio << ','
    << mse << ','
    << maxErr << ','
    << snr << ','
    << sampleRate << ','
    << bitDepth << ','
    << referenceSamples.size() << ','
    << coefficients.size() << ','
    << padding << ','
    << "zero_pad_end,"
    << coefficientOrder << ','
    << subbandSizes << '\n';      }
    }

 }

    // Daubechies experiment
    if (settings.wavelet == "Daubechies") {

 
    for (unsigned int levels : {settings.level}) {
      auto workingSamples = referenceSamples;

      const size_t multiple = size_t{1} << levels;
      size_t padding = (multiple - workingSamples.size() % multiple) % multiple;
      workingSamples.resize(workingSamples.size() + padding, 0.0);

      const auto coefficients = dwt::daubechies4EncodeLevels(workingSamples, levels);

      string coefficientOrder = "A" + to_string(levels);
      size_t size = coefficients.size() / multiple;
      string subbandSizes = to_string(size);
      for (unsigned int level = levels; level > 0; level--) {
          coefficientOrder += ";D" + to_string(level);
          subbandSizes += ";" + to_string(size);
          size *= 2;
      }

     for (double threshold : {settings.threshold}) {
    auto thresholded = coefficients;
    dwt::hardThreshold(thresholded, threshold);

    // Reconstruct audio after thresholding
    auto thresholdedAudio =
        dwt::daubechies4DecodeLevels(thresholded, levels);

    thresholdedAudio.resize(referenceSamples.size());

    // Save thresholded audio
    const std::filesystem::path thresholdedDirectory =
        std::filesystem::path("samples") /
        ("d4_L" + to_string(levels));

    std::filesystem::create_directories(thresholdedDirectory);

    const auto thresholdedPath =
        thresholdedDirectory /
        ("T" + to_string(threshold) + "_thresholded.wav");

    Decoding24Bit::saveWav(
        thresholdedAudio,
        thresholdedPath.string(),
        sampleRate
    );

    // Quantize wavelet coefficients to signed 16-bit integers
    auto quantized = Encoding24Bit::quantize(thresholded);      // Encode quantized coefficients into 24-bit records
      auto records = Encoding24Bit::encode(quantized);

      // Create a directory for the compressed files
      const std::filesystem::path compressedDirectory =
       std::filesystem::path("compressed") /
       ("d4_L" + to_string(levels));

      std::filesystem::create_directories(compressedDirectory);

      // Save the 24-bit records
      const auto binaryPath =
       compressedDirectory / ("T" + to_string(threshold) + ".bin");

      Encoding24Bit::saveRecords(records, binaryPath.string());
      // Save metadata needed for decompression
      const auto metadataPath =
        compressedDirectory / ("T" + to_string(threshold) + ".txt");

      Encoding24Bit::saveMetadata(
       quantized.size(),
       referenceSamples.size(),
       sampleRate,
       bitDepth,
       "d4_db2",
       levels,
       metadataPath.string()
    );

      // Decompress the saved 24-bit records
      const auto decompressedPath =
        compressedDirectory / ("T" + to_string(threshold) + "_decoded.wav");

      Decoding24Bit::runDecompression(
        binaryPath.string(),
        metadataPath.string(),
        decompressedPath.string()
    );
        size_t zeros = 0;
        for (int16_t c : quantized) { 
          if (c == 0.0) zeros++; 
      }

        auto restored = Decoding24Bit::reconstructAudio(
          binaryPath.string(),
          metadataPath.string()
    );
        const double snr = Decoding24Bit::calculateSNR(referenceSamples, restored);

        const std::filesystem::path directory = std::filesystem::path("samples") / ("d4_L" + to_string(levels));
        std::filesystem::create_directories(directory);
        const auto outputPath = directory / ("T" + to_string(threshold) + ".wav");
        Decoding24Bit::saveWav(restored, outputPath.string(), sampleRate);
        
        double squaredErr = 0.0;
        double maxErr = 0.0;

        for (size_t i = 0; i < referenceSamples.size(); i++) { 
          const double difference = referenceSamples[i] - restored[i]; 
          squaredErr += difference * difference;
          maxErr = max(maxErr, abs(difference));
        }

        const double mse = squaredErr / referenceSamples.size();
        const double zeroPercent = 100.0 * static_cast<double>(zeros) / quantized.size();
        const double ratio = Encoding24Bit::compressionRatio(quantized, records);

// Prepare subband information
std::string subbandInfo = "";
size_t displayBandSize = coefficients.size() / multiple;
size_t displayOffset = 0;

subbandInfo += "A" + to_string(levels) +
    ": Offset " + to_string(displayOffset) +
    ", Size " + to_string(displayBandSize) + "\n";

displayOffset += displayBandSize;

for (unsigned int level = levels; level > 0; level--) {
    subbandInfo += "D" + to_string(level) +
        ": Offset " + to_string(displayOffset) +
        ", Size " + to_string(displayBandSize) + "\n";

    displayOffset += displayBandSize;
    displayBandSize *= 2;
}

// Display formatted results
showResults(
    filename,
    "Daubechies",
    levels,
    threshold,
    padding,
    coefficients.size(),
    "Zero padding input with periodic wrapping at each D4 level",
    subbandInfo,
    sampleRate,
    channels,
    bitDepth,
    audio::calculateByteRate(sampleRate, channels, bitDepth),
    static_cast<uint64_t>(fileSize),
    dataSize,
    headerSize,
    referenceSamples.size(),
    durationSeconds,
    monoBytes.size(),
    ratio,
    snr,
    zeroPercent,
    mse,
    maxErr,
    binaryPath.string(),
    metadataPath.string(),
    decompressedPath.string(),
    outputPath.string()
);

showPlayback(
    "samples/reference_mono.wav",
    thresholdedPath.string(),
    decompressedPath.string()
);
        results
          << "samples/reference_mono.wav,d4_db2,"
          << levels << ','
          << threshold << ','
          << zeroPercent << ','
          << ratio << ','
          << mse << ','
          << maxErr << ','
          << snr << ','
          << sampleRate << ','
          << bitDepth << ','
          << referenceSamples.size() << ','
          << coefficients.size() << ','
          << padding << ','
          << "zero_pad_end_then_periodic_each_level,"
          << coefficientOrder << ','
          << subbandSizes << '\n';
      }
    }

    }
    results.close();
    if (!results) { throw runtime_error("Failed to finish writing results.csv"); }

  } catch(const exception &e) {
    cerr << e.what() << endl;
    return 1;
  }

  return 0;
}
