# Aligner — Java→JVM Compiler (C++17)

A from-scratch Java compiler written in C++17 that emits valid JVM `.class` files.

## Milestone 1 — Status

| Feature | Status |
|---|---|
| Lexer | ✅ |
| Parser (recursive-descent) | ✅ |
| Semantic analysis | ✅ |
| JVM bytecode generation | ✅ |
| Class-file serialisation (Java 8, v52) | ✅ |
| `int` variables | ✅ |
| `+` `−` `*` `/` arithmetic | ✅ |
| `System.out.println(int)` | ✅ |
| Automated unit tests (GoogleTest) | ✅ |
| End-to-end execution (requires JDK) | ⏳ install JDK |

## Supported Language Subset

```
class Main {
    public static void main(String[] args) {
        int x = 10;
        int y = 20;
        System.out.println(x + y);   // prints 30
    }
}
```

**Supported**: `int` variables, `+−*/` arithmetic, `System.out.println(int)`, `return`.  
**Not yet supported**: loops, conditionals, objects, generics, arrays, multiple classes.

## Prerequisites

| Tool | Required | Notes |
|---|---|---|
| CMake ≥ 3.20 | ✅ mandatory | |
| GCC ≥ 11 or Clang ≥ 14 (C++17) | ✅ mandatory | |
| Ninja or Make | ✅ mandatory | |
| JDK ≥ 8 | ⚠️ for e2e tests | Install [Adoptium Temurin](https://adoptium.net/) |
| Internet (first build only) | ℹ️ for GoogleTest | FetchContent downloads GTest automatically |

## Build

```powershell
# From the repository root
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

For a release build:
```powershell
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run the compiler

```powershell
# Compile a Java source file
.\build\aligner.exe samples\Main.java

# Specify output path explicitly
.\build\aligner.exe samples\Main.java -o out\Main.class
```

## Run tests

```powershell
ctest --test-dir build --output-on-failure
```

Or run the test binary directly for verbose output:
```powershell
.\build\tests\aligner_tests.exe --gtest_color=yes
```

Run a single test suite:
```powershell
.\build\tests\aligner_tests.exe --gtest_filter="Lexer.*"
.\build\tests\aligner_tests.exe --gtest_filter="Parser.*"
.\build\tests\aligner_tests.exe --gtest_filter="Sema.*"
.\build\tests\aligner_tests.exe --gtest_filter="E2E.*"
```

## Inspect generated class file (requires JDK)

```powershell
# After compiling Main.java
javap -verbose Main.class
```

## Execute generated class file (requires JDK)

```powershell
# After compiling Main.java — output should be: 30
java Main
```

## Architecture

```
Source text
    │
    ▼  src/lexer/Lexer.cpp
  Token stream
    │
    ▼  src/parser/Parser.cpp
  ClassDecl AST  (src/ast/Ast.h)
    │
    ▼  src/sema/Sema.cpp
  Validated AST
    │
    ▼  src/codegen/CodeGen.cpp
  ConstantPool + BytecodeBuilder
    │
    ▼  src/classfile/ClassFile.cpp
  std::vector<uint8_t>  →  Main.class
```

### Key files

| File | Purpose |
|---|---|
| `src/lexer/Token.h` | Token kind enum + Token struct |
| `src/lexer/Lexer.cpp` | Source → token stream |
| `src/ast/Ast.h` | All AST node types |
| `src/parser/Diagnostic.h` | Exception types (LexError, ParseError, SemanticError, CodeGenError) |
| `src/parser/Parser.cpp` | Recursive-descent parser |
| `src/sema/Sema.cpp` | Name resolution + type checking |
| `src/classfile/ClassFile.cpp` | ConstantPool builder + binary serialiser |
| `src/codegen/CodeGen.cpp` | AST → JVM bytecode |
| `src/compiler.h` | One-call pipeline: `compileSource(source)` |
| `src/main.cpp` | CLI driver |

## JVM class-file target

- Format version **52.0** (Java 8) — maximum compatibility
- All integer constants: `iconst_*` / `bipush` / `sipush`
- `System.out.println(int)`: `getstatic` + `invokevirtual`
- Local variables: `iload_*` / `istore_*`
- Arithmetic: `iadd` `isub` `imul` `idiv`

## Known limitations (Milestone 1)

- Only one class per file, with one method (`main`).
- No `if`, `while`, `for` statements.
- No array creation or access.
- No user-defined methods or constructors.
- No `String`, `boolean`, `long`, `double`, or reference types except in signatures.
- Integer constants larger than 32767 are not yet supported (need `ldc`).
- No optimisation passes.

## Next milestone (Milestone 2)

- `if`/`else` with integer comparisons (`ifeq`, `ifne`, `iflt`, `ifge`, `ifgt`, `ifle`)
- `while` loops
- `return` with int value
- Multi-method classes (static int methods calling each other)
