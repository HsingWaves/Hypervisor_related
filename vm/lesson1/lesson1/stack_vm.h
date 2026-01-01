#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <iostream>
#include <stdexcept>
#include <limits>

class StackVM {
public:
    using i32 = std::int32_t;
    using u32 = std::uint32_t;

    // 2-bit header
    enum class Type : u32 {
        PosImm = 0,   // 00: +imm (0..2^30-1)
        Prim = 1,   // 01: primitive opcode
        NegImm = 2,   // 10: -imm (-(0..2^30-1))
        Undef = 3    // 11: unused
    };

    // primitive opcodes stored in low-30 bits when Type::Prim
    enum class Prim : u32 {
        Halt = 0,
        Add = 1,
        Sub = 2,
        Mul = 3,
        Div = 4,
        Print = 5,
    };

    explicit StackVM(std::size_t stack_capacity = 1024);

    // Load "bytecode" program (vector of encoded 32-bit instructions)
    void loadProgram(std::span<const i32> prog);

    // Run until halt or error
    void run(bool trace = true);

private:
    // program
    std::vector<i32> program_;
    std::size_t pc_ = 0;              // index into program_

    // stack
    std::vector<i32> stack_;          // stack_[0..sp_-1] are valid
    std::size_t sp_ = 0;              // stack pointer = size

    bool running_ = false;

private:
    static constexpr u32 TYPE_MASK = 0xC000'0000u; // top 2 bits
    static constexpr u32 DATA_MASK = 0x3FFF'FFFFu; // low 30 bits

    static Type getType(i32 ins);
    static u32  getData(i32 ins);

    // stack helpers
    void push(i32 v);
    i32  pop();
    i32  peek(std::size_t from_top = 0) const;

    // execute
    void step(bool trace);
    void execPrimitive(Prim op, bool trace);
};
