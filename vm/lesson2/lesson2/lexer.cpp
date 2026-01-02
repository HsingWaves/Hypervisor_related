// lexer.cpp
#include "lexer.h"

strings Lexer::lex(std::string_view s) {
    enum class State : std::uint8_t {
        START,
        READWORD,
        READBLOCK,
        COMMENT
    };

    strings out;
    std::string token;
    token.reserve(64);

    State st = State::START;

    // block 相关
    char beg_char = '\0';
    char end_char = '\0';
    int  balance = 0;

    auto flush = [&]() {
        if (!token.empty()) {
            out.push_back(std::move(token));
            token.clear();
            token.reserve(64);
        }
        };

    const std::size_t n = s.size();
    std::size_t i = 0;

    while (i < n) {
        const char c = s[i];

        switch (st) {
        case State::START: {
            if (my_isspace(c)) {
                ++i;
                break;
            }

            // 行注释 //
            if (c == '/' && i + 1 < n && s[i + 1] == '/') {
                i += 2;
                st = State::COMMENT;
                break;
            }

            // group block: (),[],{}, "..."
            if (is_group_begin(c)) {
                flush(); // 保险：如果之前有残留
                beg_char = c;
                end_char = group_end(beg_char);
                balance = 1;

                token.push_back(c);
                ++i;

                st = State::READBLOCK;
                break;
            }

            // special 单字符 token（但把 // 已在上面优先处理）
            if (isspecial(c)) {
                flush();
                token.push_back(c);
                ++i;
                flush();
                break;
            }

            // 普通单词开始
            token.push_back(c);
            ++i;
            st = State::READWORD;
            break;
        }

        case State::READWORD: {
            if (i >= n) {
                flush();
                st = State::START;
                break;
            }

            const char x = s[i];

            if (my_isspace(x)) {
                flush();
                ++i;
                st = State::START;
                break;
            }

            // 行注释：先把当前词吐出，再进注释
            if (x == '/' && i + 1 < n && s[i + 1] == '/') {
                flush();
                i += 2;
                st = State::COMMENT;
                break;
            }

            // 遇到 group：先吐出当前词，再进入 block
            if (is_group_begin(x)) {
                flush();
                beg_char = x;
                end_char = group_end(beg_char);
                balance = 1;

                token.push_back(x);
                ++i;

                st = State::READBLOCK;
                break;
            }

            // 遇到 special：先吐出当前词，再把 special 单独吐出
            if (isspecial(x)) {
                flush();
                token.push_back(x);
                ++i;
                flush();
                st = State::START;
                break;
            }

            // 普通字符追加
            token.push_back(x);
            ++i;
            break;
        }

        case State::READBLOCK: {
            if (i >= n) {
                // 未闭合就结束：把已有 token 吐出（你也可以选择报错）
                flush();
                st = State::START;
                break;
            }

            const char x = s[i];

            // 字符串 block：处理转义
            if (beg_char == '"' && x == '\\') {
                // 复制 \ 和后一个字符（若存在）
                token.push_back(x);
                if (i + 1 < n) {
                    token.push_back(s[i + 1]);
                    i += 2;
                }
                else {
                    ++i;
                }
                break;
            }

            // 追加当前字符
            token.push_back(x);
            ++i;

            // 字符串结束：遇到未被转义的 "
            if (beg_char == '"' && x == '"') {
                balance = 0;
                flush();
                st = State::START;
                break;
            }

            // 括号类 block：支持嵌套
            if (beg_char != '"') {
                if (x == beg_char) {
                    ++balance;
                }
                else if (x == end_char) {
                    --balance;
                    if (balance == 0) {
                        flush();
                        st = State::START;
                    }
                }
            }
            break;
        }

        case State::COMMENT: {
            // 跳过到行尾（含 \n）
            if (c == '\n') {
                ++i;
                st = State::START;
            }
            else {
                ++i;
            }
            break;
        }
        }
    }

    // 收尾
    flush();
    return out;
}
