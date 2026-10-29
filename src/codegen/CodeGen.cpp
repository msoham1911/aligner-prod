#include "codegen/CodeGen.h"
#include "parser/Diagnostic.h"
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────────────────────────────────────

uint16_t CodeGen::accessFlags(const std::vector<std::string>& mods) {
    uint16_t flags = 0;
    for (const auto& m : mods) {
        if (m == "public")    flags |= 0x0001u;
        else if (m == "private")   flags |= 0x0002u;
        else if (m == "protected") flags |= 0x0004u;
        else if (m == "static")    flags |= 0x0008u;
        else if (m == "final")     flags |= 0x0010u;
        else if (m == "abstract")  flags |= 0x0400u;
    }
    return flags;
}

std::string CodeGen::typeDescriptor(const std::string& javaType) {
    if (javaType == "int")      return "I";
    if (javaType == "void")     return "V";
    if (javaType == "String")   return "Ljava/lang/String;";
    if (javaType == "String[]") return "[Ljava/lang/String;";
    throw CodeGenError("unknown type '" + javaType + "'");
}

std::string CodeGen::methodDescriptor(const MethodDecl& m) {
    std::string desc = "(";
    for (const auto& p : m.params)
        desc += typeDescriptor(p.type);
    desc += ")";
    desc += typeDescriptor(m.returnType);
    return desc;
}

// ─────────────────────────────────────────────────────────────────────────────
// generate — top-level entry point
// ─────────────────────────────────────────────────────────────────────────────

std::vector<uint8_t> CodeGen::generate(const ClassDecl& cls) {
    cp_ = ConstantPool{}; // fresh pool

    // Pre-populate mandatory CP entries
    uint16_t superClass = cp_.addClass("java/lang/Object");
    uint16_t thisClass  = cp_.addClass(cls.name);

    // Pre-register "Code" attribute name (must be in CP before any method)
    uint16_t codeAttr = cp_.addUtf8("Code");

    // Generate all methods
    std::vector<MethodInfo> methods;
    for (const auto& m : cls.methods) {
        MethodInfo mi;
        genMethod(m, mi);
        mi.codeAttrNameIndex = codeAttr;
        methods.push_back(std::move(mi));
    }

    // Serialise the class file
    std::vector<uint8_t> bytes;
    ClassFileWriter::write(bytes, cp_, thisClass, superClass, methods);
    return bytes;
}

// ─────────────────────────────────────────────────────────────────────────────
// genMethod
// ─────────────────────────────────────────────────────────────────────────────

void CodeGen::genMethod(const MethodDecl& m, MethodInfo& out) {
    // Reset per-method state
    locals_.clear();
    nextSlot_ = 0;

    // Assign slots to parameters
    for (const auto& p : m.params) {
        locals_[p.name] = nextSlot_++;
    }

    // Register method name and descriptor in CP
    uint16_t nameIdx = cp_.addUtf8(m.name);
    uint16_t descIdx = cp_.addUtf8(methodDescriptor(m));

    // Build bytecode
    BytecodeBuilder bb;
    bb.requireLocals(nextSlot_); // at minimum, reserve space for params

    for (const auto& s : m.body)
        genStmt(*s, bb);

    // Implicit void return at end of void methods
    if (m.returnType == "void")
        bb.returnVoid();

    out.accessFlags      = accessFlags(m.modifiers);
    out.nameIndex        = nameIdx;
    out.descriptorIndex  = descIdx;
    out.maxStack         = bb.maxStack();
    out.maxLocals        = bb.maxLocals();
    out.code             = bb.code();
}

// ─────────────────────────────────────────────────────────────────────────────
// genStmt
// ─────────────────────────────────────────────────────────────────────────────

void CodeGen::genStmt(const Stmt& s, BytecodeBuilder& bb) {
    if (const auto* v = dynamic_cast<const VarDeclStmt*>(&s)) {
        // Evaluate initialiser, store to the next available local slot
        genExpr(*v->init, bb);
        int slot = nextSlot_++;
        locals_[v->name] = slot;
        bb.istore(slot);
        return;
    }

    if (const auto* e = dynamic_cast<const ExprStmt*>(&s)) {
        genExpr(*e->expr, bb);
        return;
    }

    if (const auto* r = dynamic_cast<const ReturnStmt*>(&s)) {
        if (r->value)
            genExpr(*r->value, bb);
        bb.returnVoid(); // Milestone 1: only void methods supported
        return;
    }

    throw CodeGenError("unsupported statement kind in codegen");
}

// ─────────────────────────────────────────────────────────────────────────────
// genExpr
// ─────────────────────────────────────────────────────────────────────────────

void CodeGen::genExpr(const Expr& e, BytecodeBuilder& bb) {
    if (const auto* lit = dynamic_cast<const IntLiteralExpr*>(&e)) {
        bb.pushInt(lit->value);
        return;
    }

    if (const auto* id = dynamic_cast<const IdentExpr*>(&e)) {
        auto it = locals_.find(id->name);
        if (it == locals_.end())
            throw CodeGenError("undeclared variable '" + id->name + "'");
        bb.iload(it->second);
        return;
    }

    if (const auto* bin = dynamic_cast<const BinaryExpr*>(&e)) {
        genExpr(*bin->left,  bb);
        genExpr(*bin->right, bb);
        switch (bin->op) {
            case '+': bb.iadd(); break;
            case '-': bb.isub(); break;
            case '*': bb.imul(); break;
            case '/': bb.idiv(); break;
            default:
                throw CodeGenError(
                    std::string("unsupported binary operator '") + bin->op + "'");
        }
        return;
    }

    if (const auto* mc = dynamic_cast<const MethodCallExpr*>(&e)) {
        genMethodCall(*mc, bb);
        return;
    }

    if (const auto* fa = dynamic_cast<const FieldAccessExpr*>(&e)) {
        // Field access expressions only appear as part of System.out.println;
        // they should never be generated standalone.
        throw CodeGenError("field access '" + fa->field + "' cannot stand alone");
    }

    throw CodeGenError("unsupported expression kind in codegen");
}

// ─────────────────────────────────────────────────────────────────────────────
// genMethodCall
// ─────────────────────────────────────────────────────────────────────────────

void CodeGen::genMethodCall(const MethodCallExpr& mc, BytecodeBuilder& bb) {
    // ── System.out.println(intExpr) ──────────────────────────────────────────
    if (mc.object) {
        const auto* fa = dynamic_cast<const FieldAccessExpr*>(mc.object.get());
        if (fa && fa->field == "out" && mc.method == "println") {
            const auto* sys = dynamic_cast<const IdentExpr*>(fa->object.get());
            if (sys && sys->name == "System") {
                if (mc.args.size() != 1)
                    throw CodeGenError(
                        "System.out.println expects 1 argument, got " +
                        std::to_string(mc.args.size()));

                // getstatic java/lang/System.out : Ljava/io/PrintStream;
                uint16_t fieldIdx = cp_.addFieldref(
                    "java/lang/System", "out", "Ljava/io/PrintStream;");
                bb.getstatic(fieldIdx);

                // Evaluate argument (must be int)
                genExpr(*mc.args[0], bb);

                // invokevirtual java/io/PrintStream.println : (I)V
                uint16_t methodIdx = cp_.addMethodref(
                    "java/io/PrintStream", "println", "(I)V");
                // pops: 1 objectref + 1 int arg = 2; return type void → no push
                bb.invokevirtual(methodIdx, /*popArgs=*/1, /*pushResult=*/false);
                return;
            }
        }
    }

    throw CodeGenError(
        "unsupported method call '" + mc.method + "' in Milestone 1 — "
        "only System.out.println(int) is supported");
}
