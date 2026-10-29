#pragma once
#include "Token.h"
#include <string>
#include <vector>

/// Converts Java source text into a flat token stream.
/// Throws LexError on illegal characters or unterminated constructs.
class Lexer {
public:
    explicit Lexer(std::string source);

    /// Tokenise the entire source and return all tokens (including Eof).
    std::vector<Token> tokenize();

private:
    std::string src_;
    std::size_t pos_{0};
    int         line_{1};
    int         col_{1};

    // ── Primitives ────────────────────────────────────────────────────────────
    char peek()     const noexcept;
    char peekNext() const noexcept;
    char advance();
    bool isAtEnd()  const noexcept;

    // ── Skip whitespace and // or /* */ comments ──────────────────────────────
    void skipWhitespaceAndComments();

    // ── Readers ───────────────────────────────────────────────────────────────
    Token readIdentifierOrKeyword();
    Token readIntLiteral();
    Token readStringLiteral();
};
