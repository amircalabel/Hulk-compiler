// src/ast/Expr.cpp
#include "Expr.hpp"

// ============================================================
// LiteralExpr
// ============================================================
LiteralExpr::LiteralExpr(const std::variant<double, std::string, bool, std::nullptr_t>& value)
    : value(value) {}

std::variant<double, std::string, bool, std::nullptr_t> LiteralExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitLiteralExpr(*this);
}

// ============================================================
// BinaryExpr
// ============================================================
BinaryExpr::BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
    : left(std::move(left)), op(op), right(std::move(right)) {}

std::variant<double, std::string, bool, std::nullptr_t> BinaryExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitBinaryExpr(*this);
}

// ============================================================
// UnaryExpr
// ============================================================
UnaryExpr::UnaryExpr(Token op, std::unique_ptr<Expr> right)
    : op(op), right(std::move(right)) {}

std::variant<double, std::string, bool, std::nullptr_t> UnaryExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitUnaryExpr(*this);
}

// ============================================================
// GroupingExpr
// ============================================================
GroupingExpr::GroupingExpr(std::unique_ptr<Expr> expression)
    : expression(std::move(expression)) {}

std::variant<double, std::string, bool, std::nullptr_t> GroupingExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitGroupingExpr(*this);
}

// ============================================================
// VariableExpr
// ============================================================
VariableExpr::VariableExpr(Token name)
    : name(name) {}

std::variant<double, std::string, bool, std::nullptr_t> VariableExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitVariableExpr(*this);
}

// ============================================================
// AssignExpr
// ============================================================
AssignExpr::AssignExpr(Token name, std::unique_ptr<Expr> value)
    : name(name), value(std::move(value)) {}

std::variant<double, std::string, bool, std::nullptr_t> AssignExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitAssignExpr(*this);
}

// ============================================================
// LetExpr
// ============================================================
LetExpr::LetExpr(std::vector<Binding> bindings, std::unique_ptr<Expr> body)
    : bindings(std::move(bindings)), body(std::move(body)) {}

std::variant<double, std::string, bool, std::nullptr_t> LetExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitLetExpr(*this);
}

// ============================================================
// IfExpr (con soporte para elif)
// ============================================================
IfExpr::IfExpr(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> thenBranch,
               std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> elifBranches,
               std::unique_ptr<Expr> elseBranch)
    : condition(std::move(condition)), thenBranch(std::move(thenBranch)),
      elifBranches(std::move(elifBranches)), elseBranch(std::move(elseBranch)) {}

std::variant<double, std::string, bool, std::nullptr_t> IfExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitIfExpr(*this);
}

// ============================================================
// WhileExpr
// ============================================================
WhileExpr::WhileExpr(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> body)
    : condition(std::move(condition)), body(std::move(body)) {}

std::variant<double, std::string, bool, std::nullptr_t> WhileExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitWhileExpr(*this);
}

// ============================================================
// ForExpr
// ============================================================
ForExpr::ForExpr(std::unique_ptr<Expr> initializer, std::unique_ptr<Expr> condition,
                 std::unique_ptr<Expr> increment, std::unique_ptr<Expr> body)
    : initializer(std::move(initializer)), condition(std::move(condition)),
      increment(std::move(increment)), body(std::move(body)) {}

std::variant<double, std::string, bool, std::nullptr_t> ForExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitForExpr(*this);
}

// ============================================================
// BlockExpr
// ============================================================
BlockExpr::BlockExpr(std::vector<std::unique_ptr<Expr>> expressions)
    : expressions(std::move(expressions)) {}

std::variant<double, std::string, bool, std::nullptr_t> BlockExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitBlockExpr(*this);
}

// ============================================================
// CallExpr
// ============================================================
CallExpr::CallExpr(std::unique_ptr<Expr> callee, Token paren,
                   std::vector<std::unique_ptr<Expr>> arguments)
    : callee(std::move(callee)), paren(paren), arguments(std::move(arguments)) {}

std::variant<double, std::string, bool, std::nullptr_t> CallExpr::accept(ExprVisitor& visitor) const {
    return visitor.visitCallExpr(*this);
} 