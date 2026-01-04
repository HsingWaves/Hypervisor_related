#include <cstdint>
#include <iostream>
#include <vector>
#include <stdexcept>

using i32 = int32_t;
using u32 = uint32_t;

class StackVM {
public:
    StackVM() {
        memory.resize(1024);   // program memory
        stack.resize(1024);    // runtime stack
    }

    void loadProgram(const std::vector<u32>& prog) {
        pc = 0;
        for (size_t i = 0; i < prog.size(); ++i) {
            memory[i] = prog[i];
        }
    }

    void run() {
        running = true;
        while (running) {
            fetch();
            decode();
            execute();
        }
    }

private:
    // ===== state =====
    size_t pc = 0;
    size_t sp = 0;
    bool running = true;

    std::vector<u32> memory;  // instructions
    std::vector<i32> stack;   // data stack

    // decoded instruction
    u32 type = 0;
    u32 data = 0;

    // ===== instruction helpers =====
    static u32 getType(u32 inst) {
        return inst >> 30;
    }

    static u32 getData(u32 inst) {
        return inst & 0x3FFFFFFF;
    }

    void fetch() {
        curr = memory[pc++];
    }

    void decode() {
        type = getType(curr);
        data = getData(curr);
    }

    void execute() {
        if (type == 0) {               // positive immediate
            push(static_cast<i32>(data));
        }
        else if (type == 2) {        // negative immediate
            push(-static_cast<i32>(data));
        }
        else if (type == 1) {        // primitive
            execPrimitive(data);
        }
        else {
            throw std::runtime_error("invalid instruction type");
        }
    }

    // ===== primitives =====
    void execPrimitive(u32 op) {
        switch (op) {
        case 0: // halt
            running = false;
            break;
        case 1: // add
            binop([](i32 a, i32 b) { return a + b; });
            break;
        case 2: // sub
            binop([](i32 a, i32 b) { return a - b; });
            break;
        case 3: // mul
            binop([](i32 a, i32 b) { return a * b; });
            break;
        case 4: // div
            binop([](i32 a, i32 b) {
                if (b == 0) throw std::runtime_error("divide by zero");
                return a / b;
                });
            break;
        default:
            throw std::runtime_error("unknown primitive");
        }
    }

    // ===== stack helpers =====
    void push(i32 v) {
        stack[sp++] = v;
    }

    i32 pop() {
        if (sp == 0) throw std::runtime_error("stack underflow");
        return stack[--sp];
    }

    template <typename F>
    void binop(F f) {
        i32 b = pop();
        i32 a = pop();
        push(f(a, b));
    }

    u32 curr = 0;
};
int main() {
	StackVM vm;

	// Sample program: computes (3 + 4) * 5
	std::vector<u32> program = {
		(0 << 30) | 3,      // push 3
		(0 << 30) | 4,      // push 4
		(1 << 30) | 1,      // add
		(0 << 30) | 5,      // push 5
		(1 << 30) | 3,      // mul
		(1 << 30) | 0       // halt
	};

	vm.loadProgram(program);
	vm.run();

	return 0;
}