#include "pch.h"

#include <iostream>
#include <string>

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <conio.h>

void ConsoleSetup() {

    _setmode(_fileno(stdin), _O_U16TEXT);

}

void WaitAnyKey(const std::string & output) {

    if (!output.empty()) std::cout << output;

    _getch();

}

