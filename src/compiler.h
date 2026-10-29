#pragma once
// Convenience header: runs the full compilation pipeline and returns raw
// class-file bytes.  Both the CLI driver and integration tests use this.

#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "sema/Sema.h"
#include "codegen/CodeGen.h"
#include <cstdint>
#include <string>
#include <vector>

/// Compile Java source text to raw JVM class-file bytes.
/// Throws LexError, ParseError, SemanticError, or CodeGenError on failure.
inline std::vector<uint8_t> compileSource(const std::string& source) {
    Lexer  lexer(source);
    auto   tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    auto   cls = parser.parseCompilationUnit();

    Sema   sema;
    sema.analyse(cls);

    CodeGen gen;
    return gen.generate(cls);
}
