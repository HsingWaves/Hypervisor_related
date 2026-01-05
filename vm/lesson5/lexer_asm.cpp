// lexer_asm.cpp  (C++20)
// g++ -std=c++20 -O2 lexer_asm.cpp -o sasm
// Usage: ./sasm input.sasm
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using i32 = std::int32_t;
using u32 = std::uint32_t;
using strings = std::vector<std::string>;

struct Lexer {
    // Tokenize input into a vector of tokens.
    // Rules:
    //  - Whitespace splits tokens
    //  - '//' starts a line comment (until '\n')
    //  - Single-char tokens: ()[]{}+-*/,
    //  - String literal: " ... " supports escapes \" \\ \n \t \r
    //  - Parenthesis block: (...) captured as ONE token, supports nesting.
    strings lex(std::string_view s) {
        strings out;
        std::size_t i = 0;

        auto is_space = [](char c) {
            switch (c) {
            case ' ': case '\t': case '\n': case '\r': case '\v': case '\f':
                return true;
            default:
                return false;
            }
            };

        auto peek = [&](std::size_t k = 0) -> char {
            if (i + k >= s.size()) return '\0';
            return s[i + k];
            };

        auto starts_with_comment = [&]() -> bool {
            return peek(0) == '/' && peek(1) == '/';
            };

        auto push_token = [&](std::string tok) {
            if (!tok.empty()) out.push_back(std::move(tok));
            };

        auto read_line_comment = [&]() {
            // consume leading //
            i += 2;
            while (i < s.size() && s[i] != '\n') i++;
            // newline is consumed by main loop whitespace skip
            };

        auto read_string = [&]() -> std::string {
            // assumes s[i] == '"'
            std::string tok;
            tok.push_back('"');
            i++; // consume initial "

            while (i < s.size()) {
                char c = s[i++];
                tok.push_back(c);

                if (c == '\\') {
                    // keep escaped char as-is if exists
                    if (i < s.size()) {
                        tok.push_back(s[i++]);
                    }
                    continue;
                }
                if (c == '"') {
                    // end of string
                    break;
                }
            }
            return tok;
            };

        auto read_paren_block = [&]() -> std::string {
            // Read a (...) block as a single token, supports nesting.
            // assumes s[i] == '('
            std::string tok;
            int depth = 0;

            while (i < s.size()) {
                char c = s[i];

                // handle comments inside? we keep it simple: treat // as comment even in block
                if (starts_with_comment()) {
                    read_line_comment();
                    continue;
                }

                if (c == '"') {
                    // include string literal fully
                    tok += read_string();
                    continue;
                }

                // normal char
                tok.push_back(c);
                i++;

                if (c == '(') depth++;
                else if (c == ')') {
                    depth--;
                    if (depth == 0) break; // consumed matching ')'
                }
            }
            return tok;
            };

        auto is_single_char_token = [](char c) -> bool {
            switch (c) {
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '+':
            case '-':
            case '*':
            case '/':
            case ',':
                return true;
            default:
                return false;
            }
            };

        while (i < s.size()) {
            // skip whitespace
            if (is_space(peek())) {
                i++;
                continue;
            }

            // comment
            if (starts_with_comment()) {
                read_line_comment();
                continue;
            }

            char c = peek();

            // string literal token
            if (c == '"') {
                push_token(read_string());
                continue;
            }

            // parenthesis block token as ONE token (optional feature)
            if (c == '(') {
                push_token(read_paren_block());
                continue;
            }

            // single-char token
            if (is_single_char_token(c)) {
                // Note: '/' could be start of comment, already handled above.
                push_token(std::string(1, c));
                i++;
                continue;
            }

            // read a "word/number" token until whitespace or special
            std::string tok;
            while (i < s.size()) {
                char x = peek();
                if (is_space(x)) break;
                if (starts_with_comment()) break;
                if (x == '"' || x == '(') break;
                if (is_single_char_token(x)) break;
                tok.push_back(x);
                i++;
            }
            push_token(std::move(tok));
        }

        return out;
    }
};

struct Assembler {
    // Instruction encoding:
    //  - top 2 bits: type
    //    0 => positive integer literal
    //    1 => primitive instruction
    //    2 => negative integer literal
    //  - low 30 bits: data
    //
    // primitives:
    //  opcode 0 halt
    //  opcode 1 add (+)
    //  opcode 2 sub (-)
    //  opcode 3 mul (*)
    //  opcode 4 div (/)

    static constexpr u32 TYPE_POS = 0u;
    static constexpr u32 TYPE_PRIM = 1u;
    static constexpr u32 TYPE_NEG = 2u;

    static u32 pack(u32 type, u32 data30) {
        return (type << 30) | (data30 & 0x3FFFFFFFu);
    }

    static bool parse_int32(std::string_view sv, i32& out) {
        // supports optional leading +/-
        if (sv.empty()) return false;
        std::size_t idx = 0;
        bool neg = false;
        if (sv[0] == '+' || sv[0] == '-') {
            neg = (sv[0] == '-');
            idx = 1;
            if (idx >= sv.size()) return false;
        }
        // digits
        i32 val = 0;
        for (; idx < sv.size(); idx++) {
            char c = sv[idx];
            if (c < '0' || c > '9') return false;
            int d = c - '0';
            // overflow-safe-ish for our needs
            if (val > (std::numeric_limits<i32>::max() - d) / 10) return false;
            val = val * 10 + d;
        }
        out = neg ? -val : val;
        return true;
    }

    static u32 encode_literal(i32 v) {
        // We only have 30 bits for data.
        // Represent positive literals as TYPE_POS with data=v
        // Represent negative literals as TYPE_NEG with data=abs(v)
        // (Your VM currently doesn't reconstruct negatives; but this encoding is correct for future fix.)
        // Range:
        //  abs(v) must fit in 30 bits: 0 .. 2^30-1
        constexpr i32 MAX30 = (1 << 30) - 1;

        if (v >= 0) {
            if (v > MAX30) {
                throw std::runtime_error("Literal too large for 30-bit data: " + std::to_string(v));
            }
            return pack(TYPE_POS, static_cast<u32>(v));
        }
        else {
            i32 a = (v == std::numeric_limits<i32>::min()) ? (MAX30 + 1) : -v;
            if (a > MAX30) {
                throw std::runtime_error("Negative literal magnitude too large for 30-bit data: " + std::to_string(v));
            }
            return pack(TYPE_NEG, static_cast<u32>(a));
        }
    }

    static u32 encode_prim(u32 opcode) {
        return pack(TYPE_PRIM, opcode);
    }

    std::vector<i32> compile(const strings& toks) {
        static const std::unordered_map<std::string, u32> prim = {
            {"halt", 0},
            {"+", 1},
            {"-", 2},
            {"*", 3},
            {"/", 4},
        };

        std::vector<i32> out;
        out.reserve(toks.size());

        for (const auto& t : toks) {
            if (t.empty()) continue;

            // ignore parentheses blocks or strings for now (not part of your VM instruction set)
            // You can extend later.
            if (t.front() == '"' && t.size() >= 2 && t.back() == '"') {
                // For now: reject strings
                throw std::runtime_error("String token not supported by this assembler yet: " + t);
            }
            if (t.front() == '(') {
                // For now: reject blocks
                throw std::runtime_error("Paren-block token not supported by this assembler yet: " + t);
            }

            // primitive?
            if (auto it = prim.find(t); it != prim.end()) {
                out.push_back(static_cast<i32>(encode_prim(it->second)));
                continue;
            }

            // integer literal?
            i32 v = 0;
            if (parse_int32(t, v)) {
                out.push_back(static_cast<i32>(encode_literal(v)));
                continue;
            }

            // allow bracket tokens to pass through as errors (explicit)
            throw std::runtime_error("Invalid token/instruction: [" + t + "]");
        }

        return out;
    }
};

static std::string read_all_text(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error(std::string("Cannot open file: ") + path);
    std::string s((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return s;
}

static void write_bin(const char* path, const std::vector<i32>& code) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error(std::string("Cannot open output file: ") + path);
    for (i32 ins : code) {
        out.write(reinterpret_cast<const char*>(&ins), sizeof(ins));
    }
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <input.sasm>\n";
            return 1;
        }

        std::string text = read_all_text(argv[1]);

        Lexer lx;
        strings toks = lx.lex(text);

        // Optional: debug print tokens
        // for (auto& t : toks) std::cerr << "[" << t << "]\n";

        Assembler as;
        std::vector<i32> code = as.compile(toks);

        write_bin("out.bin", code);

        std::cerr << "OK: wrote " << code.size() << " instructions to out.bin\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
