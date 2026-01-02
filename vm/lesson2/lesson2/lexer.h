// lexer.h
#ifndef LEXER_H
#define LEXER_H

#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using strings = std::vector<std::string>;

class Lexer {
public:
    strings lex(std::string_view s);

private:
    static bool my_isspace(char c) {
        return std::isspace(static_cast<unsigned char>(c)) != 0;
    }

    //   special collections
    static bool isspecial(char c) {
        switch (c) {
        case '(': case ')':
        case '[': case ']':
        case '{': case '}':
        case ',': case ';':
        case '=': case '+': case '-':
        case '*': case '/':
        case '<': case '>':
        case '!': case '&': case '|':
        case ':':
            return true;
        default:
            return false;
        }
    }

    static bool is_group_begin(char c) {
        return c == '(' || c == '[' || c == '{' || c == '"';
    }

    static char group_end(char beg) {
        switch (beg) {
        case '(': return ')';
        case '[': return ']';
        case '{': return '}';
        case '"': return '"';
        default:  return '\0';
        }
    }
};

#endif
