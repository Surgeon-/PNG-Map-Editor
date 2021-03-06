// PNG Map Editor.cpp

#include "pch.h"

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <stdexcept>

#include "Converter.hpp"
#include "ConsoleIO.hpp"

sf::String input_text(const sf::String & prompt = "") {
    
    std::cout << prompt.toAnsiString();

    std::string temp;

    std::cin >> temp;

    return sf::String{temp};

}

void output_text(const sf::String & text) {

    std::cout << text.toAnsiString();

}

int main(int argc, char *argv[]) {

    output_text("=== PNG Map Editor ===\n");

    sf::Image img{};
    
    if (!img.loadFromFile(input_text("Input path to Image file:\n"))) {
        
        // ERROR

        char buffer[256];
        strerror_s(buffer, (sizeof(buffer) / sizeof(buffer[0])), errno);
        output_text(buffer);
        output_text("\n");

        WaitAnyKey("Program finished, press any key to continue...");
        return 1;

    }

    Converter conv{};

    conv.setOutputPath(input_text("Input path to Output file:\n"));

    try {

        auto conf_path = input_text("Input path to Config file:\n");

        output_text("Reading config file...\n");
        conv.configureFromFile(conf_path);
        output_text("Done!\n");

        output_text("Starting conversion...\n");
        conv.convertImage(img);
        output_text("Done!\n");
    
    }
    catch (std::exception & ex) {
        
        // ERROR

        output_text("Exception caught: ");
        output_text(ex.what());
        output_text("\n");

        WaitAnyKey("Program finished, press any key to continue...");
        return 1;

    }

    WaitAnyKey("Program finished, press any key to continue...");
    return 0;

}
