// stack-vm.h
#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>

using i32 = int32_t;

class StackVM {
    i32 pc_ = 100;      // program counter
    i32 sp_ = -1;       // stack pointer: -1 means empty stack
    std::vector<i32> memory_;
    i32 typ_ = 0;
    i32 dat_ = 0;
    bool running_ = true;

    static i32 getType(i32 instruction) {
        return (instruction >> 30) & 0x3;
    }
    static i32 getData(i32 instruction) {
        return instruction & 0x3fffffff;
    }

    void fetch() { ++pc_; }
    void decode() {
        typ_ = getType(memory_.at(pc_));
        dat_ = getData(memory_.at(pc_));
    }

    void push(i32 v) {
        if (sp_ + 1 >= static_cast<i32>(memory_.size())) {
            throw std::runtime_error("stack overflow");
        }
        memory_[++sp_] = v;
    }

    i32 pop() {
        if (sp_ < 0) throw std::runtime_error("stack underflow");
        return memory_[sp_--];
    }

    void doPrimitive() {
        switch (dat_) {
        case 0: // halt
            running_ = false;
            return;

        case 1: { // add
            i32 b = pop();
            i32 a = pop();
            push(a + b);
            return;
        }
        case 2: { // sub
            i32 b = pop();
            i32 a = pop();
            push(a - b);
            return;
        }
        case 3: { // mul
            i32 b = pop();
            i32 a = pop();
            push(a * b);
            return;
        }
        case 4: { // div
            i32 b = pop();
            i32 a = pop();
            if (b == 0) throw std::runtime_error("division by zero");
            push(a / b);
            return;
        }
        default:
            throw std::runtime_error("unknown primitive opcode: " + std::to_string(dat_));
        }
    }

    void execute() {
        if (typ_ == 0) {              // positive integer
            push(dat_);
        }
        else if (typ_ == 2) {       // negative integer (your encoding: -data)
            push(-dat_);
        }
        else if (typ_ == 1) {       // primitive instruction
            doPrimitive();
        }
        else {
            throw std::runtime_error("undefined instruction header: " + std::to_string(typ_));
        }
    }

public:
    StackVM() {
        memory_.resize(1'000'000); // IMPORTANT: resize, not reserve
    }

    void loadProgram(const std::vector<i32>& prog) {
        if (pc_ < 0) throw std::runtime_error("invalid pc");
        if (static_cast<size_t>(pc_) + prog.size() > memory_.size()) {
            throw std::runtime_error("program too large for memory");
        }
        for (size_t i = 0; i < prog.size(); ++i) {
            memory_[static_cast<size_t>(pc_) + i] = prog[i];
        }
    }

    void run(bool trace = true) {
        // set pc_ to just before first instruction, so fetch() lands on first
        pc_ -= 1;

        while (running_) {
            fetch();
            decode();
            execute();

            if (trace && sp_ >= 0) {
                std::cout << "pc=" << pc_ << " sp=" << sp_ << " tos=" << memory_[sp_] << "\n";
            }
        }
    }
};
