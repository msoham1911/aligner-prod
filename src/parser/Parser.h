#pragma once
#include "ast/Ast.h"
#include "lexer/Token.h"
#include "parser/Diagnostic.h"
#include <vector>
#include <memory>

/// Recursive-descent parser.
/// Input  : token stream produced by Lexer (must end with Eof).
/// Output : ClassDecl — the top-level AST node.
/// Throws : ParseError on malformed input.
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    /// Parse the whole compilation unit and return the class declaration.
    ClassDecl parseCompilationUnit();

private:
    std::vector<Token> tokens_;
    std::size_t        pos_{0};

    // ── Token navigation ──────────────────────────────────────────────────────
    const Token& current()  const noexcept;
    const Token& peek(int offset = 1) const noexcept;
    bool  check(TokenKind k) const noexcept;
    bool  match(TokenKind k);
    Token expect(TokenKind k);        // consumes & returns; throws on mismatch
    Token advance();

    // ── Grammar productions ───────────────────────────────────────────────────
    ClassDecl               parseClass();
    MethodDecl              parseMethod();
    std::string             parseType();
    std::vector<Param>      parseParams();
    std::vector<std::unique_ptr<Stmt>> parseBlock();

    std::unique_ptr<Stmt> parseStmt();
    std::unique_ptr<Stmt> parseVarDecl(const std::string& type);
    std::unique_ptr<Stmt> parseExprStmt();
    std::unique_ptr<Stmt> parseReturnStmt();

    // ── Expressions (Pratt / precedence climbing) ─────────────────────────────
    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<Expr> parseBinary(int minPrec);
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parsePrimary();
    std::vector<std::unique_ptr<Expr>> parseArgList();

    // ── Helpers ───────────────────────────────────────────────────────────────
    bool isTypeName() const noexcept;  // true if current token starts a type
    int  binaryPrecedence(TokenKind k) const noexcept;
};
