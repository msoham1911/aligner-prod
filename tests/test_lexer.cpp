#include <gtest/gtest.h>
#include "lexer/Lexer.h"
#include "parser/Diagnostic.h"

// ── Helper ────────────────────────────────────────────────────────────────────

static std::vector<Token> lex(const std::string& src) {
    return Lexer(src).tokenize();
}

// ── Individual token types ────────────────────────────────────────────────────

TEST(Lexer, EmptySourceProducesEof) {
    auto toks = lex("");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::Eof);
}

TEST(Lexer, IntLiteral) {
    auto toks = lex("42");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].kind, TokenKind::IntLiteral);
    EXPECT_EQ(toks[0].text, "42");
    EXPECT_EQ(toks[0].line, 1);
}

TEST(Lexer, IntLiteralZero) {
    auto toks = lex("0");
    EXPECT_EQ(toks[0].kind, TokenKind::IntLiteral);
    EXPECT_EQ(toks[0].text, "0");
}

TEST(Lexer, Keywords) {
    auto toks = lex("class public static void int String return new");
    EXPECT_EQ(toks[0].kind, TokenKind::KwClass);
    EXPECT_EQ(toks[1].kind, TokenKind::KwPublic);
    EXPECT_EQ(toks[2].kind, TokenKind::KwStatic);
    EXPECT_EQ(toks[3].kind, TokenKind::KwVoid);
    EXPECT_EQ(toks[4].kind, TokenKind::KwInt);
    EXPECT_EQ(toks[5].kind, TokenKind::KwString);
    EXPECT_EQ(toks[6].kind, TokenKind::KwReturn);
    EXPECT_EQ(toks[7].kind, TokenKind::KwNew);
}

TEST(Lexer, Identifier) {
    auto toks = lex("System");
    ASSERT_GE(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::Identifier);
    EXPECT_EQ(toks[0].text, "System");
}

TEST(Lexer, Operators) {
    auto toks = lex("+ - * / =");
    EXPECT_EQ(toks[0].kind, TokenKind::Plus);
    EXPECT_EQ(toks[1].kind, TokenKind::Minus);
    EXPECT_EQ(toks[2].kind, TokenKind::Star);
    EXPECT_EQ(toks[3].kind, TokenKind::Slash);
    EXPECT_EQ(toks[4].kind, TokenKind::Assign);
}

TEST(Lexer, Punctuation) {
    auto toks = lex("( ) { } [ ] ; , .");
    EXPECT_EQ(toks[0].kind, TokenKind::LParen);
    EXPECT_EQ(toks[1].kind, TokenKind::RParen);
    EXPECT_EQ(toks[2].kind, TokenKind::LBrace);
    EXPECT_EQ(toks[3].kind, TokenKind::RBrace);
    EXPECT_EQ(toks[4].kind, TokenKind::LBracket);
    EXPECT_EQ(toks[5].kind, TokenKind::RBracket);
    EXPECT_EQ(toks[6].kind, TokenKind::Semicolon);
    EXPECT_EQ(toks[7].kind, TokenKind::Comma);
    EXPECT_EQ(toks[8].kind, TokenKind::Dot);
}

TEST(Lexer, LineComment) {
    auto toks = lex("// this is a comment\n42");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].kind, TokenKind::IntLiteral);
    EXPECT_EQ(toks[0].text, "42");
}

TEST(Lexer, BlockComment) {
    auto toks = lex("/* hello world */ 99");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].kind, TokenKind::IntLiteral);
    EXPECT_EQ(toks[0].text, "99");
}

TEST(Lexer, Whitespace) {
    auto toks = lex("  \t\n  10  \n  20  ");
    ASSERT_EQ(toks.size(), 3u);
    EXPECT_EQ(toks[0].text, "10");
    EXPECT_EQ(toks[1].text, "20");
}

TEST(Lexer, LineNumbers) {
    auto toks = lex("a\nb\nc");
    EXPECT_EQ(toks[0].line, 1);
    EXPECT_EQ(toks[1].line, 2);
    EXPECT_EQ(toks[2].line, 3);
}

// ── Full program ──────────────────────────────────────────────────────────────

TEST(Lexer, SampleProgramTokenCount) {
    const char* src = R"(
class Main {
    public static void main(String[] args) {
        int x = 10;
        int y = 20;
        System.out.println(x + y);
    }
})";
    auto toks = lex(src);

    // Last token must be Eof
    EXPECT_EQ(toks.back().kind, TokenKind::Eof);

    // Spot-check a few tokens in order
    // Find 'class'
    EXPECT_EQ(toks[0].kind, TokenKind::KwClass);
    EXPECT_EQ(toks[1].kind, TokenKind::Identifier);
    EXPECT_EQ(toks[1].text, "Main");
}

TEST(Lexer, SampleProgramContainsAllKeywords) {
    const char* src = R"(
class Main {
    public static void main(String[] args) {
        int x = 10;
    }
})";
    auto toks = lex(src);
    bool foundClass = false, foundPublic = false, foundStatic = false;
    bool foundVoid  = false, foundInt    = false, foundString = false;
    for (const auto& t : toks) {
        if (t.kind == TokenKind::KwClass)  foundClass  = true;
        if (t.kind == TokenKind::KwPublic) foundPublic = true;
        if (t.kind == TokenKind::KwStatic) foundStatic = true;
        if (t.kind == TokenKind::KwVoid)   foundVoid   = true;
        if (t.kind == TokenKind::KwInt)    foundInt    = true;
        if (t.kind == TokenKind::KwString) foundString = true;
    }
    EXPECT_TRUE(foundClass);
    EXPECT_TRUE(foundPublic);
    EXPECT_TRUE(foundStatic);
    EXPECT_TRUE(foundVoid);
    EXPECT_TRUE(foundInt);
    EXPECT_TRUE(foundString);
}

// ── Error cases ───────────────────────────────────────────────────────────────

TEST(Lexer, UnexpectedCharThrows) {
    EXPECT_THROW(lex("@"), LexError);
    EXPECT_THROW(lex("#"), LexError);
    EXPECT_THROW(lex("$"), LexError);
}

TEST(Lexer, UnterminatedStringThrows) {
    EXPECT_THROW(lex("\"hello"), LexError);
}

TEST(Lexer, ErrorMessageContainsLocation) {
    try {
        lex("valid\n@bad");
        FAIL() << "Expected LexError";
    } catch (const LexError& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("2"), std::string::npos) << "Line 2 should appear in: " << msg;
    }
}
