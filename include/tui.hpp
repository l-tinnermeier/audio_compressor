#pragma once
#include <string>

struct CompressionSettings {
    std::string wavelet = "Haar";
    unsigned int level = 1;
    double threshold = 0.0;
    std:: string wavPath;
    bool saveMono = false;
    std::string monoFilename;
};

CompressionSettings runTUI();
