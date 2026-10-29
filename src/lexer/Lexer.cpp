#include "Lexer.h"
#include "parser/Diagnostic.h"  // LexError
#include <cctype>
#include <unordered_map>
#include <stdexcept>

// ── Keyword table ─────────────────────────────────────────────────────────────

static const std::unordered_map<std::string, TokenKind> KEYWORDS = {
    {"class",  TokenKind::KwClass},
    {"public", TokenKind::KwPublic},
    {"static", TokenKind::KwStatic},
    {"void",   TokenKind::KwVoid},
    {"int",    TokenKind::KwInt},
    {"String", TokenKind::KwString},
    {"return", TokenKind::KwReturn},
    {"new",    TokenKind::KwNew},
};

// ── Constructor ───────────────────────────────────────────────────────────────

Lexer::Lexer(std::string source) : src_(std::move(source)) {}

// ── Position primitives ───────────────────────────────────────────────────────

bool Lexer::isAtEnd() const noexcept {
    return pos_ >= src_.size();
}

char Lexer::peek() const noexcept {
    if (isAtEnd()) return '\0';
    return src_[pos_];
}

char Lexer::peekNext() const noexcept {
    if (pos_ + 1 >= src_.size()) return '\0';
    return src_[pos_ + 1];
}

char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') { ++line_; col_ = 1; }
    else            { ++col_; }
    return c;
}

// ── Whitespace / comment skipping ─────────────────────────────────────────────

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peekNext() == '/') {
            // Line comment — consume until newline
            while (!isAtEnd() && peek() != '\n') advance();
        } else if (c == '/' && peekNext() == '*') {
            // Block comment — consume until */
            advance(); advance(); // consume /*
            while (!isAtEnd()) {
                if (peek() == '*' && peekNext() == '/') {
                    advance(); advance(); // consume */
                    break;
                }
                advance();
            }
        } else {
            break;
        }
    }
}

// ── Readers ───────────────────────────────────────────────────────────────────

Token Lexer::readIdentifierOrKeyword() {
    int    startLine = line_, startCol = col_;
    size_t start     = pos_;

    while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_'))
        advance();

    std::string text = src_.substr(start, pos_ - start);
    auto        it   = KEYWORDS.find(text);
    TokenKind   kind = (it != KEYWORDS.end()) ? it->second : TokenKind::Identifier;
    return Token{kind, text, startLine, startCol};
}

Token Lexer::readIntLiteral() {
    int    startLine = line_, startCol = col_;
    size_t start     = pos_;

    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek())))
        advance();

    return Token{TokenKind::IntLiteral, src_.substr(start, pos_ - start), startLine, startCol};
}

Token Lexer::readStringLiteral() {
    int startLine = line_, startCol = col_;
    advance(); // consume opening "

    size_t start = pos_;
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\\') advance(); // skip escape character
        if (!isAtEnd()) advance();
    }
    std::string text = src_.substr(start, pos_ - start);

    if (isAtEnd())
        throw LexError("unterminated string literal", startLine, startCol);

    advance(); // consume closing "
    return Token{TokenKind::StringLiteral, text, startLine, startCol};
}

// ── Main tokenise loop ────────────────────────────────────────────────────────

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespaceAndComments();

        if (isAtEnd()) {
            tokens.push_back(Token{TokenKind::Eof, "", line_, col_});
            break;
        }

        int  startLine = line_;
        int  startCol  = col_;
        char c         = peek();

        // Identifiers / keywords
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
            continue;
        }

        // Integer literals
        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(readIntLiteral());
            continue;
        }

        // String literals
        if (c == '"') {
            tokens.push_back(readStringLiteral());
            continue;
        }

        // Single-character tokens
        advance();
        switch (c) {
            case '+': tokens.push_back({TokenKind::Plus,      "+", startLine, startCol}); break;
            case '-': tokens.push_back({TokenKind::Minus,     "-", startLine, startCol}); break;
            case '*': tokens.push_back({TokenKind::Star,      "*", startLine, startCol}); break;
            case '/': tokens.push_back({TokenKind::Slash,     "/", startLine, startCol}); break;
            case '=': tokens.push_back({TokenKind::Assign,    "=", startLine, startCol}); break;
            case '(': tokens.push_back({TokenKind::LParen,    "(", startLine, startCol}); break;
            case ')': tokens.push_back({TokenKind::RParen,    ")", startLine, startCol}); break;
            case '{': tokens.push_back({TokenKind::LBrace,    "{", startLine, startCol}); break;
            case '}': tokens.push_back({TokenKind::RBrace,    "}", startLine, startCol}); break;
            case '[': tokens.push_back({TokenKind::LBracket,  "[", startLine, startCol}); break;
            case ']': tokens.push_back({TokenKind::RBracket,  "]", startLine, startCol}); break;
            case ';': tokens.push_back({TokenKind::Semicolon, ";", startLine, startCol}); break;
            case ',': tokens.push_back({TokenKind::Comma,     ",", startLine, startCol}); break;
            case '.': tokens.push_back({TokenKind::Dot,       ".", startLine, startCol}); break;
            default:
                throw LexError(
                    std::string("unexpected character '") + c + "'",
                    startLine, startCol);
        }
    }

    return tokens;
}
