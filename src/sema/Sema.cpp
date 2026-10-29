#include "sema/Sema.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// analyse — class-level entry point
// ─────────────────────────────────────────────────────────────────────────────

void Sema::analyse(const ClassDecl& cls) {
    if (cls.name.empty())
        throw SemanticError("class must have a name");

    // Check that at least one method named 'main' exists
    bool foundMain = false;
    for (const auto& m : cls.methods) {
        if (m.name == "main") foundMain = true;
        checkMethod(m);
    }
    if (!foundMain)
        throw SemanticError("class '" + cls.name + "' must contain a 'main' method",
                            cls.line, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// checkMethod
// ─────────────────────────────────────────────────────────────────────────────

void Sema::checkMethod(const MethodDecl& m) {
    if (m.name == "main") {
        // Validate signature: public static void main(String[] args)
        bool hasPublic = std::find(m.modifiers.begin(), m.modifiers.end(), "public")  != m.modifiers.end();
        bool hasStatic = std::find(m.modifiers.begin(), m.modifiers.end(), "static") != m.modifiers.end();

        if (!hasPublic || !hasStatic)
            throw SemanticError(
                "'main' must be declared 'public static'", m.line, m.col);

        if (m.returnType != "void")
            throw SemanticError(
                "'main' must return void, got '" + m.returnType + "'", m.line, m.col);

        if (m.params.size() != 1 || m.params[0].type != "String[]")
            throw SemanticError(
                "'main' must have exactly one parameter of type String[]", m.line, m.col);
    }

    // Build the local variable symbol table for this method
    locals_.clear();

    // Parameters are in scope for the body
    for (const auto& p : m.params)
        locals_[p.name] = p.type;

    // Walk statements
    for (const auto& s : m.body)
        checkStmt(*s);
}

// ─────────────────────────────────────────────────────────────────────────────
// checkStmt
// ─────────────────────────────────────────────────────────────────────────────

void Sema::checkStmt(const Stmt& s) {
    if (const auto* v = dynamic_cast<const VarDeclStmt*>(&s)) {
        if (locals_.count(v->name))
            throw SemanticError(
                "variable '" + v->name + "' already declared", v->line, v->col);

        std::string initType = typeOfExpr(*v->init);

        if (v->type == "int" && initType != "int")
            throw SemanticError(
                "type mismatch: cannot assign '" + initType +
                "' to variable '" + v->name + "' of type 'int'",
                v->line, v->col);

        locals_[v->name] = v->type;
        return;
    }

    if (const auto* e = dynamic_cast<const ExprStmt*>(&s)) {
        typeOfExpr(*e->expr); // type-check the expression; result unused for void calls
        return;
    }

    if (const auto* r = dynamic_cast<const ReturnStmt*>(&s)) {
        if (r->value)
            typeOfExpr(*r->value);
        return;
    }

    throw SemanticError("unknown statement kind", s.line, s.col);
}

// ─────────────────────────────────────────────────────────────────────────────
// typeOfExpr — returns the Java type string of an expression
// ─────────────────────────────────────────────────────────────────────────────

std::string Sema::typeOfExpr(const Expr& e) {
    if (dynamic_cast<const IntLiteralExpr*>(&e))
        return "int";

    if (const auto* id = dynamic_cast<const IdentExpr*>(&e)) {
        auto it = locals_.find(id->name);
        if (it == locals_.end())
            throw SemanticError(
                "undeclared variable '" + id->name + "'", id->line, id->col);
        return it->second;
    }

    if (const auto* bin = dynamic_cast<const BinaryExpr*>(&e)) {
        std::string lType = typeOfExpr(*bin->left);
        std::string rType = typeOfExpr(*bin->right);
        if (lType != "int" || rType != "int")
            throw SemanticError(
                "arithmetic operator '" + std::string(1, bin->op) +
                "' requires int operands, got '" + lType + "' and '" + rType + "'",
                bin->line, bin->col);
        return "int";
    }

    if (const auto* mc = dynamic_cast<const MethodCallExpr*>(&e)) {
        // Supported pattern: System.out.println(intExpr)
        if (mc->object) {
            const auto* fa = dynamic_cast<const FieldAccessExpr*>(mc->object.get());
            if (fa && fa->field == "out" && mc->method == "println") {
                if (const auto* sys = dynamic_cast<const IdentExpr*>(fa->object.get())) {
                    if (sys->name == "System") {
                        if (mc->args.size() != 1)
                            throw SemanticError(
                                "System.out.println expects exactly 1 argument",
                                mc->line, mc->col);
                        std::string argType = typeOfExpr(*mc->args[0]);
                        if (argType != "int")
                            throw SemanticError(
                                "System.out.println(int) requires an int argument, got '" +
                                argType + "'",
                                mc->line, mc->col);
                        return "void";
                    }
                }
            }
        }
        throw SemanticError(
            "unsupported method call '" + mc->method + "' in Milestone 1",
            mc->line, mc->col);
    }

    if (const auto* fa = dynamic_cast<const FieldAccessExpr*>(&e)) {
        // Allow System.out as a sub-expression (type not used standalone)
        if (const auto* id = dynamic_cast<const IdentExpr*>(fa->object.get())) {
            if (id->name == "System" && fa->field == "out")
                return "PrintStream";
        }
        throw SemanticError(
            "unsupported field access '." + fa->field + "' in Milestone 1",
            fa->line, fa->col);
    }

    throw SemanticError("unknown expression kind", e.line, e.col);
    return ""; // unreachable
}
