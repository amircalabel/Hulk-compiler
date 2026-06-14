// src/ast/Stmt.hpp
#ifndef HULK_STMT_HPP
#define HULK_STMT_HPP

#include <memory>
#include <vector>
#include <string>
#include "scanner/Token.hpp"
#include "ast/Expr.hpp"

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
// Expression Statement: expression;
// (útil para llamadas a funciones con side effects)
// ============================================================
class ExpressionStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;

    explicit ExpressionStmt(std::unique_ptr<Expr> expression);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Print Statement: print expression;
// (built-in según sección A.2.3)
// ============================================================
class PrintStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;

    explicit PrintStmt(std::unique_ptr<Expr> expression);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Return Statement: return expression? ;
// (sección A.3.2)
// ============================================================
class ReturnStmt : public Stmt {
public:
    Token keyword;
    std::unique_ptr<Expr> value;  // nullptr si no hay valor de retorno

    ReturnStmt(Token keyword, std::unique_ptr<Expr> value);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Block Statement: { declaration* }
// (sección A.2.4 - los bloques son expresiones, pero también se usan como statements)
// Nota: HULK trata los bloques como expresiones (retornan el valor de la última expresión)
// pero también pueden usarse donde se espera un statement.
// ============================================================
class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Variable Declaration: var name = expression? ;
// (sección A.4 - let es expresión, var es statement a nivel global)
// ============================================================
class VarDeclStmt : public Stmt {
public:
    Token name;
    Token typeAnnotation;           // TOKEN_ERROR si no hay anotación (A.8.1)
    std::unique_ptr<Expr> initializer;  // nullptr si no hay inicializador

    VarDeclStmt(Token name, Token typeAnnotation, std::unique_ptr<Expr> initializer);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Function Declaration: function name(params) { body }
// (sección A.3)
// ============================================================
class FunctionDeclStmt : public Stmt {
public:
    Token name;
    struct Parameter {
        Token name;
        Token typeAnnotation;   // TOKEN_ERROR si no hay anotación (A.8.2)
    };
    std::vector<Parameter> parameters;
    Token returnTypeAnnotation;  // TOKEN_ERROR si no hay anotación
    std::vector<std::unique_ptr<Stmt>> body;  // bloque de statements

    FunctionDeclStmt(Token name, std::vector<Parameter> parameters,
                     Token returnTypeAnnotation, std::vector<std::unique_ptr<Stmt>> body);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Class Declaration: type name (args?) { attributes? methods? }
// (sección A.7)
// ============================================================
class ClassDeclStmt : public Stmt {
public:
    Token name;
    std::vector<Token> typeArguments;          // parámetros de tipo (x, y en type Point(x,y))
    std::vector<std::pair<Token, Token>> attributes;  // (nombre, tipoAnotacion) - A.8.3
    std::vector<std::unique_ptr<FunctionDeclStmt>> methods;
    Token superclass;                          // TOKEN_ERROR si no hay superclase (A.7.3)
    std::vector<std::unique_ptr<Expr>> superclassArguments;    // argumentos para el constructor de la superclase

    ClassDeclStmt(Token name, std::vector<Token> typeArguments,
                  std::vector<std::pair<Token, Token>> attributes,
                  std::vector<std::unique_ptr<FunctionDeclStmt>> methods,
                  Token superclass, std::vector<std::unique_ptr<Expr>> superclassArguments);

    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Protocol Declaration: protocol name { methodSignatures }
// (sección A.10)
// ============================================================
class ProtocolDeclStmt : public Stmt {
public:
    Token name;
    struct MethodSignature {
        Token name;
        std::vector<std::pair<Token, Token>> parameters;  // (nombre, tipo)
        Token returnType;
    };
    std::vector<MethodSignature> methods;
    Token extends;  // protocolo del que extiende (TOKEN_ERROR si ninguno)

    ProtocolDeclStmt(Token name, std::vector<MethodSignature> methods, Token extends);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Macro Declaration: def name(args) => expr;
// (sección A.14)
// ============================================================
class MacroDeclStmt : public Stmt {
public:
    Token name;
    struct Parameter {
        Token name;
        bool isSymbolic;      // true si es @param (parámetro simbólico, A.14.3)
        bool isPlaceholder;   // true si es $param (placeholder, A.14.4)
    };
    std::vector<Parameter> parameters;
    std::unique_ptr<Expr> body;
    bool hasPatternMatching;  // si usa match (A.14.5)

    MacroDeclStmt(Token name, std::vector<Parameter> parameters,
                  std::unique_ptr<Expr> body, bool hasPatternMatching);
    void accept(StmtVisitor& visitor) const override;
};

// ============================================================
// Visitor interface para statements
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
};

#endif // HULK_STMT_HPP