// End-to-end integration tests.
//
// These tests compile a Java source string all the way to a .class file,
// then (if a JDK is available on PATH) invoke `java` and check stdout.
//
// When Java is NOT installed the tests are automatically SKIPPED — they
// will not fail, so the CI/CD pipeline stays green.  Once a JDK is
// installed and `java` is on PATH, re-run the tests and they will execute.

#include <gtest/gtest.h>
#include "compiler.h"
#include "parser/Diagnostic.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Utility: check if `java` is on PATH
// ─────────────────────────────────────────────────────────────────────────────

static bool javaAvailable() {
#ifdef _WIN32
    return std::system("java -version >nul 2>&1") == 0;
#else
    return std::system("java -version >/dev/null 2>&1") == 0;
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Utility: run a command and capture stdout + stderr
// ─────────────────────────────────────────────────────────────────────────────

static std::string runCommand(const std::string& cmd) {
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) throw std::runtime_error("popen failed for: " + cmd);

    std::string result;
    std::array<char, 256> buf{};
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        result += buf.data();

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Utility: compile source, write Main.class to tempDir, return tempDir path
// ─────────────────────────────────────────────────────────────────────────────

static fs::path compileToDir(const std::string& source) {
    auto bytes = compileSource(source);

    // Create a unique temp directory
    fs::path tempDir = fs::temp_directory_path() /
                       ("aligner_e2e_" + std::to_string(
                           static_cast<unsigned long long>(
                               std::hash<std::string>{}(source) & 0xFFFFFFFF)));
    fs::create_directories(tempDir);

    fs::path classFile = tempDir / "Main.class";
    std::ofstream out(classFile, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot write " + classFile.string());
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    out.close();

    return tempDir;
}

// ─────────────────────────────────────────────────────────────────────────────
// Utility: run `java -cp <dir> Main` and return trimmed stdout
// ─────────────────────────────────────────────────────────────────────────────

static std::string runMain(const fs::path& classDir) {
    // Use forward slashes for the classpath (works on Windows too)
    std::string cp = classDir.string();
    for (auto& c : cp) if (c == '\\') c = '/';

    std::string cmd = "java -cp \"" + cp + "\" Main 2>&1";
    std::string out = runCommand(cmd);

    // Trim trailing newline / CRLF
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
        out.pop_back();
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(E2E, SampleProgramPrints30) {
    if (!javaAvailable())
        GTEST_SKIP() << "java not found in PATH — install a JDK to run this test";

    const char* src = R"(
class Main {
    public static void main(String[] args) {
        int x = 10;
        int y = 20;
        System.out.println(x + y);
    }
})";

    fs::path dir;
    ASSERT_NO_THROW(dir = compileToDir(src)) << "Compilation failed";

    std::string output = runMain(dir);
    EXPECT_EQ(output, "30") << "Expected '30', got: '" << output << "'";
}

TEST(E2E, CompilerProducesNonEmptyClassFile) {
    const char* src = R"(
class Main {
    public static void main(String[] args) {
        int x = 10;
        int y = 20;
        System.out.println(x + y);
    }
})";

    std::vector<uint8_t> bytes;
    ASSERT_NO_THROW(bytes = compileSource(src));
    EXPECT_GT(bytes.size(), 10u);

    // Magic bytes: 0xCAFEBABE
    ASSERT_GE(bytes.size(), 4u);
    EXPECT_EQ(bytes[0], 0xCA);
    EXPECT_EQ(bytes[1], 0xFE);
    EXPECT_EQ(bytes[2], 0xBA);
    EXPECT_EQ(bytes[3], 0xBE);
}

TEST(E2E, ClassFileVersionIsJava8) {
    const char* src = R"(
class Main {
    public static void main(String[] args) {
        int x = 1;
        System.out.println(x);
    }
})";

    std::vector<uint8_t> bytes;
    ASSERT_NO_THROW(bytes = compileSource(src));
    ASSERT_GE(bytes.size(), 8u);

    // Bytes 4-5: minor version (should be 0)
    uint16_t minor = (static_cast<uint16_t>(bytes[4]) << 8) | bytes[5];
    // Bytes 6-7: major version (should be 52 for Java 8)
    uint16_t major = (static_cast<uint16_t>(bytes[6]) << 8) | bytes[7];

    EXPECT_EQ(minor, 0u)  << "minor_version should be 0";
    EXPECT_EQ(major, 52u) << "major_version should be 52 (Java 8)";
}

TEST(E2E, AdditionVariants) {
    if (!javaAvailable())
        GTEST_SKIP() << "java not found in PATH";

    const char* src = R"(
class Main {
    public static void main(String[] args) {
        int a = 100;
        int b = 200;
        System.out.println(a + b);
    }
})";

    fs::path dir;
    ASSERT_NO_THROW(dir = compileToDir(src));
    EXPECT_EQ(runMain(dir), "300");
}

TEST(E2E, InvalidSourceProducesError) {
    EXPECT_THROW(compileSource("this is not java at all @#$"), std::exception);
}

TEST(E2E, MissingMainProducesSemanticError) {
    EXPECT_THROW(compileSource("class Foo {}"), SemanticError);
}
