cmake_minimum_required(VERSION 3.20)
project(aligner LANGUAGES CXX)

# ── Language standard ─────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Warnings ──────────────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── GoogleTest ────────────────────────────────────────────────────────────────
include(cmake/FetchGoogleTest.cmake)

# ── Compiler library (no main; linked by both driver and tests) ───────────────
add_library(aligner_lib STATIC
    src/lexer/Lexer.cpp
    src/parser/Parser.cpp
    src/sema/Sema.cpp
    src/classfile/ClassFile.cpp
    src/codegen/CodeGen.cpp
)
target_include_directories(aligner_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ── Driver executable ─────────────────────────────────────────────────────────
add_executable(aligner src/main.cpp)
target_link_libraries(aligner PRIVATE aligner_lib)

# ── Tests ─────────────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)
