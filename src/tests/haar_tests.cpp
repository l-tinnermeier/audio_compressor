#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

// inverse equations are:
// left = (approx + detail) / sqrt(2)
// right = (approx - detail) / sqrt(2)
std::vector<double> haarDecode(
    std::vector<double> coefficients,
    unsigned int levels)
{
    std::size_t active = coefficients.size();

    // Find the size of the final approximation subband.
    for (unsigned int level = 0; level < levels; ++level) {
        if (active < 2 || active % 2 != 0) {
            throw std::invalid_argument("Invalid length for these levels");
        }
        active /= 2;
    }

    std::vector<double> temp(coefficients.size());
    const double scale = 1.0 / std::sqrt(2.0);

    for (unsigned int level = 0; level < levels; ++level) {
        const std::size_t half = active;
        active *= 2;

        for (std::size_t i = 0; i < half; ++i) {
            const double approximation = coefficients[i];
            const double detail = coefficients[half + i];

            temp[2 * i]     = (approximation + detail) * scale;
            temp[2 * i + 1] = (approximation - detail) * scale;
        }

        for (std::size_t i = 0; i < active; ++i) {
            coefficients[i] = temp[i];
        }
    }

    return coefficients;
}

std::vector<double> haarEncode(
    std::vector<double> input,
    unsigned int levels)
{
    std::size_t active = input.size();

    // Check that every requested level can split into pairs.
    for (unsigned int level = 0; level < levels; ++level) {
        if (active < 2 || active % 2 != 0) {
            throw std::invalid_argument("Invalid length for these levels");
        }
        active /= 2;
    }

    std::vector<double> temp(input.size());
    const double scale = 1.0 / std::sqrt(2.0);
    active = input.size();

    for (unsigned int level = 0; level < levels; ++level) {
        const std::size_t half = active / 2;

        for (std::size_t i = 0; i < half; ++i) {
            const double left = input[2 * i];
            const double right = input[2 * i + 1];

            temp[i] = (left + right) * scale;
            temp[half + i] = (left - right) * scale;
        }

        for (std::size_t i = 0; i < active; ++i) {
            input[i] = temp[i];
        }

        active = half;
    }

    return input;
}

void hardThreshold(std::vector<double>& coefficients, double threshold) {
    if (!std::isfinite(threshold) || threshold < 0.0) {
        throw std::invalid_argument(
            "Threshold must be finite and nonnegative");
    }

    for (double& c : coefficients) {
        if (std::abs(c) <= threshold) {
            c = 0.0;
        }
    }
}

std::vector<double> decodeMonoPCM(
    const std::vector<unsigned char>& bytes,
    unsigned int bitDepth)
{
    if (bitDepth != 8 && bitDepth != 16) {
        throw std::invalid_argument("Expected 8-bit or 16-bit PCM");
    }

    const std::size_t bytesPerSample = bitDepth / 8;

    if (bytes.size() % bytesPerSample != 0) {
        throw std::invalid_argument("Incomplete PCM sample");
    }

    std::vector<double> samples;
    samples.reserve(bytes.size() / bytesPerSample);

    for (std::size_t i = 0; i < bytes.size(); i += bytesPerSample) {
        if (bitDepth == 8) {
            // Unsigned PCM: 128 represents silence.
            samples.push_back((static_cast<int>(bytes[i]) - 128) / 128.0);
        } else {
            // Combine the two little-endian bytes.
            const unsigned int bits =
                static_cast<unsigned int>(bytes[i])
                | (static_cast<unsigned int>(bytes[i + 1]) << 8);

            // Interpret the result as a signed 16-bit value.
            const int value = bits >= 32768
                ? static_cast<int>(bits) - 65536
                : static_cast<int>(bits);

            samples.push_back(value / 32768.0);
        }
    }

    return samples;
}

void main() {
    const std::vector<unsigned char> pcm8{
        0, 128, 255
    };

    const std::vector<unsigned char> pcm16{
        0x00, 0x80, // -32768
        0x00, 0x00, //      0
        0xFF, 0x7F  //  32767
    };

    std::cout << "8-bit: ";
    for (double sample : decodeMonoPCM(pcm8, 8)) {
        std::cout << sample << ' ';
    }

    std::cout << "\n16-bit: ";
    for (double sample : decodeMonoPCM(pcm16, 16)) {
        std::cout << sample << ' ';
    }

    std::cout << '\n';
}
