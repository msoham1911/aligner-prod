#include <gtest/gtest.h>
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "parser/Diagnostic.h"
#include "ast/Ast.h"

// ── Helper ────────────────────────────────────────────────────────────────────

static ClassDecl parse(const std::string& src) {
    Lexer  lex(src);
    auto   toks = lex.tokenize();
    Parser p(std::move(toks));
    return p.parseCompilationUnit();
}

// ── Class-level ───────────────────────────────────────────────────────────────

TEST(Parser, ParsesEmptyClass) {
    auto cls = parse("class Foo {}");
    EXPECT_EQ(cls.name, "Foo");
    EXPECT_TRUE(cls.methods.empty());
}

TEST(Parser, ParsesClassName) {
    auto cls = parse("class MyClass { public static void main(String[] a) {} }");
    EXPECT_EQ(cls.name, "MyClass");
}

// ── Method-level ──────────────────────────────────────────────────────────────

TEST(Parser, ParsesMainSignature) {
    auto cls = parse("class M { public static void main(String[] args) {} }");
    ASSERT_EQ(cls.methods.size(), 1u);
    const auto& m = cls.methods[0];
    EXPECT_EQ(m.name, "main");
    EXPECT_EQ(m.returnType, "void");
    ASSERT_EQ(m.modifiers.size(), 2u);
    EXPECT_EQ(m.modifiers[0], "public");
    EXPECT_EQ(m.modifiers[1], "static");
    ASSERT_EQ(m.params.size(), 1u);
    EXPECT_EQ(m.params[0].type, "String[]");
    EXPECT_EQ(m.params[0].name, "args");
}

TEST(Parser, ParsesEmptyMethodBody) {
    auto cls = parse("class M { static void foo() {} }");
    EXPECT_TRUE(cls.methods[0].body.empty());
}

// ── Statements ────────────────────────────────────────────────────────────────

TEST(Parser, ParsesIntVarDecl) {
    auto cls = parse("class M { static void f() { int x = 5; } }");
    ASSERT_EQ(cls.methods[0].body.size(), 1u);
    auto* v = dynamic_cast<VarDeclStmt*>(cls.methods[0].body[0].get());
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->type, "int");
    EXPECT_EQ(v->name, "x");
    auto* lit = dynamic_cast<IntLiteralExpr*>(v->init.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->value, 5);
}

TEST(Parser, ParsesExprStatement) {
    auto cls = parse("class M { static void f() { System.out.println(1); } }");
    ASSERT_EQ(cls.methods[0].body.size(), 1u);
    auto* es = dynamic_cast<ExprStmt*>(cls.methods[0].body[0].get());
    ASSERT_NE(es, nullptr);
    auto* mc = dynamic_cast<MethodCallExpr*>(es->expr.get());
    ASSERT_NE(mc, nullptr);
    EXPECT_EQ(mc->method, "println");
}

TEST(Parser, ParsesReturnVoid) {
    auto cls = parse("class M { static void f() { return; } }");
    auto* r = dynamic_cast<ReturnStmt*>(cls.methods[0].body[0].get());
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->value, nullptr);
}

// ── Expressions ───────────────────────────────────────────────────────────────

TEST(Parser, ParsesBinaryAdd) {
    auto cls = parse("class M { static void f() { int z = 1 + 2; } }");
    auto* v = dynamic_cast<VarDeclStmt*>(cls.methods[0].body[0].get());
    ASSERT_NE(v, nullptr);
    auto* bin = dynamic_cast<BinaryExpr*>(v->init.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, '+');
    auto* l = dynamic_cast<IntLiteralExpr*>(bin->left.get());
    auto* r = dynamic_cast<IntLiteralExpr*>(bin->right.get());
    ASSERT_NE(l, nullptr); EXPECT_EQ(l->value, 1);
    ASSERT_NE(r, nullptr); EXPECT_EQ(r->value, 2);
}

TEST(Parser, OperatorPrecedenceMultiplyBeforeAdd) {
    // 2 + 3 * 4  should parse as  2 + (3 * 4)
    auto cls = parse("class M { static void f() { int z = 2 + 3 * 4; } }");
    auto* v   = dynamic_cast<VarDeclStmt*>(cls.methods[0].body[0].get());
    auto* add = dynamic_cast<BinaryExpr*>(v->init.get());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op, '+');
    auto* mul = dynamic_cast<BinaryExpr*>(add->right.get());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op, '*');
}

TEST(Parser, ParsesChainedFieldAccess) {
    auto cls = parse("class M { static void f() { System.out.println(0); } }");
    auto* es = dynamic_cast<ExprStmt*>(cls.methods[0].body[0].get());
    auto* mc = dynamic_cast<MethodCallExpr*>(es->expr.get());
    ASSERT_NE(mc, nullptr);
    EXPECT_EQ(mc->method, "println");
    auto* fa = dynamic_cast<FieldAccessExpr*>(mc->object.get());
    ASSERT_NE(fa, nullptr);
    EXPECT_EQ(fa->field, "out");
    auto* sys = dynamic_cast<IdentExpr*>(fa->object.get());
    ASSERT_NE(sys, nullptr);
    EXPECT_EQ(sys->name, "System");
}

// ── Full sample program ───────────────────────────────────────────────────────

TEST(Parser, ParsesSampleProgram) {
    const char* src = R"(
class Main {
    public static void main(String[] args) {
        int x = 10;
        int y = 20;
        System.out.println(x + y);
    }
})";
    EXPECT_NO_THROW({
        auto cls = parse(src);
        EXPECT_EQ(cls.name, "Main");
        EXPECT_EQ(cls.methods.size(), 1u);
        EXPECT_EQ(cls.methods[0].body.size(), 3u);
    });
}

// ── Error cases ───────────────────────────────────────────────────────────────

TEST(Parser, MissingClassNameThrows) {
    EXPECT_THROW(parse("class { }"), ParseError);
}

TEST(Parser, MissingOpenBraceThrows) {
    EXPECT_THROW(parse("class Foo }"), ParseError);
}

TEST(Parser, MissingSemicolonThrows) {
    EXPECT_THROW(parse("class M { static void f() { int x = 5 } }"), ParseError);
}

TEST(Parser, MissingEqualSignThrows) {
    EXPECT_THROW(parse("class M { static void f() { int x 5; } }"), ParseError);
}

TEST(Parser, ErrorMessageContainsLineNumber) {
    try {
        parse("class M {\n  static void f() {\n    int x 5;\n  }\n}");
        FAIL() << "Expected ParseError";
    } catch (const ParseError& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("3"), std::string::npos) << "Line 3 should appear in: " << msg;
    }
}

TEST(Parser, TrailingTokensThrows) {
    EXPECT_THROW(parse("class M {} class N {}"), ParseError);
}
