#include "compiler.h"
#include "parser/Diagnostic.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " <source.java> [-o <output.class>]\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    std::string inputPath;
    std::string outputPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg[0] != '-') {
            inputPath = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            usage(argv[0]);
            return 1;
        }
    }

    if (inputPath.empty()) {
        std::cerr << "error: no input file specified\n";
        usage(argv[0]);
        return 1;
    }

    // Default output: same directory as input, class name Main.class
    if (outputPath.empty()) {
        // Strip path and extension, append .class
        std::string base = inputPath;
        auto slash = base.find_last_of("/\\");
        if (slash != std::string::npos) base = base.substr(slash + 1);
        auto dot = base.rfind('.');
        if (dot != std::string::npos) base = base.substr(0, dot);
        outputPath = base + ".class";
    }

    // Read source file
    std::ifstream fin(inputPath, std::ios::binary);
    if (!fin) {
        std::cerr << "error: cannot open '" << inputPath << "'\n";
        return 1;
    }
    std::string source((std::istreambuf_iterator<char>(fin)),
                        std::istreambuf_iterator<char>());

    // Compile
    std::vector<uint8_t> classBytes;
    try {
        classBytes = compileSource(source);
    } catch (const LexError& e) {
        std::cerr << inputPath << ":" << e.line << ":" << e.col
                  << ": " << e.what() << "\n";
        return 1;
    } catch (const ParseError& e) {
        std::cerr << inputPath << ":" << e.line << ":" << e.col
                  << ": " << e.what() << "\n";
        return 1;
    } catch (const SemanticError& e) {
        std::cerr << inputPath << ":" << e.line << ":" << e.col
                  << ": " << e.what() << "\n";
        return 1;
    } catch (const CodeGenError& e) {
        std::cerr << "codegen: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "internal error: " << e.what() << "\n";
        return 1;
    }

    // Write class file
    std::ofstream fout(outputPath, std::ios::binary);
    if (!fout) {
        std::cerr << "error: cannot write '" << outputPath << "'\n";
        return 1;
    }
    fout.write(reinterpret_cast<const char*>(classBytes.data()),
               static_cast<std::streamsize>(classBytes.size()));

    if (!fout) {
        std::cerr << "error: write to '" << outputPath << "' failed\n";
        return 1;
    }

    std::cout << "Wrote " << classBytes.size() << " bytes to " << outputPath << "\n";
    return 0;
}
