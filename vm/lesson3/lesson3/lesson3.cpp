#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using i32 = std::int32_t;
using u32 = std::uint32_t;

enum class Prim : u32 {
    Halt = 0,
    Add = 1,
    Sub = 2,
    Mul = 3,
    Div = 4,
};


//The program area and stack are separated(avoiding cramming them together into the same vector).
//
//The instruction format remains 2 - bit type + 30 - bit data.
//
//Negative number push is supported(when type = 2, the 30 - bit data is interpreted as a signed 30 - bit and sign - extended).
//
//Boundary checks and division by zero checks have been added.

struct Instr {
    // type: 0 => push non-negative
    // type: 1 => primitive
    // type: 2 => push negative (signed 30-bit)
    // type: 3 => unused
    static constexpr u32 TYPE_MASK = 0xC0000000u;
    static constexpr u32 DATA_MASK = 0x3FFFFFFFu;

    static u32 prim(Prim p) {
        return (1u << 30) | (static_cast<u32>(p) & DATA_MASK);
    }

    // Encode an int32 into your instruction format.
    // Non-negative -> type 0, store magnitude in 30-bit.
    // Negative     -> type 2, store 30-bit two's-complement in data field.
    static u32 push(i32 x) {
        if (x >= 0) {
            u32 ux = static_cast<u32>(x);
            if (ux > DATA_MASK) throw std::out_of_range("push: value too large for 30-bit immediate");
            return (0u << 30) | (ux & DATA_MASK);
        }
        else {
            // store signed 30-bit two's complement in data
            // range for signed 30-bit: [-2^29, 2^29 - 1]
            constexpr i32 MIN30 = -(1 << 29);
            constexpr i32 MAX30 = (1 << 29) - 1;
            if (x < MIN30 || x > MAX30) throw std::out_of_range("push: negative value out of signed 30-bit range");

            // Convert to 30-bit two's complement
            // Example: -1 -> 0x3FFFFFFF (30 ones)
            u32 data = static_cast<u32>(x) & DATA_MASK;
            return (2u << 30) | data;
        }
    }

    static u32 type(u32 instruction) {
        return (instruction & TYPE_MASK) >> 30;
    }

    static u32 data(u32 instruction) {
        return (instruction & DATA_MASK);
    }

    // Decode push-immediate into i32.
    static i32 decode_push(u32 instruction) {
        u32 t = type(instruction);
        u32 d = data(instruction);

        if (t == 0u) {
            return static_cast<i32>(d);
        }
        if (t == 2u) {
            // d is 30-bit signed two's complement -> sign-extend to 32-bit
            // If bit 29 is set, it's negative.
            constexpr u32 SIGN_BIT_30 = 1u << 29;
            if (d & SIGN_BIT_30) {
                // fill upper bits with 1s
                u32 extended = d | ~DATA_MASK;
                return static_cast<i32>(extended);
            }
            return static_cast<i32>(d);
        }

        throw std::runtime_error("decode_push called on non-push instruction");
    }
};

class StackVM {
public:
    explicit StackVM(std::size_t mem_words = 1'000'000, std::size_t program_base = 100)
        : mem_(mem_words, 0), program_base_(program_base) {
        if (program_base_ >= mem_.size()) throw std::out_of_range("program_base out of memory range");
    }

    void loadProgram(const std::vector<u32>& prog) {
        if (program_base_ + prog.size() > mem_.size()) throw std::out_of_range("program too large for memory");
        for (std::size_t i = 0; i < prog.size(); ++i) {
            mem_[program_base_ + i] = prog[i];
        }
        pc_ = program_base_;
        sp_ = 0;
        running_ = true;
    }

    void run(bool trace = true) {
        while (running_) {
            u32 instr = fetch();
            if (trace) {
                std::cout << "[pc=" << pc_ - 1 << "] instr=0x" << std::hex << instr << std::dec << "\n";
            }
            execute(instr, trace);
            if (trace && sp_ > 0) {
                std::cout << "  tos: " << stackTop() << "\n";
            }
        }
    }

private:
    // memory[0] unused for stack; stack uses mem_[1..sp_]
    std::vector<u32> mem_;
    std::size_t program_base_ = 100;

    std::size_t pc_ = 100; // points to next instruction to fetch
    std::size_t sp_ = 0;   // number of items on stack
    bool running_ = true;

    u32 fetch() {
        if (pc_ >= mem_.size()) throw std::out_of_range("pc out of memory range");
        return mem_[pc_++]; // fetch then advance
    }

    i32 pop() {
        if (sp_ == 0) throw std::runtime_error("stack underflow");
        i32 v = static_cast<i32>(mem_[sp_]); // stack values stored in low 32 bits
        --sp_;
        return v;
    }

    void push(i32 v) {
        if (sp_ + 1 >= program_base_) {
            throw std::runtime_error("stack overflow into program area");
        }
        ++sp_;
        mem_[sp_] = static_cast<u32>(v);
    }

    i32 stackTop() const {
        if (sp_ == 0) throw std::runtime_error("stack empty");
        return static_cast<i32>(mem_[sp_]);
    }

    void execute(u32 instr, bool trace) {
        u32 t = Instr::type(instr);
        u32 d = Instr::data(instr);

        if (t == 0u || t == 2u) {
            i32 imm = Instr::decode_push(instr);
            if (trace) std::cout << "  push " << imm << "\n";
            push(imm);
            return;
        }

        if (t != 1u) {
            throw std::runtime_error("undefined instruction type=3");
        }

        switch (static_cast<Prim>(d)) {
        case Prim::Halt: {
            if (trace) std::cout << "  halt\n";
            running_ = false;
            break;
        }
        case Prim::Add: {
            i32 b = pop();
            i32 a = pop();
            if (trace) std::cout << "  add " << a << " " << b << "\n";
            push(a + b);
            break;
        }
        case Prim::Sub: {
            i32 b = pop();
            i32 a = pop();
            if (trace) std::cout << "  sub " << a << " " << b << "\n";
            push(a - b);
            break;
        }
        case Prim::Mul: {
            i32 b = pop();
            i32 a = pop();
            if (trace) std::cout << "  mul " << a << " " << b << "\n";
            push(a * b);
            break;
        }
        case Prim::Div: {
            i32 b = pop();
            i32 a = pop();
            if (b == 0) throw std::runtime_error("division by zero");
            if (trace) std::cout << "  div " << a << " " << b << "\n";
            push(a / b);
            break;
        }
        default:
            throw std::runtime_error("unknown primitive opcode");
        }
    }
};

int main() {
    try {
        StackVM vm;

        // Same math as your example:
        // push 3; push 4; add; push 5; sub; push 3; mul; push 2; div; halt
        std::vector<u32> prog{
            Instr::push(3),
            Instr::push(4),
            Instr::prim(Prim::Add),
            Instr::push(5),
            Instr::prim(Prim::Sub),
            Instr::push(3),
            Instr::prim(Prim::Mul),
            Instr::push(2),
            Instr::prim(Prim::Div),
            Instr::prim(Prim::Halt),
        };

        vm.loadProgram(prog);
        vm.run(true);
    }
    catch (const std::exception& e) {
        std::cerr << "VM error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
