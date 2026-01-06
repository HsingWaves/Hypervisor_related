// main.cpp
#include <fstream>
#include <iostream>
#include <vector>
#include "stack-vm.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <filename>\n";
        return 0;
    }

    std::ifstream r(argv[1], std::ios::binary);
    if (!r) {
        std::cerr << "Failed to open file: " << argv[1] << "\n";
        return 1;
    }

    std::vector<i32> prog;
    i32 word = 0;

    while (r.read(reinterpret_cast<char*>(&word), sizeof(word))) {
        prog.push_back(word);
    }

    try {
        StackVM vm;
        vm.loadProgram(prog);
        vm.run(true);
    }
    catch (const std::exception& e) {
        std::cerr << "VM error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
