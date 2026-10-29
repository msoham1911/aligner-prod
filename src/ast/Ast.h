#pragma once
#include <memory>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Base nodes
// ─────────────────────────────────────────────────────────────────────────────

struct Expr {
    virtual ~Expr() = default;
    int line{0};
    int col{0};
};

struct Stmt {
    virtual ~Stmt() = default;
    int line{0};
    int col{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Expressions
// ─────────────────────────────────────────────────────────────────────────────

/// Integer literal: 42
struct IntLiteralExpr : Expr {
    int value{0};
    explicit IntLiteralExpr(int v) : value(v) {}
};

/// Identifier reference: x
struct IdentExpr : Expr {
    std::string name;
    explicit IdentExpr(std::string n) : name(std::move(n)) {}
};

/// Binary arithmetic: a + b, a - b, a * b, a / b
struct BinaryExpr : Expr {
    char                    op{'+'}; // '+', '-', '*', '/'
    std::unique_ptr<Expr>   left;
    std::unique_ptr<Expr>   right;

    BinaryExpr(char op_, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : op(op_), left(std::move(l)), right(std::move(r)) {}
};

/// Field access: obj.field
struct FieldAccessExpr : Expr {
    std::unique_ptr<Expr> object;
    std::string           field;

    FieldAccessExpr(std::unique_ptr<Expr> obj, std::string f)
        : object(std::move(obj)), field(std::move(f)) {}
};

/// Method call: obj.method(args) or method(args)
struct MethodCallExpr : Expr {
    std::unique_ptr<Expr>              object; // null for unqualified calls
    std::string                        method;
    std::vector<std::unique_ptr<Expr>> args;

    MethodCallExpr(std::unique_ptr<Expr>              obj,
                   std::string                        m,
                   std::vector<std::unique_ptr<Expr>> a)
        : object(std::move(obj)), method(std::move(m)), args(std::move(a)) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Statements
// ─────────────────────────────────────────────────────────────────────────────

/// Expression used as a statement: System.out.println(x);
struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;
    explicit ExprStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
};

/// Local variable declaration with mandatory initialiser: int x = 10;
struct VarDeclStmt : Stmt {
    std::string           type; // "int", "String", "String[]"
    std::string           name;
    std::unique_ptr<Expr> init;

    VarDeclStmt(std::string t, std::string n, std::unique_ptr<Expr> i)
        : type(std::move(t)), name(std::move(n)), init(std::move(i)) {}
};

/// Return statement: return expr; or return;
struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value; // null for void return

    explicit ReturnStmt(std::unique_ptr<Expr> v) : value(std::move(v)) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Declarations
// ─────────────────────────────────────────────────────────────────────────────

struct Param {
    std::string type; // "int", "String[]", …
    std::string name;
};

struct MethodDecl {
    std::vector<std::string>           modifiers; // "public", "static"
    std::string                        returnType;
    std::string                        name;
    std::vector<Param>                 params;
    std::vector<std::unique_ptr<Stmt>> body;
    int                                line{0};
    int                                col{0};
};

struct ClassDecl {
    std::string             name;
    std::vector<MethodDecl> methods;
    int                     line{0};
};
