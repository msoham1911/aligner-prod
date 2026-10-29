#pragma once
#include "ast/Ast.h"
#include "parser/Diagnostic.h"
#include <map>
#include <string>

/// Semantic analyser — validates the AST after parsing.
///
/// Milestone-1 checks:
///   - Class must have a 'main' method with modifiers {public, static},
///     return type void, and exactly one parameter of type String[].
///   - All identifier expressions must refer to a variable declared earlier
///     in the same method body.
///   - System.out.println must receive exactly one int-typed argument.
///   - Binary arithmetic operands must be int-typed.
///   - Variable initialisers must be int-typed for int variables.
///
/// Throws SemanticError on the first violation found.
class Sema {
public:
    Sema() = default;

    /// Run all semantic checks. Throws SemanticError on failure.
    void analyse(const ClassDecl& cls);

private:
    // Current method's local variable table: name → type string ("int", …)
    std::map<std::string, std::string> locals_;

    void checkMethod(const MethodDecl& m);
    void checkStmt(const Stmt& s);
    std::string typeOfExpr(const Expr& e);
};
