// src/ast/Stmt.cpp
#include "Stmt.hpp"

// ============================================================
// ExpressionStmt
// ============================================================
ExpressionStmt::ExpressionStmt(std::unique_ptr<Expr> expression)
    : expression(std::move(expression)) {}

void ExpressionStmt::accept(StmtVisitor& visitor) const {
    visitor.visitExpressionStmt(*this);
}

// ============================================================
// PrintStmt
// ============================================================
PrintStmt::PrintStmt(std::unique_ptr<Expr> expression)
    : expression(std::move(expression)) {}

void PrintStmt::accept(StmtVisitor& visitor) const {
    visitor.visitPrintStmt(*this);
}

// ============================================================
// ReturnStmt
// ============================================================
ReturnStmt::ReturnStmt(Token keyword, std::unique_ptr<Expr> value)
    : keyword(keyword), value(std::move(value)) {}

void ReturnStmt::accept(StmtVisitor& visitor) const {
    visitor.visitReturnStmt(*this);
}

// ============================================================
// BlockStmt
// ============================================================
BlockStmt::BlockStmt(std::vector<std::unique_ptr<Stmt>> statements)
    : statements(std::move(statements)) {}

void BlockStmt::accept(StmtVisitor& visitor) const {
    visitor.visitBlockStmt(*this);
}

// ============================================================
// VarDeclStmt
// ============================================================
VarDeclStmt::VarDeclStmt(Token name, Token typeAnnotation, std::unique_ptr<Expr> initializer)
    : name(name), typeAnnotation(typeAnnotation), initializer(std::move(initializer)) {}

void VarDeclStmt::accept(StmtVisitor& visitor) const {
    visitor.visitVarDeclStmt(*this);
}

// ============================================================
// FunctionDeclStmt
// ============================================================
FunctionDeclStmt::FunctionDeclStmt(Token name, std::vector<Parameter> parameters,
                                   Token returnTypeAnnotation,
                                   std::vector<std::unique_ptr<Stmt>> body)
    : name(name), parameters(std::move(parameters)),
      returnTypeAnnotation(returnTypeAnnotation), body(std::move(body)) {}

void FunctionDeclStmt::accept(StmtVisitor& visitor) const {
    visitor.visitFunctionDeclStmt(*this);
}
// ============================================================
// IfStmt
// ============================================================
IfStmt::IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch,
               std::unique_ptr<Stmt> elseBranch)
    : condition(std::move(condition)), thenBranch(std::move(thenBranch)),
      elseBranch(std::move(elseBranch)) {}

void IfStmt::accept(StmtVisitor& visitor) const {
    visitor.visitIfStmt(*this);
}

// ============================================================
// WhileStmt
// ============================================================
WhileStmt::WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
    : condition(std::move(condition)), body(std::move(body)) {}

void WhileStmt::accept(StmtVisitor& visitor) const {
    visitor.visitWhileStmt(*this);
}

// ============================================================
// ForStmt
// ============================================================
ForStmt::ForStmt(std::unique_ptr<Stmt> initializer, std::unique_ptr<Expr> condition,
                 std::unique_ptr<Expr> increment, std::unique_ptr<Stmt> body)
    : initializer(std::move(initializer)), condition(std::move(condition)),
      increment(std::move(increment)), body(std::move(body)) {}

void ForStmt::accept(StmtVisitor& visitor) const {
    visitor.visitForStmt(*this);
}
// ============================================================
// ClassDeclStmt
// ============================================================
ClassDeclStmt::ClassDeclStmt(Token name, std::vector<Token> typeArguments,
                             std::vector<std::pair<Token, Token>> attributes,
                             std::vector<std::unique_ptr<FunctionDeclStmt>> methods,
                             Token superclass, std::vector<std::unique_ptr<Expr>> superclassArguments)
    : name(name), typeArguments(std::move(typeArguments)),
      attributes(std::move(attributes)), methods(std::move(methods)),
      superclass(superclass), superclassArguments(std::move(superclassArguments)) {}

void ClassDeclStmt::accept(StmtVisitor& visitor) const {
    visitor.visitClassDeclStmt(*this);
}

// ============================================================
// ProtocolDeclStmt
// ============================================================
ProtocolDeclStmt::ProtocolDeclStmt(Token name, std::vector<MethodSignature> methods, Token extends)
    : name(name), methods(std::move(methods)), extends(extends) {}

void ProtocolDeclStmt::accept(StmtVisitor& visitor) const {
    visitor.visitProtocolDeclStmt(*this);
}

// ============================================================
// MacroDeclStmt
// ============================================================
MacroDeclStmt::MacroDeclStmt(Token name, std::vector<Parameter> parameters,
                             std::unique_ptr<Expr> body, bool hasPatternMatching)
    : name(name), parameters(std::move(parameters)), body(std::move(body)),
      hasPatternMatching(hasPatternMatching) {}

void MacroDeclStmt::accept(StmtVisitor& visitor) const {
    visitor.visitMacroDeclStmt(*this);
}// namespace hulk