
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

void runTUI() {
    auto screen = ScreenInteractive::TerminalOutput();

    auto exitButton = Button("Exit", screen.ExitLoopClosure());

    auto interface = Renderer(exitButton, [&] {
        return vbox({
            text("Audio Compression") | bold | center,
            separator(),
            exitButton->Render()
        }) | border;
    });

    screen.Loop(interface);
}

