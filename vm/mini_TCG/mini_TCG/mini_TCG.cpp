#include <cstdint>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <limits>
#include <string>

#include <chrono>
#include <numeric>
#include <iomanip>

class MiniTCGVM {
public:
    using i32 = std::int32_t;
    using u32 = std::uint32_t;

    // 2-bit type (same idea as your encoding)
    enum class Type : u32 { PosImm = 0, Prim = 1, NegImm = 2, Undef = 3 };
    enum class Prim : u32 { Halt = 0, Add = 1, Print = 5 };

    struct State {
        std::size_t pc = 0;
        bool running = false;
        std::vector<i32> stack;
    };

    // Translation Block: compiled host "code" for a guest pc
    struct TB {
        std::size_t guest_pc = 0;
        std::size_t next_pc = 0;           // pc after executing this TB
        u32 compiled_version = 0;          // invalidation check
        std::function<void(State&)> exec;  // "host code"
        std::string debug;                 // optional: what got compiled
    };

public:
    explicit MiniTCGVM(std::size_t max_tb_insns = 8)
        : max_tb_insns_(max_tb_insns) {}

    void loadProgram(const std::vector<i32>& prog) {
        program_ = prog;
        // program changed => invalidate all TBs (like code page write)
        program_version_++;
        tb_cache_.clear();
    }

    // simulate "self-modifying code": patch one instruction
    void patch(std::size_t index, i32 new_insn) {
        if (index >= program_.size()) throw std::runtime_error("patch out of range");
        program_[index] = new_insn;
        program_version_++;
        // in real QEMU you might invalidate only TBs on the affected page/range
        tb_cache_.clear();
    }

    void run(bool trace = true) {
        State s;
        s.running = true;
        s.pc = 0;
        s.stack.clear();

        while (s.running) {
            if (s.pc >= program_.size()) throw std::runtime_error("pc out of range (missing halt?)");

            TB& tb = getOrTranslateTB(s.pc, trace);

            if (trace) {
                std::cout << ">> exec TB @pc=" << tb.guest_pc
                    << " (next_pc=" << tb.next_pc
                    << ", ver=" << tb.compiled_version << ")\n";
            }

            tb.exec(s);              // run host code
            s.pc = tb.next_pc;       // emulate "pc update" at TB exit

            if (trace) {
                if (!s.stack.empty()) std::cout << "   tos=" << s.stack.back() << "\n";
                else std::cout << "   tos=<empty>\n";
            }
        }
    }

    // helpers to build encoded instructions (like assembler)
    static i32 enc_pos_imm(i32 x) {
        if (x < 0) throw std::runtime_error("use enc_neg_imm for negative");
        u32 ux = static_cast<u32>(x);
        if (ux > DATA_MASK) throw std::runtime_error("imm too large");
        return static_cast<i32>((static_cast<u32>(Type::PosImm) << 30) | (ux & DATA_MASK));
    }
    static i32 enc_neg_imm(i32 x) { // x should be negative
        if (x > 0) throw std::runtime_error("use enc_pos_imm for positive");
        u32 mag = static_cast<u32>(-x);
        if (mag > DATA_MASK) throw std::runtime_error("imm too large");
        return static_cast<i32>((static_cast<u32>(Type::NegImm) << 30) | (mag & DATA_MASK));
    }
    static i32 enc_prim(Prim p) {
        return static_cast<i32>((static_cast<u32>(Type::Prim) << 30) | (static_cast<u32>(p) & DATA_MASK));
    }

private:
    static constexpr u32 TYPE_MASK = 0xC0000000u;
    static constexpr u32 DATA_MASK = 0x3FFFFFFFu;

    static Type getType(i32 ins) {
        u32 u = static_cast<u32>(ins);
        return static_cast<Type>((u & TYPE_MASK) >> 30);
    }
    static u32 getData(i32 ins) {
        u32 u = static_cast<u32>(ins);
        return (u & DATA_MASK);
    }

    static void push(State& s, i32 v) { s.stack.push_back(v); }
    static i32 pop(State& s) {
        if (s.stack.empty()) throw std::runtime_error("stack underflow");
        i32 v = s.stack.back();
        s.stack.pop_back();
        return v;
    }

    TB& getOrTranslateTB(std::size_t pc, bool trace) {
        auto it = tb_cache_.find(pc);
        if (it != tb_cache_.end() && it->second.compiled_version == program_version_) {
            if (trace) std::cout << "[TB HIT]  pc=" << pc << "\n";
            return it->second;
        }
        if (trace) std::cout << "[TB MISS] pc=" << pc << " -> translating...\n";
        TB tb = translateTB(pc);
        auto [ins_it, _] = tb_cache_.emplace(pc, std::move(tb));
        return ins_it->second;
    }

    TB translateTB(std::size_t start_pc) {
        TB tb;
        tb.guest_pc = start_pc;
        tb.compiled_version = program_version_;

        // "host ops": pre-decoded micro-ops for the block
        // Each op is a lambda that mutates VM state (like TCG IR lowered to host)
        std::vector<std::function<void(State&)>> ops;

        std::size_t pc = start_pc;
        std::size_t insn_count = 0;
        bool ended = false;

        tb.debug.clear();

        while (!ended && pc < program_.size() && insn_count < max_tb_insns_) {
            i32 ins = program_[pc];
            Type typ = getType(ins);
            u32 dat = getData(ins);

            // This is the only "switch-heavy" part: translation.
            switch (typ) {
            case Type::PosImm: {
                i32 imm = static_cast<i32>(dat);
                ops.emplace_back([imm](State& s) { push(s, imm); });
                tb.debug += "PUSH +" + std::to_string(imm) + "\n";
                pc++; insn_count++;
                break;
            }
            case Type::NegImm: {
                i32 imm = -static_cast<i32>(dat);
                ops.emplace_back([imm](State& s) { push(s, imm); });
                tb.debug += "PUSH " + std::to_string(imm) + "\n";
                pc++; insn_count++;
                break;
            }
            case Type::Prim: {
                auto op = static_cast<Prim>(dat);
                if (op == Prim::Halt) {
                    ops.emplace_back([](State& s) { s.running = false; });
                    tb.debug += "HALT\n";
                    pc++; insn_count++;
                    ended = true; // stop TB at halt
                }
                else if (op == Prim::Add) {
                    ops.emplace_back([](State& s) {
                        i32 b = pop(s);
                        i32 a = pop(s);
                        push(s, a + b);
                        });
                    tb.debug += "ADD\n";
                    pc++; insn_count++;
                }
                else if (op == Prim::Print) {
                    ops.emplace_back([](State& s) {
                        if (s.stack.empty()) std::cout << "[print] <empty>\n";
                        else std::cout << "[print] " << s.stack.back() << "\n";
                        });
                    tb.debug += "PRINT\n";
                    pc++; insn_count++;
                }
                else {
                    throw std::runtime_error("unknown primitive opcode");
                }
                break;
            }
            case Type::Undef:
            default:
                throw std::runtime_error("undefined instruction type");
            }

            // In real TCG, TB also often ends at control-flow boundaries.
            // Here we only end on HALT or max_tb_insns.
        }

        tb.next_pc = pc;

        // "compile": fuse ops into one callable (host code)
        tb.exec = [ops = std::move(ops)](State& s) {
            for (auto& f : ops) f(s);
            };

        return tb;
    }

private:
    std::vector<i32> program_;
    u32 program_version_ = 1;

    std::unordered_map<std::size_t, TB> tb_cache_;
    std::size_t max_tb_insns_;
};

template <class F>
static long long time_us(F&& f, int rounds = 1) {
    using namespace std::chrono;
    auto t0 = steady_clock::now();
    for (int i = 0; i < rounds; ++i) f();
    auto t1 = steady_clock::now();
    return duration_cast<microseconds>(t1 - t0).count();
}
// ---- demo ----
int main() {
    MiniTCGVM vm(/*max_tb_insns=*/8);

   
    std::vector<MiniTCGVM::i32> prog;
    const int N = 20000; //
    for (int i = 0; i < N; ++i) {
        prog.push_back(MiniTCGVM::enc_pos_imm(i));
        prog.push_back(MiniTCGVM::enc_pos_imm(i + 1));
        prog.push_back(MiniTCGVM::enc_prim(MiniTCGVM::Prim::Add));
    }
    prog.push_back(MiniTCGVM::enc_prim(MiniTCGVM::Prim::Print));
    prog.push_back(MiniTCGVM::enc_prim(MiniTCGVM::Prim::Halt));

    vm.loadProgram(prog);
   
    auto cold = [&]() {
        vm.loadProgram(prog);
        vm.run(false);
        };

    auto hot = [&]() {
        vm.run(false);
        };

    vm.loadProgram(prog);
    vm.run(false);

    const int cold_rounds = 5;  //
    const int hot_rounds = 30;  

    long long cold_us = time_us(cold, cold_rounds);
    vm.loadProgram(prog);
    vm.run(false); // cache
    long long hot_us = time_us(hot, hot_rounds);

    double cold_avg = double(cold_us) / cold_rounds;
    double hot_avg = double(hot_us) / hot_rounds;
    double speedup = cold_avg / hot_avg;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Program insns ~ " << prog.size() << "\n";
    std::cout << "Cold avg: " << cold_avg << " us/run (miss+translate)\n";
    std::cout << "Hot  avg: " << hot_avg << " us/run (hit+exec)\n";
    std::cout << "Speedup:  " << speedup << "x (cold/hot)\n";
}
