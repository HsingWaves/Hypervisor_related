#ifndef STACK_VM_H
#define STACK_VM_H

#include <cstdint>
#include <vector>
#include <iostream>
#include <stdexcept>

using i32 = std::int32_t;
using u32 = std::uint32_t;

class StackVM {
public:
    StackVM();

    void loadProgram(const std::vector<u32>& prog);
    void run();

private:
    // VM state
    std::size_t pc = 0;
    bool running = true;

    // memory
    std::vector<u32> program;
    std::vector<i32> stack;

    // decoded instruction
    u32 instr = 0;
    u32 type = 0;
    i32 data = 0;

private:
    // pipeline
    void fetch();
    void decode();
    void execute();

    // helpers
    static u32 getType(u32 instr);
    static i32 getData(u32 instr);
    void doPrimitive(i32 opcode);

    void push(i32 v);
    i32  pop();
};

#endif
