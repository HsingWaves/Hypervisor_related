#include "stack_vm.h"
#include <vector>

int main() {
    StackVM vm;

    // your original program:
    // 3, 4, add, halt
    std::vector<StackVM::i32> prog{
        3,
        4,
        0x40000001, // prim add
        0x40000000  // prim halt
    };

    vm.loadProgram(prog);
    vm.run(true);
    return 0;
}
