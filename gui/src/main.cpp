#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>

int main (void) {
    ftxui::Element document = ftxui::hbox({
        ftxui::text("left")   | ftxui::border,
        ftxui::text("middle") | ftxui::border | ftxui::flex,
        ftxui::text("right")  | ftxui::border
    });

    auto screen = ftxui::Screen::Create(
        ftxui::Dimension::Full(), // width
        ftxui::Dimension::Fit(document)
    );

    ftxui::Render(screen, document);
    screen.Print();

    return EXIT_SUCCESS;
}

