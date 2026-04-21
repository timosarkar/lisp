#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include <vector>

enum class TokenType {
    Int, Float, Str, Char, Bool,
    Op, Ident,
    Def, Loop, While, For, If, Cond,
    Mac, Import, Ffi, Raw,
    LParen, RParen, LBrack, RBrack,
    Eof
};

struct Token {
    TokenType type;
    std::string val;
};

class Tokenizer {
public:
    Tokenizer(const std::string& src);
    Token next();
    void adv() { cur = next(); }
    Token cur;
    size_t pos = 0;

private:
    std::string src;
    void skip();
};

#endif