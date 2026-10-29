#pragma once
#include "ast/Ast.h"
#include "classfile/ClassFile.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

/// Walks the annotated AST and emits a valid JVM class file.
///
/// Milestone-1 scope:
///   - Single public class, single static void main(String[]) method.
///   - int local variables, bipush/sipush integer constants.
///   - +  −  *  /  arithmetic on ints.
///   - System.out.println(int) via getstatic + invokevirtual.
///
/// Throws CodeGenError on unsupported constructs.
class CodeGen {
public:
    CodeGen() = default;

    /// Generate and return raw class-file bytes for cls.
    std::vector<uint8_t> generate(const ClassDecl& cls);

private:
    ConstantPool cp_;

    // Per-method state (reset for each method)
    std::map<std::string, int> locals_;  // name → slot index
    int nextSlot_{0};

    void genMethod(const MethodDecl& m, MethodInfo& out);
    void genStmt  (const Stmt&       s, BytecodeBuilder& bb);
    void genExpr  (const Expr&       e, BytecodeBuilder& bb);
    void genMethodCall(const MethodCallExpr& mc, BytecodeBuilder& bb);

    /// Map Java source modifier names to JVM access flag bits.
    static uint16_t accessFlags(const std::vector<std::string>& modifiers);

    /// Build the JVM method descriptor string for a MethodDecl.
    static std::string methodDescriptor(const MethodDecl& m);

    /// Map a Java type name to its JVM descriptor character/string.
    static std::string typeDescriptor(const std::string& javaType);
};
