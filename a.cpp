#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <map>
#include <vector>
#include <functional>
using namespace std;
namespace fs = filesystem;

constexpr auto VERSION = "2.0.0";

enum class TokenType {
    Int, Float, Str, Char, Bool,
    Op, Ident,
    Def, Loop, While, For, If, Cond,
    Mac, Import, Ffi, Raw,
    LParen, RParen, LBrack, RBrack,
    Eof
};

enum class Type { Int, Float, Bool, Char, Ptr, FuncRef };

struct Token { TokenType type; string val; };
struct Value { Type type; string name; bool ptr = false; };

// ─── Macro AST node (stores raw tokens) ──────────────────────────────────────
struct MacroTemplate {
    vector<string> params;       // parameter names
    vector<Token>  body;         // token stream of the body
};

class Compiler {
    string src;
    string blk = "entry";
    size_t pos  = 0;
    int    tmp  = 0, lbl = 0;
    ostringstream ir, allocs, funcDefs, externDecls;
    Token cur;

    map<string, Value>         vars;
    map<string, MacroTemplate> macros;

    struct Func { Type retType; vector<Type> paramTypes; };
    map<string, Func> funcs;

    // string literal pool
    int strIdx = 0;
    ostringstream strDefs;

    static inline map<string, pair<string,string>> ops = {
        {"+", {"add","fadd"}}, {"-", {"sub","fsub"}},
        {"*", {"mul","fmul"}}, {"/", {"sdiv","fdiv"}},
        {"%", {"srem","frem"}}
    };
    static inline map<string, pair<string,string>> cmps = {
        {"<", {"slt","olt"}}, {">", {"sgt","ogt"}},
        {"=", {"eq","oeq"}},  {"!=",{"ne","one"}},
        {"<=",{"sle","ole"}}, {">=",{"sge","oge"}}
    };

    // ── error with context ─────────────────────────────────────────────────────
    size_t errLine = 0, errCol = 0;

    void updateErrPos() {
        errLine = 1; errCol = 1;
        for (size_t i = 0; i < pos && i < src.size(); ++i) {
            if (src[i] == '\n') { errLine++; errCol = 1; }
            else errCol++;
        }
    }

    string getErrLine() {
        size_t start = pos;
        while (start > 0 && src[start-1] != '\n') start--;
        size_t end = pos;
        while (end < src.size() && src[end] != '\n') end++;
        return src.substr(start, end - start);
    }

    [[noreturn]] void err(const char* m) {
        updateErrPos();
        cerr << "\n========================================\n";
        cerr << "ERROR at line " << errLine << ", col " << errCol << "\n";
        string line = getErrLine();
        if (!line.empty()) {
            if (line.size() > 50) line = line.substr(0, 50) + "...";
            cerr << "Context: " << line << "\n";
            for (size_t i = 0; i < 14 + errCol && i < 50; i++) cerr << " ";
            cerr << "^\n";
        }
        cerr << "Message: " << m << "\n";
        cerr << "========================================\n";
        exit(1);
    }

    [[noreturn]] void err(const string& m) { err(m.c_str()); }

    // ── expect with helpful message ──────────────────────────────────────────
    [[noreturn]] void expected(const char* what) {
        updateErrPos();
        string got = "unknown";
        switch (cur.type) {
            case TokenType::LParen: got = "'('"; break;
            case TokenType::RParen: got = "')'"; break;
            case TokenType::LBrack: got = "'['"; break;
            case TokenType::RBrack: got = "']'"; break;
            case TokenType::Int: got = "integer"; break;
            case TokenType::Float: got = "float"; break;
            case TokenType::Str: got = "string"; break;
            case TokenType::Char: got = "char"; break;
            case TokenType::Bool: got = "bool"; break;
            case TokenType::Op: got = "operator '" + cur.val + "'"; break;
            case TokenType::Ident: got = "identifier '" + cur.val + "'"; break;
            case TokenType::Eof: got = "end of file"; break;
            default: got = "token"; break;
        }
        cerr << "\n========================================\n";
        cerr << "SYNTAX ERROR at line " << errLine << ", col " << errCol << "\n";
        string line = getErrLine();
        if (!line.empty()) {
            if (line.size() > 50) line = line.substr(0, 50) + "...";
            cerr << "Context: " << line << "\n";
        }
        cerr << "Expected: " << what << "\n";
        cerr << "Got: " << got << "\n";
        if (cur.type == TokenType::Ident && cur.val == "def")
            cerr << "Hint: Use '(def name value)' not '(define ...)'" << "\n";
        if (cur.type == TokenType::Ident && cur.val == "macro")
            cerr << "Hint: Use '(mac name ...)' not '(macro ...)'" << "\n";
        if (cur.type == TokenType::Ident && cur.val == "extern")
            cerr << "Hint: Use '(ffi ...)' not '(extern ...)'" << "\n";
        if (cur.type == TokenType::Ident && cur.val == "raw-ir")
            cerr << "Hint: Use '(llvm ...)' not '(raw-ir ...)'" << "\n";
        cerr << "========================================\n";
        exit(1);
    }

    void expect(TokenType t, const char* what) {
        if (cur.type != t) expected(what);
        adv();
    }

    string ty(Type t) {
        switch (t) {
            case Type::Float: return "double";
            case Type::Bool:  return "i1";
            case Type::Char:  return "i8";
            case Type::Ptr:   return "i8*";
            case Type::FuncRef: return "i32*";
            default:          return "i32";
        }
    }

    string t()  { return "%" + to_string(tmp++); }
    string L()  { return "L" + to_string(lbl++); }

    // ── lexer ─────────────────────────────────────────────────────────────────
    void skip() {
        while (pos < src.size()) {
            if (isspace(src[pos])) { ++pos; continue; }
            if (src[pos] == ';') {
                while (pos < src.size() && src[pos] != '\n') ++pos;
                continue;
            }
            break;
        }
    }

    Token next() {
        skip();
        if (pos >= src.size()) return {TokenType::Eof, ""};
        char c = src[pos];

        // single-char tokens
        if (c == '(') { ++pos; return {TokenType::LParen, "("}; }
        if (c == ')') { ++pos; return {TokenType::RParen, ")"}; }
        if (c == '[') { ++pos; return {TokenType::LBrack, "["}; }
        if (c == ']') { ++pos; return {TokenType::RBrack, "]"}; }

        // string literals  "..."
        if (c == '"') {
            ++pos;
            string s;
            while (pos < src.size() && src[pos] != '"') {
                if (src[pos] == '\\' && pos + 1 < src.size()) {
                    ++pos;
                    switch (src[pos]) {
                        case 'n': s += '\n'; break;
                        case 't': s += '\t'; break;
                        case 'r': s += '\r'; break;
                        case '0': s += '\0'; break;
                        default:  s += src[pos]; break;
                    }
                } else {
                    s += src[pos];
                }
                ++pos;
            }
            if (pos >= src.size()) err("unterminated string");
            ++pos;
            return {TokenType::Str, s};
        }

        // char literals  #\a  #\newline  #\space
        if (c == '#' && pos + 1 < src.size() && src[pos+1] == '\\') {
            pos += 2;
            string name;
            while (pos < src.size() && (isalnum(src[pos]) || src[pos] == '_')) name += src[pos++];
            if (name == "newline") return {TokenType::Char, "\n"};
            if (name == "space")   return {TokenType::Char, " "};
            if (name == "tab")     return {TokenType::Char, "\t"};
            if (name == "null")    return {TokenType::Char, string(1,'\0')};
            if (name.size() == 1)  return {TokenType::Char, name};
            err("unknown char name: " + name);
        }

        // bool literals  #t  #f
        if (c == '#' && pos + 1 < src.size()) {
            if (src[pos+1] == 't') { pos += 2; return {TokenType::Bool, "1"}; }
            if (src[pos+1] == 'f') { pos += 2; return {TokenType::Bool, "0"}; }
        }

        // 2-char operators  !=  <=  >=
        if (pos + 1 < src.size()) {
            string two = string(1,c) + src[pos+1];
            if (ops.count(two) || cmps.count(two)) { pos += 2; return {TokenType::Op, two}; }
        }

        // 1-char operators
        {
            string one(1, c);
            if (ops.count(one) || cmps.count(one)) { ++pos; return {TokenType::Op, one}; }
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

        err("unexpected character: " + string(1,c));
    }

    void adv() { cur = next(); }

    // ── type parsing  (int float bool char ptr) ───────────────────────────────
    Type parseTypeName() {
        if (cur.type != TokenType::Ident) err("expected type name");
        string n = cur.val; adv();
        if (n == "int")   return Type::Int;
        if (n == "float") return Type::Float;
        if (n == "bool")  return Type::Bool;
        if (n == "char")  return Type::Char;
        if (n == "ptr")   return Type::Ptr;
        err("unknown type: " + n);
    }

    // ── value helpers ─────────────────────────────────────────────────────────
    Value load(Value v) {
        if (!v.ptr) return v;
        string reg = t();
        ir << "  " << reg << " = load " << ty(v.type) << ", "
           << ty(v.type) << "* " << v.name << "\n";
        return {v.type, reg};
    }

    // Convert i32 to i1 for br i1 instructions
    Value condToI1(Value v) {
        v = load(v);
        if (v.type == Type::Int) {
            string reg = t();
            ir << "  " << reg << " = icmp ne i32 " << v.name << ", 0\n";
            return {Type::Bool, reg};
        }
        return v;
    }

    Value conv(Value v) {
        v = load(v);
        if (v.type == Type::Int) {
            string reg = t();
            ir << "  " << reg << " = sitofp i32 " << v.name << " to double\n";
            return {Type::Float, reg};
        }
        return v;
    }

    Value binop(const string& op, Value left, Value right) {
        left  = load(left);
        right = load(right);
        bool isFloat = (left.type == Type::Float || right.type == Type::Float);
        if (isFloat) { left = conv(left); right = conv(right); }
        string res = t();
        string opcode = isFloat ? ops[op].second : ops[op].first;
        Type rtype = isFloat ? Type::Float : Type::Int;
        ir << "  " << res << " = " << opcode << " " << ty(rtype)
           << " " << left.name << ", " << right.name << "\n";
        return {rtype, res};
    }

    Value cmpop(const string& op, Value left, Value right) {
        left  = load(left);
        right = load(right);
        bool isFl = (left.type == Type::Float || right.type == Type::Float);
        if (isFl) { left = conv(left); right = conv(right); }
        string res = t();
        ir << "  " << res << " = " << (isFl ? "fcmp o" : "icmp ")
           << (isFl ? cmps[op].second : cmps[op].first) << " "
           << ty(isFl ? Type::Float : Type::Int) << " "
           << left.name << ", " << right.name << "\n";
        return {Type::Bool, res};
    }

    // ── string-literal helper  ─────────────────────────────────────────────────
    // Emits a global constant and returns an i8* pointer value
    Value makeString(const string& s) {
        // build escaped LLVM string
        string llvmStr;
        size_t len = 0;
        for (unsigned char ch : s) {
            if (ch == '"' || ch == '\\' || ch < 0x20 || ch > 0x7e) {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\%02X", ch);
                llvmStr += buf;
            } else {
                llvmStr += ch;
            }
            ++len;
        }
        llvmStr += "\\00"; // null terminator

        int idx = strIdx++;
        string gname = "@.str." + to_string(idx);
        size_t totalLen = len + 1;
        strDefs << gname << " = private constant [" << totalLen << " x i8] c\""
                << llvmStr << "\"\n";

        string reg = t();
        ir << "  " << reg << " = getelementptr [" << totalLen << " x i8], ["
           << totalLen << " x i8]* " << gname << ", i32 0, i32 0\n";
        return {Type::Ptr, reg};
    }

    // ── macro expansion ───────────────────────────────────────────────────────
    // Temporarily splice the macro body (with substituted params) into src
    Value expandMacro(const string& name) {
        auto& m = macros[name];

        // collect arguments (each arg is a balanced token sequence)
        vector<vector<Token>> args;
        while (cur.type != TokenType::RParen) {
            vector<Token> arg;
            int depth = 0;
            while (!(depth == 0 && (cur.type == TokenType::RParen ||
                                    (arg.size() > 0 && cur.type != TokenType::RParen &&
                                     args.size() < m.params.size() - 1 &&
                                     cur.type != TokenType::LParen)))) {
                // collect one sexp
                if (cur.type == TokenType::Eof) err("unexpected eof in macro arg");
                if (cur.type == TokenType::LParen) ++depth;
                if (cur.type == TokenType::RParen) {
                    if (depth == 0) break;
                    --depth;
                }
                arg.push_back(cur);
                adv();
                if (depth == 0) break;
            }
            args.push_back(arg);
        }
        adv(); // consume ')'

        if (args.size() != m.params.size())
            err("macro " + name + " expects " + to_string(m.params.size()) + " args");

        // build substituted token stream
        map<string, vector<Token>> subst;
        for (size_t i = 0; i < m.params.size(); ++i)
            subst[m.params[i]] = args[i];

        // serialize substituted body into a string, then re-lex
        ostringstream expanded;
        for (auto& tok : m.body) {
            if (tok.type == TokenType::Ident && subst.count(tok.val)) {
                for (auto& at : subst[tok.val]) expanded << " " << at.val;
            } else {
                expanded << " " << tok.val;
            }
        }

        // save & restore lexer state
        string savedSrc = src;
        size_t savedPos = pos;
        Token  savedCur = cur;

        src = expanded.str();
        pos = 0;
        adv();
        Value result = {Type::Int, ""};
        while (cur.type != TokenType::Eof && cur.type != TokenType::RParen) result = load(parse());

        src = savedSrc;
        pos = savedPos;
        cur = savedCur;
        return result;
    }

    // ── main parser ───────────────────────────────────────────────────────────
    Value parse() {
        // ── (sexp ...) ──
        if (cur.type == TokenType::LParen) {
            adv();

            // ── (macro name (params...) body) ──────────────────────────────
            if (cur.type == TokenType::Mac) {
                adv();
                string mname = cur.val;
                expect(TokenType::Ident, "expected macro name");
                expect(TokenType::LParen, "expected (");
                vector<string> params;
                while (cur.type == TokenType::Ident) { params.push_back(cur.val); adv(); }
                expect(TokenType::RParen, "expected )");

                // collect body tokens (balanced up to final ')')
                vector<Token> body;
                int depth = 1; // we've already consumed the outer '('
                while (depth > 0) {
                    if (cur.type == TokenType::Eof) err("unterminated macro body");
                    if (cur.type == TokenType::LParen) ++depth;
                    if (cur.type == TokenType::RParen) { --depth; if (depth == 0) break; }
                    body.push_back(cur);
                    adv();
                }
                adv(); // consume final ')'

                macros[mname] = {params, body};
                return {Type::Int, ""};
            }

            // ── (import "file.tsp") ────────────────────────────────────────
            if (cur.type == TokenType::Import) {
                adv();
                if (cur.type != TokenType::Str) err("import expects a string path");
                string path = cur.val; adv();
                expect(TokenType::RParen, "expected )");

                ifstream ifs(path);
                if (!ifs) err("cannot open import: " + path);
                string imported((istreambuf_iterator<char>(ifs)), {});

                // splice imported source into current stream
                string tail = src.substr(pos);
                src = imported + "\n" + tail;
                pos = 0;
                adv();
                return {Type::Int, ""};
            }

            // ── (extern ret-type name (param-types...)) ───────────────────
            if (cur.type == TokenType::Ffi) {
                adv();
                Type retT = parseTypeName();
                string fname = cur.val;
                expect(TokenType::Ident, "expected function name");
                expect(TokenType::LParen, "expected (");
                vector<Type> paramTypes;
                bool vararg = false;
                while (cur.type != TokenType::RParen) {
                    if (cur.type == TokenType::Op && cur.val == ".") {
                        vararg = true; adv();
                        break;
                    }
                    paramTypes.push_back(parseTypeName());
                }
                expect(TokenType::RParen, "expected )");
                expect(TokenType::RParen, "expected )");

                // emit extern declaration
                ostringstream decl;
                decl << "declare " << ty(retT) << " @" << fname << "(";
                for (size_t i = 0; i < paramTypes.size(); ++i)
                    decl << (i ? ", " : "") << ty(paramTypes[i]);
                if (vararg) decl << (paramTypes.empty() ? "" : ", ") << "...";
                decl << ")\n";
                externDecls << decl.str();

                funcs[fname] = {retT, paramTypes};
                return {Type::Int, ""};
            }

            // ── (raw-ir "...llvm ir...") ───────────────────────────────────
            if (cur.type == TokenType::Raw) {
                adv();
                if (cur.type != TokenType::Str) err("raw-ir expects a string");
                string rawCode = cur.val; adv();
                expect(TokenType::RParen, "expected )");
                ir << rawCode << "\n";
                return {Type::Int, ""};
            }

            // ── (define ...) ──────────────────────────────────────────────
            if (cur.type == TokenType::Def) {
                adv();

                // (define (name [type] args...) body)
                if (cur.type == TokenType::LParen) {
                    adv();
                    string fname = cur.val; adv();

                    // optional return type annotation: (define (name : ret-type args...) ...)
                    Type explicitRet = Type::Int;
                    bool hasRetType = false;
                    if (cur.type == TokenType::Op && cur.val == ":") {
                        adv(); explicitRet = parseTypeName(); hasRetType = true;
                    }

                    // params: name or (name type)
                    vector<pair<string,Type>> params;
                    while (cur.type != TokenType::RParen) {
                        if (cur.type == TokenType::LParen) {
                            adv();
                            string pname = cur.val; adv();
                            Type   ptype = parseTypeName();
                            expect(TokenType::RParen, "expected )");
                            params.push_back({pname, ptype});
                        } else {
                            string pname = cur.val; adv();
                            params.push_back({pname, Type::Int});
                        }
                    }
                    expect(TokenType::RParen, "expected )");

                    auto savedVars   = vars;
                    auto savedIr     = ir.str();
                    auto savedAllocs = allocs.str();
                    int  savedTmp    = tmp;
                    string savedBlk  = blk;

                    vars.clear(); ir.str(""); allocs.str(""); tmp = 0; blk = "entry";

                    funcs[fname] = {explicitRet, {}};
                    for (auto& [pn, pt] : params) {
                        vars[pn] = {pt, "%" + pn, false};
                        funcs[fname].paramTypes.push_back(pt);
                    }

                    Value result = {Type::Int, ""};
                    while (cur.type != TokenType::RParen && cur.type != TokenType::Eof)
                        result = load(parse());
                    if (!hasRetType) funcs[fname].retType = result.type;
                    else result.type = explicitRet;

                    funcDefs << "define " << ty(funcs[fname].retType) << " @" << fname << "(";
                    for (size_t i = 0; i < params.size(); ++i)
                        funcDefs << (i ? ", " : "") << ty(params[i].second) << " %" << params[i].first;
                    funcDefs << ") {\nentry:\n" << allocs.str() << ir.str();
                    funcDefs << "  ret " << ty(funcs[fname].retType) << " " << result.name << "\n}\n\n";

                    vars = savedVars;
                    ir.str(savedIr);     ir.seekp(0, ios::end);
                    allocs.str(savedAllocs); allocs.seekp(0, ios::end);
                    tmp = savedTmp; blk = savedBlk;

                    expect(TokenType::RParen, "expected )");
                    return {Type::Int, ""};
                }

                // (define name value)
                string name = cur.val;
                expect(TokenType::Ident, "expected identifier");
                Value val = load(parse());

                if (!vars.count(name)) {
                    string ptr = t();
                    allocs << "  " << ptr << " = alloca " << ty(val.type) << "\n";
                    vars[name] = {val.type, ptr, true};
                }

                ir << "  store " << ty(val.type) << " " << val.name << ", "
                   << ty(val.type) << "* " << vars[name].name << "\n";
                expect(TokenType::RParen, "expected )");
                return {Type::Int, ""};
            }

            // ── (loop count body...) ──────────────────────────────────────
            if (cur.type == TokenType::Loop) {
                adv();
                Value count = load(parse());

                string preBlock    = blk;
                string headerBlock = L();
                string bodyBlock   = L();
                string exitBlock   = L();
                int phiId = tmp++;

                ir << "  br label %" << headerBlock << "\n";
                ir << headerBlock << ":\n";
                size_t phiPos = ir.str().size();
                ir << string(99, ' ') << "\n";
                string cond = t();
                ir << "  " << cond << " = icmp slt i32 %" << phiId << ", " << count.name << "\n";
                ir << "  br i1 " << cond << ", label %" << bodyBlock << ", label %" << exitBlock << "\n";
                ir << bodyBlock << ":\n";
                blk = bodyBlock;
                while (cur.type != TokenType::RParen) parse();
                string backBlock = blk;
                string next = t();
                ir << "  " << next << " = add i32 %" << phiId << ", 1\n";
                ir << "  br label %" << headerBlock << "\n";
                ir << exitBlock << ":\n";
                blk = exitBlock;

                string code = ir.str();
                string phi = "  %" + to_string(phiId) + " = phi i32 [0, %" + preBlock + "], [" + next + ", %" + backBlock + "]";
                phi.resize(99, ' ');
                code.replace(phiPos, 99, phi);
                ir.str(code); ir.seekp(0, ios::end);

                adv();
                return {Type::Int, ""};
            }

            // ── (while cond body...) ──────────────────────────────────────
            if (cur.type == TokenType::While) {
                adv();
                string headerBlock = L();
                string bodyBlock   = L();
                string exitBlock   = L();

                ir << "  br label %" << headerBlock << "\n";
                ir << headerBlock << ":\n";
                blk = headerBlock;

                Value cond = condToI1(parse()); // condition expression
                ir << "  br i1 " << cond.name << ", label %" << bodyBlock
                   << ", label %" << exitBlock << "\n";

                ir << bodyBlock << ":\n";
                blk = bodyBlock;
                while (cur.type != TokenType::RParen) parse();
                ir << "  br label %" << headerBlock << "\n";

                ir << exitBlock << ":\n";
                blk = exitBlock;

                adv();
                return {Type::Int, ""};
            }

            // ── (for var init cond step body...) ──────────────────────────
            if (cur.type == TokenType::For) {
                adv();

                // init: (define var expr) or bare name
                string varName = cur.val;
                expect(TokenType::Ident, "expected loop variable");
                Value initVal = load(parse());

                // allocate or reuse
                if (!vars.count(varName)) {
                    string ptr = t();
                    allocs << "  " << ptr << " = alloca " << ty(initVal.type) << "\n";
                    vars[varName] = {initVal.type, ptr, true};
                }
                ir << "  store " << ty(initVal.type) << " " << initVal.name
                   << ", " << ty(initVal.type) << "* " << vars[varName].name << "\n";

                string headerBlock = L();
                string bodyBlock   = L();
                string exitBlock   = L();

                ir << "  br label %" << headerBlock << "\n";
                ir << headerBlock << ":\n";
                blk = headerBlock;

                // condition
                Value cond = load(parse());
                ir << "  br i1 " << cond.name << ", label %" << bodyBlock
                   << ", label %" << exitBlock << "\n";

                // step expression — save for after body
                // We need to parse it but emit it after the body.
                // Strategy: parse step into a sub-buffer.
                ostringstream savedIrStream;
                savedIrStream << ir.str();
                ostringstream stepBuf;
                auto savedIrPtr = &ir;

                // Temporarily redirect ir to stepBuf to collect step IR
                ostringstream mainIrSave;
                mainIrSave << ir.str();
                ir.str(""); ir.clear();
                Value step = load(parse()); // parse step into ir (now stepBuf equivalent)
                // record variable update from step
                ir << "  store " << ty(step.type) << " " << step.name
                   << ", " << ty(step.type) << "* " << vars[varName].name << "\n";
                string stepIR = ir.str();
                ir.str(mainIrSave.str()); ir.seekp(0, ios::end);

                // body
                ir << bodyBlock << ":\n";
                blk = bodyBlock;
                while (cur.type != TokenType::RParen) parse();
                // emit step
                ir << stepIR;
                ir << "  br label %" << headerBlock << "\n";

                ir << exitBlock << ":\n";
                blk = exitBlock;

                adv();
                return {Type::Int, ""};
            }

            // ── (if cond then else) ───────────────────────────────────────
            if (cur.type == TokenType::If) {
                adv();
                Value c = condToI1(parse());
                string thenL = L(), elseL = L(), endL = L();
                ir << "  br i1 " << c.name << ", label %" << thenL << ", label %" << elseL << "\n";
                ir << thenL << ":\n"; blk = thenL;
                Value th = load(parse()); string thB = blk;
                ir << "  br label %" << endL << "\n";
                ir << elseL << ":\n"; blk = elseL;
                Value el = load(parse()); string elB = blk;
                ir << "  br label %" << endL << "\n";
                ir << endL << ":\n"; blk = endL;
                string res = t();
                ir << "  " << res << " = phi " << ty(th.type)
                   << " [" << th.name << ", %" << thB << "], [" << el.name << ", %" << elB << "]\n";
                expect(TokenType::RParen, "expected )");
                return {th.type, res};
            }

            // ── (cond [cond val] ...) ─────────────────────────────────────
            if (cur.type == TokenType::Cond) {
                adv();
                string endL = L();
                vector<pair<Value,string>> rs;
                while (cur.type == TokenType::LBrack) {
                    adv();
                    Value c = condToI1(parse());
                    string thenL = L(), nextL = L();
                    ir << "  br i1 " << c.name << ", label %" << thenL << ", label %" << nextL << "\n";
                    ir << thenL << ":\n"; blk = thenL;
                    Value r = load(parse()); rs.push_back({r, blk});
                    ir << "  br label %" << endL << "\n";
                    ir << nextL << ":\n"; blk = nextL;
                    expect(TokenType::RBrack, "expected ]");
                }
                rs.push_back({{Type::Int, "0"}, blk});
                ir << "  br label %" << endL << "\n";
                ir << endL << ":\n"; blk = endL;
                string res = t();
                ir << "  " << res << " = phi " << ty(rs[0].first.type);
                for (size_t i = 0; i < rs.size(); ++i)
                    ir << (i ? ", " : " ") << "[" << rs[i].first.name << ", %" << rs[i].second << "]";
                ir << "\n";
                expect(TokenType::RParen, "expected )");
                return {rs[0].first.type, res};
            }

            // ── macro expansion ───────────────────────────────────────────
            if (cur.type == TokenType::Ident && macros.count(cur.val)) {
                string mname = cur.val; adv();
                return expandMacro(mname);
            }

            // ── known function call ───────────────────────────────────────
            if (cur.type == TokenType::Ident && funcs.count(cur.val)) {
                string fname = cur.val; adv();
                vector<Value> args;
                while (cur.type != TokenType::RParen) args.push_back(load(parse()));
                adv();
                Func& fn = funcs[fname];
                string res = t();
                ir << "  " << res << " = call " << ty(fn.retType) << " @" << fname << "(";
                for (size_t i = 0; i < args.size(); ++i)
                    ir << (i ? ", " : "") << ty(args[i].type) << " " << args[i].name;
                ir << ")\n";
                return {fn.retType, res};
            }

            // ── operator expression ───────────────────────────────────────
            string op = cur.val;
            expect(TokenType::Op, "expected operator");
            Value acc = parse();
            if (cmps.count(op)) {
                acc = cmpop(op, acc, parse());
            } else {
                while (cur.type != TokenType::RParen)
                    acc = binop(op, acc, parse());
            }
            adv();
            return acc;
        }

        // ── variable reference ─────────────────────────────────────────────
        if (cur.type == TokenType::Ident) {
            string name = cur.val; adv();

            // If identifier is followed by LParen, check if it's a function call
            if (cur.type == TokenType::LParen) {
                // First check if it's a function in our map
                if (funcs.count(name)) {
                    // Known function call
                    vector<Value> args;
                    while (cur.type != TokenType::RParen) args.push_back(load(parse()));
                    adv();
                    Func& fn = funcs[name];
                    string res = t();
                    ir << "  " << res << " = call " << ty(fn.retType) << " @" << name << "(";
                    for (size_t i = 0; i < args.size(); ++i)
                        ir << (i ? ", " : "") << ty(args[i].type) << " " << args[i].name;
                    ir << ")\n";
                    return {fn.retType, res};
                }
                // Check if it's a variable (could hold a function reference)
                if (vars.count(name)) {
                    Value& var = vars[name];
                    vector<Value> args;
                    while (cur.type != TokenType::RParen) args.push_back(load(parse()));
                    adv();
                    string res = t();
                    // Load the function pointer from the variable and call it
                    string fptr = t();
                    ir << "  " << fptr << " = load i32*, i32** " << var.name << "\n";
                    ir << "  " << res << " = call i32 " << fptr << "(";
                    for (size_t i = 0; i < args.size(); ++i)
                        ir << (i ? ", " : "") << ty(args[i].type) << " " << args[i].name;
                    ir << ")\n";
                    return {Type::Int, res};
                }
                // Unknown function call
                err("undefined function: " + name);
            }

            // Function reference as value (bare identifier, not followed by LParen)
            if (funcs.count(name)) {
                // Return a FuncRef - store function pointer in a variable
                string res = t();
                string ptr = t();
                ir << "  " << ptr << " = alloca i32*\n";
                ir << "  " << res << " = bitcast i32 (...)* @" << name << " to i32*\n";
                ir << "  store i32* " << res << ", i32** " << ptr << "\n";
                return {Type::FuncRef, ptr, true};
            }

            // Just a variable reference
            if (!vars.count(name)) err("undefined variable: " + name);
            return vars[name];
        }

        // ── literals ──────────────────────────────────────────────────────
        if (cur.type == TokenType::Int) {
            Value v = {Type::Int, cur.val}; adv(); return v;
        }
        if (cur.type == TokenType::Float) {
            Value v = {Type::Float, cur.val}; adv(); return v;
        }
        if (cur.type == TokenType::Bool) {
            Value v = {Type::Bool, cur.val}; adv(); return v;
        }
        if (cur.type == TokenType::Char) {
            int code = (unsigned char)cur.val[0];
            Value v = {Type::Char, to_string(code)}; adv(); return v;
        }
        if (cur.type == TokenType::Str) {
            string s = cur.val; adv();
            return makeString(s);
        }

        err("unexpected token");
    }

public:
    string compile(const string& source) {
        src = source; pos = tmp = lbl = strIdx = 0; blk = "entry";
        ir.str(""); allocs.str(""); funcDefs.str(""); externDecls.str(""); strDefs.str("");
        vars.clear(); funcs.clear(); macros.clear();

        // register built-in printf/puts
        funcs["printf"] = {Type::Int, {Type::Ptr}};
        funcs["puts"]   = {Type::Int, {Type::Ptr}};
        funcs["putchar"]= {Type::Int, {Type::Char}};

        ostringstream hdr;
        hdr << "; generated by lisp " << VERSION << "\n"
            << "declare i32 @printf(i8*, ...)\n"
            << "declare i32 @puts(i8*)\n"
            << "declare i32 @putchar(i32)\n\n"
            << "@.fmt.int   = private constant [4 x i8] c\"%d\\0A\\00\"\n"
            << "@.fmt.float = private constant [4 x i8] c\"%f\\0A\\00\"\n"
            << "@.fmt.char  = private constant [4 x i8] c\"%c\\0A\\00\"\n"
            << "@.fmt.bool  = private constant [4 x i8] c\"%d\\0A\\00\"\n\n";

        for (adv(); cur.type != TokenType::Eof;) {
            Value res = load(parse());
            if (!res.name.empty()) {
                string fmt;
                switch (res.type) {
                    case Type::Float: fmt = "@.fmt.float"; break;
                    case Type::Char:  fmt = "@.fmt.char";  break;
                    case Type::Ptr:   /* print strings with puts */
                        ir << "  " << t() << " = call i32 @puts(i8* " << res.name << ")\n";
                        continue;
                    default:          fmt = "@.fmt.int";   break;
                }
                ir << "  " << t() << " = call i32 (i8*, ...) @printf("
                   << "i8* getelementptr ([4 x i8], [4 x i8]* " << fmt << ", i32 0, i32 0), "
                   << ty(res.type) << " " << res.name << ")\n";
            }
        }

        ir << "  ret i32 0\n}\n";

        return hdr.str()
             + externDecls.str() + "\n"
             + strDefs.str()     + "\n"
             + funcDefs.str()
             + "define i32 @main() {\nentry:\n"
             + allocs.str()
             + ir.str();
    }
};

// ─── CLI ─────────────────────────────────────────────────────────────────────

void help(const char* p) {
    cout << "Usage: " << p << " <input.tsp> [options]\n\n"
         << "Options:\n"
         << "  -o <output>   Specify output executable name\n"
         << "  --emit-ir     Emit LLVM IR only (.ll)\n"
         << "  --emit-asm    Emit assembly only (.s)\n"
         << "  --emit-obj    Emit object file only (.o)\n"
         << "  --verbose     Preserve all intermediates\n"
         << "  --help        Show this help message\n"
         << "  --version     Show version information\n\n"
         << "Language features (v2):\n"
         << "  Macros:   (macro name (params...) body)\n"
         << "  Import:   (import \"file.tsp\")\n"
         << "  Extern:   (extern ret-type name (param-types...))\n"
         << "  Raw IR:   (raw-ir \"...llvm ir...\")\n"
         << "  While:    (while cond body...)\n"
         << "  For:      (for var init cond step body...)\n"
         << "  Strings:  \"hello\\n\"   -> i8*\n"
         << "  Chars:    #\\a  #\\newline  #\\space\n"
         << "  Bools:    #t  #f\n\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) { help(argv[0]); return 1; }
    string input, output;
    int  emit    = 0;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--help"    || a == "-h") { help(argv[0]); return 0; }
        if (a == "--version" || a == "-v") { cout << "lisp " << VERSION << " - Tiny Lisp\n"; return 0; }
        if (a == "-o" && i + 1 < argc)    { output = argv[++i]; continue; }
        if (a == "--emit-ir")  { emit = 1; continue; }
        if (a == "--emit-asm") { emit = 2; continue; }
        if (a == "--emit-obj") { emit = 3; continue; }
        if (a == "--verbose")  { verbose = true; continue; }
        if (a[0] != '-')       { input = a; continue; }
        cerr << "error: unknown option: " << a << "\n"; return 1;
    }
    if (input.empty()) { cerr << "error: no input file\n"; return 1; }

    ifstream ifs(input);
    if (!ifs) { cerr << "error: cannot open " << input << "\n"; return 1; }
    string src((istreambuf_iterator<char>(ifs)), {});
    ifs.close();

    string llvm = Compiler().compile(src);

    string base = fs::path(input).stem().string();
    if (output.empty()) output = base;
    string ll = base + ".ll", as = base + ".s", ob = base + ".o";

    ofstream(ll) << llvm;
    if (emit == 1) return 0;

    string llc = "llc -O2 " + ll + (emit == 3 ? " --filetype=obj -o " + ob : " -o " + as);
    if (system(llc.c_str())) { cerr << "error: llc failed\n"; return 1; }

    if (emit == 2 || emit == 3) { if (!verbose) fs::remove(ll); return 0; }

#ifdef _WIN32
    string exe  = output + ".exe";
    string link = "gcc "   + as + " -o " + exe;
    string alt  = "clang " + as + " -o " + exe;
#else
    string exe  = output;
    string link = "clang " + as + " -o " + exe;
    string alt  = "gcc "   + as + " -o " + exe;
#endif
    if (system(link.c_str()) && system(alt.c_str()))
        { cerr << "error: linking failed\n"; return 1; }

    if (!verbose) fs::remove(ll), fs::remove(as);
    return 0;
}
