#include "stack_vm.h"

int main() {
    StackVM vm;

    std::vector<u32> prog{
        3,
        4,
        0x40000001, // add
        5,
        0x40000002, // sub
        3,
        0x40000003, // mul
        2,
        0x40000004, // div
        0x40000000  // halt
    };

    vm.loadProgram(prog);
    vm.run();
}
