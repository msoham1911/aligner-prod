#pragma once
#include <stdexcept>
#include <string>
#include "lexer/Token.h"

// ── Lex-phase error ───────────────────────────────────────────────────────────

struct LexError : std::runtime_error {
    int line, col;

    LexError(const std::string& msg, int ln, int cl)
        : std::runtime_error(
              std::string("lex error at ") + std::to_string(ln) + ":" +
              std::to_string(cl) + ": " + msg)
        , line(ln), col(cl) {}
};

// ── Parse-phase error ─────────────────────────────────────────────────────────

struct ParseError : std::runtime_error {
    int line, col;

    /// Construct from an unexpected token.
    ParseError(const std::string& msg, const Token& tok)
        : std::runtime_error(
              std::string("parse error at ") + std::to_string(tok.line) +
              ":" + std::to_string(tok.col) + ": " + msg +
              " (got " + tokenKindName(tok.kind) +
              (tok.text.empty() ? "" : " '" + tok.text + "'") + ")")
        , line(tok.line), col(tok.col) {}

    /// Construct with explicit location (no token).
    ParseError(const std::string& msg, int ln, int cl)
        : std::runtime_error(
              std::string("parse error at ") + std::to_string(ln) +
              ":" + std::to_string(cl) + ": " + msg)
        , line(ln), col(cl) {}
};

// ── Semantic-analysis error ───────────────────────────────────────────────────

struct SemanticError : std::runtime_error {
    int line, col;

    SemanticError(const std::string& msg, int ln = 0, int cl = 0)
        : std::runtime_error(
              std::string("semantic error at ") + std::to_string(ln) +
              ":" + std::to_string(cl) + ": " + msg)
        , line(ln), col(cl) {}
};

// ── Code-generation error ─────────────────────────────────────────────────────

struct CodeGenError : std::runtime_error {
    explicit CodeGenError(const std::string& msg)
        : std::runtime_error("codegen error: " + msg) {}
};
