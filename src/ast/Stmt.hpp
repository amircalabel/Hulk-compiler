// src/ast/Stmt.hpp
#ifndef HULK_STMT_HPP
#define HULK_STMT_HPP

#include <memory>
#include <vector>
#include <string>
#include "scanner/Token.hpp"
#include "ast/Expr.hpp"

namespace hulk {

// Forward declarations
class StmtVisitor;

// ============================================================
// Clase base para statements
// ============================================================
class Stmt {
public:
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitor& visitor) const = 0;
};

// ============================================================
// Expression Statement
// ============================================================
class ExpressionStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;
    explicit ExpressionStmt(std::unique_ptr<Expr> expression);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Print Statement
// ============================================================
class PrintStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;
    explicit PrintStmt(std::unique_ptr<Expr> expression);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Return Statement
// ============================================================
class ReturnStmt : public Stmt {
public:
    Token keyword;
    std::unique_ptr<Expr> value;
    ReturnStmt(Token keyword, std::unique_ptr<Expr> value);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Block Statement
// ============================================================
class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Variable Declaration
// ============================================================
class VarDeclStmt : public Stmt {
public:
    Token name;
    Token typeAnnotation;
    std::unique_ptr<Expr> initializer;
    VarDeclStmt(Token name, Token typeAnnotation, std::unique_ptr<Expr> initializer);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Function Declaration
// ============================================================
class FunctionDeclStmt : public Stmt {
public:
    struct Parameter {
        Token name;
        Token typeAnnotation;
    };
    Token name;
    std::vector<Parameter> parameters;
    Token returnTypeAnnotation;
    std::vector<std::unique_ptr<Stmt>> body;
    FunctionDeclStmt(Token name, std::vector<Parameter> parameters,
                     Token returnTypeAnnotation, std::vector<std::unique_ptr<Stmt>> body);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// If Statement
// ============================================================
class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch,
           std::unique_ptr<Stmt> elseBranch);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// While Statement
// ============================================================
class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// For Statement
// ============================================================
class ForStmt : public Stmt {
public:
    std::unique_ptr<Stmt> initializer;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<Stmt> body;
    ForStmt(std::unique_ptr<Stmt> initializer, std::unique_ptr<Expr> condition,
            std::unique_ptr<Expr> increment, std::unique_ptr<Stmt> body);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Class Declaration
// ============================================================
class ClassDeclStmt : public Stmt {
public:
    Token name;
    std::vector<Token> typeArguments;
    std::vector<std::pair<Token, Token>> attributes;
    std::vector<std::unique_ptr<FunctionDeclStmt>> methods;
    Token superclass;
    std::vector<std::unique_ptr<Expr>> superclassArguments;
    ClassDeclStmt(Token name, std::vector<Token> typeArguments,
                  std::vector<std::pair<Token, Token>> attributes,
                  std::vector<std::unique_ptr<FunctionDeclStmt>> methods,
                  Token superclass, std::vector<std::unique_ptr<Expr>> superclassArguments);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Protocol Declaration
// ============================================================
class ProtocolDeclStmt : public Stmt {
public:
    struct MethodSignature {
        Token name;
        std::vector<std::pair<Token, Token>> parameters;
        Token returnType;
    };
    Token name;
    std::vector<MethodSignature> methods;
    Token extends;
    ProtocolDeclStmt(Token name, std::vector<MethodSignature> methods, Token extends);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Macro Declaration
// ============================================================
class MacroDeclStmt : public Stmt {
public:
    struct Parameter {
        Token name;
        bool isSymbolic;
        bool isPlaceholder;
    };
    Token name;
    std::vector<Parameter> parameters;
    std::unique_ptr<Expr> body;
    bool hasPatternMatching;
    MacroDeclStmt(Token name, std::vector<Parameter> parameters,
                  std::unique_ptr<Expr> body, bool hasPatternMatching);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// StmtVisitor Interface
// ============================================================
class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;
    virtual void visitExpressionStmt(const ExpressionStmt& stmt) = 0;
    virtual void visitPrintStmt(const PrintStmt& stmt) = 0;
    virtual void visitReturnStmt(const ReturnStmt& stmt) = 0;
    virtual void visitBlockStmt(const BlockStmt& stmt) = 0;
    virtual void visitVarDeclStmt(const VarDeclStmt& stmt) = 0;
    virtual void visitFunctionDeclStmt(const FunctionDeclStmt& stmt) = 0;
    virtual void visitClassDeclStmt(const ClassDeclStmt& stmt) = 0;
    virtual void visitProtocolDeclStmt(const ProtocolDeclStmt& stmt) = 0;
    virtual void visitMacroDeclStmt(const MacroDeclStmt& stmt) = 0;
    virtual void visitIfStmt(const IfStmt& stmt) = 0;
    virtual void visitWhileStmt(const WhileStmt& stmt) = 0;
    virtual void visitForStmt(const ForStmt& stmt) = 0;
};

} // namespace hulk

#endif // HULK_STMT_HPP