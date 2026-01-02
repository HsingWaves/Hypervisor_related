#include "stack_vm.h"

/* ---------- constructor ---------- */

StackVM::StackVM() {
    stack.reserve(1024);
}

/* ---------- instruction helpers ---------- */

u32 StackVM::getType(u32 instr) {
    return (instr >> 30) & 0b11;
}

i32 StackVM::getData(u32 instr) {
    return static_cast<i32>(instr & 0x3fffffff);
}

/* ---------- stack helpers ---------- */

void StackVM::push(i32 v) {
    stack.push_back(v);
}

i32 StackVM::pop() {
    if (stack.empty())
        throw std::runtime_error("stack underflow");
    i32 v = stack.back();
    stack.pop_back();
    return v;
}

/* ---------- program loading ---------- */

void StackVM::loadProgram(const std::vector<u32>& prog) {
    program = prog;
    pc = 0;
}

/* ---------- pipeline ---------- */

void StackVM::fetch() {
    if (pc >= program.size())
        throw std::runtime_error("pc out of range");
    instr = program[pc++];
}

void StackVM::decode() {
    type = getType(instr);
    data = getData(instr);

    // type 2 = negative immediate
    if (type == 2)
        data = -data;
}

void StackVM::execute() {
    switch (type) {
    case 0: // positive imm
    case 2: // negative imm
        push(data);
        std::cout << "push " << data << "\n";
        break;

    case 1: // primitive
        doPrimitive(data);
        break;

    default:
        throw std::runtime_error("invalid instruction type");
    }
}

/* ---------- primitives ---------- */

void StackVM::doPrimitive(i32 op) {
    switch (op) {
    case 0: // halt
        std::cout << "halt\n";
        running = false;
        break;

    case 1: { // add
        auto b = pop();
        auto a = pop();
        push(a + b);
        std::cout << "add " << a << " " << b << "\n";
        break;
    }

    case 2: { // sub
        auto b = pop();
        auto a = pop();
        push(a - b);
        std::cout << "sub " << a << " " << b << "\n";
        break;
    }

    case 3: { // mul
        auto b = pop();
        auto a = pop();
        push(a * b);
        std::cout << "mul " << a << " " << b << "\n";
        break;
    }

    case 4: { // div
        auto b = pop();
        auto a = pop();
        if (b == 0)
            throw std::runtime_error("division by zero");
        push(a / b);
        std::cout << "div " << a << " " << b << "\n";
        break;
    }

    default:
        throw std::runtime_error("unknown primitive");
    }
}

/* ---------- run loop ---------- */

void StackVM::run() {
    running = true;
    while (running) {
        fetch();
        decode();
        execute();

        if (!stack.empty())
            std::cout << "tos = " << stack.back() << "\n";
    }
}
