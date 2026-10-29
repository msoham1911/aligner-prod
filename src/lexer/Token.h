#pragma once
#include <string>

// ── Token kinds ───────────────────────────────────────────────────────────────

enum class TokenKind {
    // Literals
    IntLiteral,
    StringLiteral,
    Identifier,

    // Keywords
    KwClass,
    KwPublic,
    KwStatic,
    KwVoid,
    KwInt,
    KwString,
    KwReturn,
    KwNew,

    // Operators
    Plus,
    Minus,
    Star,
    Slash,
    Assign,

    // Punctuation
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Semicolon,
    Comma,
    Dot,

    // Sentinel
    Eof,
};

// ── Token ─────────────────────────────────────────────────────────────────────

struct Token {
    TokenKind   kind{TokenKind::Eof};
    std::string text;
    int         line{1};
    int         col{1};
};

// ── Human-readable name (used in error messages) ──────────────────────────────

inline const char* tokenKindName(TokenKind k) noexcept {
    switch (k) {
        case TokenKind::IntLiteral:    return "integer literal";
        case TokenKind::StringLiteral: return "string literal";
        case TokenKind::Identifier:    return "identifier";
        case TokenKind::KwClass:       return "'class'";
        case TokenKind::KwPublic:      return "'public'";
        case TokenKind::KwStatic:      return "'static'";
        case TokenKind::KwVoid:        return "'void'";
        case TokenKind::KwInt:         return "'int'";
        case TokenKind::KwString:      return "'String'";
        case TokenKind::KwReturn:      return "'return'";
        case TokenKind::KwNew:         return "'new'";
        case TokenKind::Plus:          return "'+'";
        case TokenKind::Minus:         return "'-'";
        case TokenKind::Star:          return "'*'";
        case TokenKind::Slash:         return "'/'";
        case TokenKind::Assign:        return "'='";
        case TokenKind::LParen:        return "'('";
        case TokenKind::RParen:        return "')'";
        case TokenKind::LBrace:        return "'{'";
        case TokenKind::RBrace:        return "'}'";
        case TokenKind::LBracket:      return "'['";
        case TokenKind::RBracket:      return "']'";
        case TokenKind::Semicolon:     return "';'";
        case TokenKind::Comma:         return "','";
        case TokenKind::Dot:           return "'.'";
        case TokenKind::Eof:           return "<EOF>";
    }
    return "?";
}
