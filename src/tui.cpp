#include "tui.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include<windows.h>
#include<mmsystem.h>

using namespace ftxui;


CompressionSettings runTUI(){
  auto screen = ScreenInteractive::Fullscreen();

  // Find available WAV files
  std::vector<std::string> wavFiles;

  if (std::filesystem::exists("samples")) {
    for (const auto& entry :
        std::filesystem::directory_iterator("samples")) {

      if (entry.is_regular_file() &&
          entry.path().extension() == ".wav") {
          
          std::string name = entry.path().filename().string();

          if(name == "bushw_war.wav" ||
             name == "no_ways_tired.wav" ||
             name == "sample-5mb.wav") {

        wavFiles.push_back(entry.path().string());
      }
    }
  }
  }
  // WAV File Selection
  std::sort(wavFiles.begin(), wavFiles.end());

  if (wavFiles.empty()) {
    wavFiles.push_back("No WAV files found");
  }

  int selectedWav = 0;
  auto wavMenu = Radiobox(&wavFiles, &selectedWav);

  // Wavelet Selection
  std::vector<std::string> wavelets = {
    "Haar", "Daubechies"
  };
  int selectedWavelet = 0;
  auto waveletMenu = Radiobox(&wavelets, &selectedWavelet);

  // Decomposition Level Selection
  std::vector<std::string> levels = {
    "1", "3", "5"
  };
  int selectedLevel = 0;
  auto levelMenu = Radiobox(&levels, &selectedLevel);

  // Threshold Selection
  std::vector<std::string> thresholds = {
    "0", "0.001", "0.005", "0.01"
  };
  int selectedThreshold = 0;
  auto thresholdMenu = Radiobox(&thresholds, &selectedThreshold);

  // Save Button
  bool saved = false;

  auto saveButton = Button("Save Changes",[&] {
      saved = true;
      screen.ExitLoopClosure()();
  });
  

  // Container
  auto container = Container:: Vertical({
      waveletMenu, levelMenu, thresholdMenu, saveButton
 });    

  // Interface
  auto mainInterface = Renderer(container, [&]{
    return vbox({
        text("Audio Compression") | bold | center,
        separator(),

        text("Select Wavelet:"),
        waveletMenu->Render(),
        
        separator(),

        text("Select Decomposition Level:"),
        levelMenu->Render(),

        separator(),

        text("Select Threshold:"),
        thresholdMenu->Render(),

        separator(),

        saveButton->Render() | center
    })  | border;        
  });

  screen.Loop(mainInterface);
  std::string selectedMethod =
    wavelets[selectedWavelet] == "Haar" ? "haar" : "d4";

  std::string matchingWav =
    "samples/" + selectedMethod +
    "_L" + levels[selectedLevel] +
    "/T" + std::to_string(std::stod(thresholds[selectedThreshold])) + ".wav";
 
  if (wavFiles.size() == 1 && wavFiles[0] == "No WAV files found") {
    wavFiles.clear();
}
  if (std::filesystem::exists(matchingWav)) {
    wavFiles.push_back(matchingWav);
}
  screen = ScreenInteractive::Fullscreen();
  wavMenu = Radiobox(&wavFiles, &selectedWav);

  if (saved) {
       auto continueButton = Button("Continue", [&] {
        screen.ExitLoopClosure()();
    });
    auto wavContainer = Container::Vertical({
        wavMenu, continueButton
    });

    auto wavInterface = Renderer(wavContainer, [&] {
        return vbox({
            text("Select WAV File") | bold | center,
            separator(),
            wavMenu->Render(),
            separator(),
            continueButton->Render() | center
        }) | border;
    });

    screen.Loop(wavInterface);
}
  CompressionSettings settings;

  if (saved) {
      settings.wavelet = wavelets[selectedWavelet];
      settings.level = std::stoul(levels[selectedLevel]);
      settings.threshold = std::stod(thresholds[selectedThreshold]);
      settings.wavPath = wavFiles[selectedWav];
  }

  return settings;
  }
