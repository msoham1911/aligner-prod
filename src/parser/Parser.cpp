#include "parser/Parser.h"
#include <cassert>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {
    // Ensure there's always an Eof sentinel so bounds checks are cheap.
    if (tokens_.empty() || tokens_.back().kind != TokenKind::Eof)
        tokens_.push_back(Token{TokenKind::Eof, "", 0, 0});
}

// ─────────────────────────────────────────────────────────────────────────────
// Token navigation
// ─────────────────────────────────────────────────────────────────────────────

const Token& Parser::current() const noexcept {
    return tokens_[pos_ < tokens_.size() ? pos_ : tokens_.size() - 1];
}

const Token& Parser::peek(int offset) const noexcept {
    std::size_t idx = pos_ + static_cast<std::size_t>(offset);
    return tokens_[idx < tokens_.size() ? idx : tokens_.size() - 1];
}

bool Parser::check(TokenKind k) const noexcept {
    return current().kind == k;
}

bool Parser::match(TokenKind k) {
    if (check(k)) { advance(); return true; }
    return false;
}

Token Parser::advance() {
    Token t = current();
    if (pos_ < tokens_.size() - 1) ++pos_;
    return t;
}

Token Parser::expect(TokenKind k) {
    if (!check(k)) {
        throw ParseError(
            std::string("expected ") + tokenKindName(k), current());
    }
    return advance();
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

bool Parser::isTypeName() const noexcept {
    TokenKind k = current().kind;
    return k == TokenKind::KwInt    ||
           k == TokenKind::KwVoid   ||
           k == TokenKind::KwString;
}

int Parser::binaryPrecedence(TokenKind k) const noexcept {
    switch (k) {
        case TokenKind::Plus:  return 1;
        case TokenKind::Minus: return 1;
        case TokenKind::Star:  return 2;
        case TokenKind::Slash: return 2;
        default:               return -1;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Top-level entry point
// ─────────────────────────────────────────────────────────────────────────────

ClassDecl Parser::parseCompilationUnit() {
    ClassDecl cls = parseClass();
    if (!check(TokenKind::Eof))
        throw ParseError("unexpected tokens after class declaration", current());
    return cls;
}

// ─────────────────────────────────────────────────────────────────────────────
// Class declaration
// ─────────────────────────────────────────────────────────────────────────────

ClassDecl Parser::parseClass() {
    int ln = current().line;
    expect(TokenKind::KwClass);
    Token name = expect(TokenKind::Identifier);
    expect(TokenKind::LBrace);

    ClassDecl cls;
    cls.name = name.text;
    cls.line = ln;

    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        cls.methods.push_back(parseMethod());
    }

    expect(TokenKind::RBrace);
    return cls;
}

// ─────────────────────────────────────────────────────────────────────────────
// Method declaration
// ─────────────────────────────────────────────────────────────────────────────

MethodDecl Parser::parseMethod() {
    MethodDecl m;
    m.line = current().line;
    m.col  = current().col;

    // Collect any modifiers that precede the return type
    while (current().kind == TokenKind::KwPublic ||
           current().kind == TokenKind::KwStatic) {
        m.modifiers.push_back(current().text);
        advance();
    }

    m.returnType = parseType();

    Token nameTok = expect(TokenKind::Identifier);
    m.name = nameTok.text;

    expect(TokenKind::LParen);
    m.params = parseParams();
    expect(TokenKind::RParen);

    m.body = parseBlock();
    return m;
}

// ─────────────────────────────────────────────────────────────────────────────
// Type parsing
// ─────────────────────────────────────────────────────────────────────────────

std::string Parser::parseType() {
    if (check(TokenKind::KwInt)) {
        advance();
        return "int";
    }
    if (check(TokenKind::KwVoid)) {
        advance();
        return "void";
    }
    if (check(TokenKind::KwString)) {
        advance();
        if (check(TokenKind::LBracket)) {
            advance();                           // consume '['
            expect(TokenKind::RBracket);         // consume ']'
            return "String[]";
        }
        return "String";
    }
    throw ParseError("expected a type name ('int', 'void', or 'String')", current());
}

// ─────────────────────────────────────────────────────────────────────────────
// Parameter list
// ─────────────────────────────────────────────────────────────────────────────

std::vector<Param> Parser::parseParams() {
    std::vector<Param> params;
    if (check(TokenKind::RParen)) return params; // empty list

    do {
        Param p;
        p.type = parseType();
        p.name = expect(TokenKind::Identifier).text;
        params.push_back(std::move(p));
    } while (match(TokenKind::Comma));

    return params;
}

// ─────────────────────────────────────────────────────────────────────────────
// Block
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::unique_ptr<Stmt>> Parser::parseBlock() {
    expect(TokenKind::LBrace);
    std::vector<std::unique_ptr<Stmt>> stmts;

    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        stmts.push_back(parseStmt());
    }

    expect(TokenKind::RBrace);
    return stmts;
}

// ─────────────────────────────────────────────────────────────────────────────
// Statements
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<Stmt> Parser::parseStmt() {
    if (check(TokenKind::KwReturn))
        return parseReturnStmt();

    // Local variable declaration: starts with a type keyword
    if (isTypeName())
        return parseVarDecl(parseType());

    // Anything else is an expression statement
    return parseExprStmt();
}

std::unique_ptr<Stmt> Parser::parseVarDecl(const std::string& type) {
    int         ln   = current().line;
    int         cl   = current().col;
    std::string name = expect(TokenKind::Identifier).text;

    expect(TokenKind::Assign);
    auto init = parseExpr();
    expect(TokenKind::Semicolon);

    auto stmt  = std::make_unique<VarDeclStmt>(type, name, std::move(init));
    stmt->line = ln;
    stmt->col  = cl;
    return stmt;
}

std::unique_ptr<Stmt> Parser::parseExprStmt() {
    int  ln   = current().line;
    int  cl   = current().col;
    auto expr = parseExpr();
    expect(TokenKind::Semicolon);
    auto stmt  = std::make_unique<ExprStmt>(std::move(expr));
    stmt->line = ln;
    stmt->col  = cl;
    return stmt;
}

std::unique_ptr<Stmt> Parser::parseReturnStmt() {
    int ln = current().line;
    int cl = current().col;
    expect(TokenKind::KwReturn);

    std::unique_ptr<Expr> val;
    if (!check(TokenKind::Semicolon))
        val = parseExpr();

    expect(TokenKind::Semicolon);
    auto stmt  = std::make_unique<ReturnStmt>(std::move(val));
    stmt->line = ln;
    stmt->col  = cl;
    return stmt;
}

// ─────────────────────────────────────────────────────────────────────────────
// Expressions — Pratt precedence climbing
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<Expr> Parser::parseExpr() {
    return parseBinary(0);
}

std::unique_ptr<Expr> Parser::parseBinary(int minPrec) {
    auto left = parsePostfix();

    while (true) {
        int prec = binaryPrecedence(current().kind);
        if (prec < minPrec) break;

        char op = current().text.empty() ? '?' : current().text[0];
        advance(); // consume operator

        // Left-associative: right side uses prec+1
        auto right = parseBinary(prec + 1);
        auto node  = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        left = std::move(node);
    }

    return left;
}

/// parsePostfix handles chained `.field` and `.method(args)`.
/// Example: System . out . println ( x + y )
std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();

    while (check(TokenKind::Dot)) {
        int ln = current().line;
        int cl = current().col;
        advance(); // consume '.'

        std::string name = expect(TokenKind::Identifier).text;

        if (check(TokenKind::LParen)) {
            advance(); // consume '('
            auto args = parseArgList();
            expect(TokenKind::RParen);
            auto call   = std::make_unique<MethodCallExpr>(
                              std::move(expr), name, std::move(args));
            call->line  = ln;
            call->col   = cl;
            expr = std::move(call);
        } else {
            auto fa   = std::make_unique<FieldAccessExpr>(std::move(expr), name);
            fa->line  = ln;
            fa->col   = cl;
            expr = std::move(fa);
        }
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    // Integer literal
    if (check(TokenKind::IntLiteral)) {
        Token tok = advance();
        int val = std::stoi(tok.text);
        auto node  = std::make_unique<IntLiteralExpr>(val);
        node->line = tok.line;
        node->col  = tok.col;
        return node;
    }

    // Identifier (or unqualified method call)
    if (check(TokenKind::Identifier)) {
        Token tok  = advance();
        if (check(TokenKind::LParen)) {
            // Unqualified call: method(args)
            advance(); // consume '('
            auto args = parseArgList();
            expect(TokenKind::RParen);
            auto call   = std::make_unique<MethodCallExpr>(
                              nullptr, tok.text, std::move(args));
            call->line  = tok.line;
            call->col   = tok.col;
            return call;
        }
        auto id   = std::make_unique<IdentExpr>(tok.text);
        id->line  = tok.line;
        id->col   = tok.col;
        return id;
    }

    // Parenthesised expression
    if (check(TokenKind::LParen)) {
        advance();
        auto expr = parseExpr();
        expect(TokenKind::RParen);
        return expr;
    }

    throw ParseError("expected an expression", current());
}

std::vector<std::unique_ptr<Expr>> Parser::parseArgList() {
    std::vector<std::unique_ptr<Expr>> args;
    if (check(TokenKind::RParen)) return args; // empty

    args.push_back(parseExpr());
    while (match(TokenKind::Comma))
        args.push_back(parseExpr());

    return args;
}
