#include <gtest/gtest.h>
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "sema/Sema.h"
#include "parser/Diagnostic.h"

// ── Helpers ───────────────────────────────────────────────────────────────────

static ClassDecl parseStr(const std::string& src) {
    Lexer  lex(src);
    auto   toks = lex.tokenize();
    Parser p(std::move(toks));
    return p.parseCompilationUnit();
}

static void analyseStr(const std::string& src) {
    auto cls = parseStr(src);
    Sema sema;
    sema.analyse(cls);
}

// ── Valid programs ────────────────────────────────────────────────────────────

TEST(Sema, AcceptsSampleProgram) {
    EXPECT_NO_THROW(analyseStr(R"(
class Main {
    public static void main(String[] args) {
        int x = 10;
        int y = 20;
        System.out.println(x + y);
    }
})"));
}

TEST(Sema, AcceptsMinimalMain) {
    EXPECT_NO_THROW(analyseStr(
        "class Main { public static void main(String[] args) {} }"));
}

TEST(Sema, AcceptsSingleVarDecl) {
    EXPECT_NO_THROW(analyseStr(R"(
class Main {
    public static void main(String[] args) {
        int x = 42;
    }
})"));
}

TEST(Sema, AcceptsArithmetic) {
    EXPECT_NO_THROW(analyseStr(R"(
class Main {
    public static void main(String[] args) {
        int a = 1 + 2 * 3 - 4 / 2;
    }
})"));
}

TEST(Sema, AcceptsChainedArithmetic) {
    EXPECT_NO_THROW(analyseStr(R"(
class Main {
    public static void main(String[] args) {
        int a = 10;
        int b = 20;
        int c = a + b;
        System.out.println(c);
    }
})"));
}

// ── Missing main ─────────────────────────────────────────────────────────────

TEST(Sema, MissingMainThrows) {
    EXPECT_THROW(analyseStr(
        "class Foo { static void notMain() {} }"),
        SemanticError);
}

// ── Signature errors ──────────────────────────────────────────────────────────

TEST(Sema, MainMissingPublicThrows) {
    EXPECT_THROW(analyseStr(
        "class M { static void main(String[] args) {} }"),
        SemanticError);
}

TEST(Sema, MainMissingStaticThrows) {
    EXPECT_THROW(analyseStr(
        "class M { public void main(String[] args) {} }"),
        SemanticError);
}

TEST(Sema, MainWrongReturnTypeThrows) {
    EXPECT_THROW(analyseStr(
        "class M { public static int main(String[] args) { return 0; } }"),
        SemanticError);
}

TEST(Sema, MainWrongParamTypeThrows) {
    EXPECT_THROW(analyseStr(
        "class M { public static void main(int args) {} }"),
        SemanticError);
}

// ── Name resolution ───────────────────────────────────────────────────────────

TEST(Sema, UndeclaredVariableThrows) {
    EXPECT_THROW(analyseStr(R"(
class Main {
    public static void main(String[] args) {
        System.out.println(z);
    }
})"), SemanticError);
}

TEST(Sema, UseBeforeDeclThrows) {
    EXPECT_THROW(analyseStr(R"(
class Main {
    public static void main(String[] args) {
        int y = x + 1;
        int x = 5;
    }
})"), SemanticError);
}

TEST(Sema, DuplicateVarDeclThrows) {
    EXPECT_THROW(analyseStr(R"(
class Main {
    public static void main(String[] args) {
        int x = 1;
        int x = 2;
    }
})"), SemanticError);
}

// ── Type errors ───────────────────────────────────────────────────────────────

TEST(Sema, PrintlnWithUndeclaredVarThrows) {
    EXPECT_THROW(analyseStr(R"(
class Main {
    public static void main(String[] args) {
        System.out.println(undeclared);
    }
})"), SemanticError);
}

// ── Error messages contain location ──────────────────────────────────────────

TEST(Sema, ErrorMessageContainsInfo) {
    try {
        analyseStr(R"(
class Main {
    public static void main(String[] args) {
        System.out.println(bad);
    }
})");
        FAIL() << "Expected SemanticError";
    } catch (const SemanticError& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("bad"), std::string::npos)
            << "Variable name should appear in error: " << msg;
    }
}
