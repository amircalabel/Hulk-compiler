// src/ast/Expr.hpp
#ifndef HULK_EXPR_HPP
#define HULK_EXPR_HPP

#include <memory>
#include <vector>
#include <variant>
#include <string>
#include <utility>
#include "scanner/Token.hpp"

// Forward declarations
class ExprVisitor;

// ============================================================
// Clase base para expresiones
// ============================================================
class Expr {
public:
    virtual ~Expr() = default;
    virtual std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const = 0;
};

// ============================================================
// LiteralExpr
// ============================================================
class LiteralExpr : public Expr {
public:
    std::variant<double, std::string, bool, std::nullptr_t> value;

    explicit LiteralExpr(const std::variant<double, std::string, bool, std::nullptr_t>& value);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// BinaryExpr
// ============================================================
class BinaryExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// UnaryExpr
// ============================================================
class UnaryExpr : public Expr {
public:
    Token op;
    std::unique_ptr<Expr> right;

    UnaryExpr(Token op, std::unique_ptr<Expr> right);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// GroupingExpr
// ============================================================
class GroupingExpr : public Expr {
public:
    std::unique_ptr<Expr> expression;

    explicit GroupingExpr(std::unique_ptr<Expr> expression);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// VariableExpr
// ============================================================
class VariableExpr : public Expr {
public:
    Token name;

    explicit VariableExpr(Token name);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// AssignExpr
// ============================================================
class AssignExpr : public Expr {
public:
    Token name;
    std::unique_ptr<Expr> value;

    AssignExpr(Token name, std::unique_ptr<Expr> value);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// LetExpr
// ============================================================
class LetExpr : public Expr {
public:
    struct Binding {
        Token name;
        Token typeAnnotation;     // TOKEN_ERROR si no hay anotación de tipo
        std::unique_ptr<Expr> initializer;
    };
    std::vector<Binding> bindings;
    std::unique_ptr<Expr> body;

    LetExpr(std::vector<Binding> bindings, std::unique_ptr<Expr> body);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// IfExpr (soporta elif)
// ============================================================
class IfExpr : public Expr {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> thenBranch;
    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> elifBranches;
    std::unique_ptr<Expr> elseBranch;

    IfExpr(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> thenBranch,
           std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> elifBranches,
           std::unique_ptr<Expr> elseBranch);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// WhileExpr
// ============================================================
class WhileExpr : public Expr {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> body;

    WhileExpr(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> body);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// ForExpr
// ============================================================
class ForExpr : public Expr {
public:
    std::unique_ptr<Expr> initializer;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<Expr> body;

    ForExpr(std::unique_ptr<Expr> initializer, std::unique_ptr<Expr> condition,
            std::unique_ptr<Expr> increment, std::unique_ptr<Expr> body);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// BlockExpr
// ============================================================
class BlockExpr : public Expr {
public:
    std::vector<std::unique_ptr<Expr>> expressions;

    explicit BlockExpr(std::vector<std::unique_ptr<Expr>> expressions);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// CallExpr
// ============================================================
class CallExpr : public Expr {
public:
    std::unique_ptr<Expr> callee;
    Token paren;      // para reporte de errores
    std::vector<std::unique_ptr<Expr>> arguments;

    CallExpr(std::unique_ptr<Expr> callee, Token paren, std::vector<std::unique_ptr<Expr>> arguments);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// GetExpr  (object.property)
// ============================================================
class GetExpr : public Expr {
public:
    std::unique_ptr<Expr> object;
    Token name;

    GetExpr(std::unique_ptr<Expr> object, Token name);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// SetExpr  (object.property := value)
// ============================================================
class SetExpr : public Expr {
public:
    std::unique_ptr<Expr> object;
    Token name;
    std::unique_ptr<Expr> value;

    SetExpr(std::unique_ptr<Expr> object, Token name, std::unique_ptr<Expr> value);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// SelfExpr  (self)
// ============================================================
class SelfExpr : public Expr {
public:
    Token keyword;

    explicit SelfExpr(Token keyword);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// NewExpr  (new Class(args))
// ============================================================
class NewExpr : public Expr {
public:
    Token className;
    std::vector<std::unique_ptr<Expr>> arguments;

    NewExpr(Token className, std::vector<std::unique_ptr<Expr>> arguments);
    std::variant<double, std::string, bool, std::nullptr_t> accept(ExprVisitor& visitor) const override;
};

// ============================================================
// Visitor interface
// ============================================================
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;

    virtual std::variant<double, std::string, bool, std::nullptr_t> visitLiteralExpr(const LiteralExpr& expr) = 0;
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitBinaryExpr(const BinaryExpr& expr) = 0;
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitUnaryExpr(const UnaryExpr& expr) = 0;
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitGroupingExpr(const GroupingExpr& expr) = 0;
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitVariableExpr(const VariableExpr& expr) = 0;
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitAssignExpr(const AssignExpr& expr) = 0;
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitLetExpr(const LetExpr& expr) = 0;
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitIfExpr(const IfExpr& expr) = 0;
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitWhileExpr(const WhileExpr& expr) = 0;
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitForExpr(const ForExpr& expr) = 0;
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitBlockExpr(const BlockExpr& expr) = 0;
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitCallExpr(const CallExpr& expr) = 0;

    // Nodos de POO (métodos por defecto para no romper visitantes existentes)
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitGetExpr(const GetExpr&) { return nullptr; }
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitSetExpr(const SetExpr&) { return nullptr; }
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitSelfExpr(const SelfExpr&) { return nullptr; }
    virtual std::variant<double, std::string, bool, std::nullptr_t> visitNewExpr(const NewExpr&) { return nullptr; }
};

#endif // HULK_EXPR_HPP
