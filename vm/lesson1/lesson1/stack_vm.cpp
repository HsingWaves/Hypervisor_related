#include "stack_vm.h"

StackVM::StackVM(std::size_t stack_capacity) {
    stack_.reserve(stack_capacity);
}

void StackVM::loadProgram(std::span<const i32> prog) {
    program_.assign(prog.begin(), prog.end());
    pc_ = 0;
}

StackVM::Type StackVM::getType(i32 ins) {
    // cast to u32 to avoid implementation-defined shift of signed
    u32 u = static_cast<u32>(ins);
    u32 t = (u & TYPE_MASK) >> 30;
    return static_cast<Type>(t);
}

StackVM::u32 StackVM::getData(i32 ins) {
    u32 u = static_cast<u32>(ins);
    return (u & DATA_MASK);
}

void StackVM::push(i32 v) {
    // optional overflow check if you want hard capacity:
    // if (stack_.size() == stack_.capacity()) throw std::runtime_error("stack overflow");
    stack_.push_back(v);
    sp_ = stack_.size();
}

StackVM::i32 StackVM::pop() {
    if (stack_.empty()) throw std::runtime_error("stack underflow");
    i32 v = stack_.back();
    stack_.pop_back();
    sp_ = stack_.size();
    return v;
}

StackVM::i32 StackVM::peek(std::size_t from_top) const {
    if (stack_.size() <= from_top) throw std::runtime_error("stack underflow (peek)");
    return stack_[stack_.size() - 1 - from_top];
}

void StackVM::run(bool trace) {
    if (program_.empty()) return;

    running_ = true;
    while (running_) {
        if (pc_ >= program_.size()) {
            throw std::runtime_error("pc out of program range (missing halt?)");
        }
        step(trace);
    }
}

void StackVM::step(bool trace) {
    const i32 ins = program_[pc_++];

    const auto typ = getType(ins);
    const auto dat = getData(ins);

    switch (typ) {
    case Type::PosImm: {
        // +dat
        push(static_cast<i32>(dat));
        if (trace) {
            std::cout << "[imm +] push " << static_cast<i32>(dat)
                << " | tos=" << peek() << "\n";
        }
        break;
    }
    case Type::NegImm: {
        // -dat
        // Note: dat is 0..(2^30-1), so -dat fits i32
        push(-static_cast<i32>(dat));
        if (trace) {
            std::cout << "[imm -] push " << -static_cast<i32>(dat)
                << " | tos=" << peek() << "\n";
        }
        break;
    }
    case Type::Prim: {
        auto op = static_cast<Prim>(dat);
        execPrimitive(op, trace);
        if (trace && !stack_.empty()) {
            std::cout << "        tos=" << peek() << "\n";
        }
        else if (trace && stack_.empty()) {
            std::cout << "        tos=<empty>\n";
        }
        break;
    }
    case Type::Undef:
    default:
        throw std::runtime_error("undefined instruction type (11)");
    }
}

void StackVM::execPrimitive(Prim op, bool trace) {
    switch (op) {
    case Prim::Halt:
        if (trace) std::cout << "[prim] halt\n";
        running_ = false;
        return;

    case Prim::Add: {
        i32 b = pop();
        i32 a = pop();
        i32 r = a + b;
        if (trace) std::cout << "[prim] add " << a << " " << b << " => " << r << "\n";
        push(r);
        return;
    }

    case Prim::Sub: {
        i32 b = pop();
        i32 a = pop();
        i32 r = a - b;
        if (trace) std::cout << "[prim] sub " << a << " " << b << " => " << r << "\n";
        push(r);
        return;
    }

    case Prim::Mul: {
        i32 b = pop();
        i32 a = pop();
        i32 r = a * b;
        if (trace) std::cout << "[prim] mul " << a << " " << b << " => " << r << "\n";
        push(r);
        return;
    }

    case Prim::Div: {
        i32 b = pop();
        i32 a = pop();
        if (b == 0) throw std::runtime_error("division by zero");
        // optional: check INT_MIN / -1 overflow
        if (a == std::numeric_limits<i32>::min() && b == -1) {
            throw std::runtime_error("division overflow (INT_MIN / -1)");
        }
        i32 r = a / b;
        if (trace) std::cout << "[prim] div " << a << " " << b << " => " << r << "\n";
        push(r);
        return;
    }

    case Prim::Print: {
        i32 v = peek();
        std::cout << "[prim] print: " << v << "\n";
        return;
    }

    default:
        throw std::runtime_error("unknown primitive opcode");
    }
}
