#include "tokenizer.h"
#include <cctype>
#include <iostream>

using namespace std;

Tokenizer::Tokenizer(const string& s) : src(s), pos(0) {}

void Tokenizer::skip() {
    while (pos < src.size()) {
        if (isspace(src[pos])) { ++pos; continue; }
        if (src[pos] == ';') {
            while (pos < src.size() && src[pos] != '\n') ++pos;
            continue;
        }
        break;
    }
}

Token Tokenizer::next() {
    skip();
    if (pos >= src.size()) return {TokenType::Eof, ""};
    char c = src[pos];

    if (c == '(') { ++pos; return {TokenType::LParen, "("}; }
    if (c == ')') { ++pos; return {TokenType::RParen, ")"}; }
    if (c == '[') { ++pos; return {TokenType::LBrack, "["}; }
    if (c == ']') { ++pos; return {TokenType::RBrack, "]"}; }

    // strings
    if (c == '"') {
        string s;
        ++pos;
        while (pos < src.size() && src[pos] != '"') {
            if (src[pos] == '\\') { ++pos; }
            s += src[pos++];
        }
        if (pos < src.size()) ++pos;
        return {TokenType::Str, s};
    }

    // character literals #\ ...
    if (c == '#' && pos + 1 < src.size() && src[pos+1] == '\\') {
        pos += 2;
        string name;
        while (pos < src.size() && isalpha(src[pos])) { name += src[pos++]; }
        if (name == "newline") return {TokenType::Char, "\n"};
        if (name == "space")   return {TokenType::Char, " "};
        if (name == "tab")     return {TokenType::Char, "\t"};
        if (name == "null")    return {TokenType::Char, string(1,'\0')};
        if (name.empty())     return {TokenType::Char, string(1, ' ')};
        return {TokenType::Char, name.substr(0,1)};
    }

    // booleans #t #f
    if (c == '#' && pos + 1 < src.size()) {
        if (src[pos+1] == 't') { pos += 2; return {TokenType::Bool, "1"}; }
        if (src[pos+1] == 'f') { pos += 2; return {TokenType::Bool, "0"}; }
    }

    // operators (check 2-char first)
    if (pos + 1 < src.size()) {
        string two = string(1,c) + src[pos+1];
        if (two == "!=" || two == "<=" || two == ">=") { pos += 2; return {TokenType::Op, two}; }
    }
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
        c == '<' || c == '>' || c == '=' || c == '!') {
        ++pos; return {TokenType::Op, string(1,c)};
    }

    // identifiers / keywords
    if (isalpha(c) || c == '_') {
        string id;
        while (pos < src.size() && (isalnum(src[pos]) || src[pos] == '_' || src[pos] == '-' || src[pos] == '?')) {
            id += src[pos++];
        }
        if (id == "def")     return {TokenType::Def,    id};
        if (id == "loop")    return {TokenType::Loop,   id};
        if (id == "while")   return {TokenType::While,  id};
        if (id == "for")     return {TokenType::For,    id};
        if (id == "if")      return {TokenType::If,     id};
        if (id == "cond")    return {TokenType::Cond,   id};
        if (id == "mac")    return {TokenType::Mac,    id};
        if (id == "import")  return {TokenType::Import, id};
        if (id == "ffi")    return {TokenType::Ffi,    id};
        if (id == "llvm")   return {TokenType::Raw,    id};
        if (id == "not")    return {TokenType::Ident,  id};
        return {TokenType::Ident, id};
    }

    // numbers
    bool isSign = (c == '-' || c == '+') && pos + 1 < src.size() &&
                  (isdigit(src[pos+1]) || src[pos+1] == '.');
    if (isdigit(c) || c == '.' || isSign) {
        string num;
        if (c == '-' || c == '+') num += src[pos++];
        bool hasDot = false;
        while (pos < src.size() && (isdigit(src[pos]) || src[pos] == '.')) {
            if (src[pos] == '.') hasDot = true;
            num += src[pos++];
        }
        return {hasDot ? TokenType::Float : TokenType::Int, num};
    }

    ++pos;
    return {TokenType::Ident, string(1,c)};
}