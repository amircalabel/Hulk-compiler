// src/inferer/TypeInferer.cpp
#include "TypeInferer.hpp"
#include <iostream>

namespace hulk {

TypeInferer::TypeInferer(Resolver& resolver) : resolver(resolver) {}

void TypeInferer::infer(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& stmt : statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
}

// Statements - implementación vacía
void TypeInferer::visitExpressionStmt(const ExpressionStmt& stmt) {
    if (stmt.expression) stmt.expression->accept(*this);
}

void TypeInferer::visitPrintStmt(const PrintStmt& stmt) {
    if (stmt.expression) stmt.expression->accept(*this);
}

void TypeInferer::visitReturnStmt(const ReturnStmt& stmt) {
    if (stmt.value) stmt.value->accept(*this);
}

void TypeInferer::visitBlockStmt(const BlockStmt& stmt) {
    for (const auto& s : stmt.statements) {
        if (s) s->accept(*this);
    }
}

void TypeInferer::visitVarDeclStmt(const VarDeclStmt& stmt) {
    if (stmt.initializer) stmt.initializer->accept(*this);
}

void TypeInferer::visitFunctionDeclStmt(const FunctionDeclStmt& stmt) {
    for (const auto& body : stmt.body) {
        if (body) body->accept(*this);
    }
}

void TypeInferer::visitClassDeclStmt(const ClassDeclStmt& stmt) {
    for (const auto& method : stmt.methods) {
        if (method) method->accept(*this);
    }
}

void TypeInferer::visitProtocolDeclStmt(const ProtocolDeclStmt& stmt) {
    // Nothing to do
}

void TypeInferer::visitMacroDeclStmt(const MacroDeclStmt& stmt) {
    if (stmt.body) stmt.body->accept(*this);
}

// Expressions
std::variant<double, std::string, bool, std::nullptr_t> 
TypeInferer::visitLiteralExpr(const LiteralExpr& expr) {
    return expr.value;
}

std::variant<double, std::string, bool, std::nullptr_t> 
TypeInferer::visitBinaryExpr(const BinaryExpr& expr) {
    if (expr.left) expr.left->accept(*this);
    if (expr.right) expr.right->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
TypeInferer::visitUnaryExpr(const UnaryExpr& expr) {
    if (expr.right) expr.right->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
TypeInferer::visitGroupingExpr(const GroupingExpr& expr) {
    if (expr.expression) expr.expression->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
TypeInferer::visitVariableExpr(const VariableExpr& expr) {
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
TypeInferer::visitAssignExpr(const AssignExpr& expr) {
    if (expr.value) expr.value->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
TypeInferer::visitLetExpr(const LetExpr& expr) {
    for (const auto& binding : expr.bindings) {
        if (binding.initializer) binding.initializer->accept(*this);
    }
    if (expr.body) expr.body->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
TypeInferer::visitIfExpr(const IfExpr& expr) {
    if (expr.condition) expr.condition->accept(*this);
    if (expr.thenBranch) expr.thenBranch->accept(*this);
    for (const auto& elif : expr.elifBranches) {
        if (elif.first) elif.first->accept(*this);
        if (elif.second) elif.second->accept(*this);
    }
    if (expr.elseBranch) expr.elseBranch->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
TypeInferer::visitWhileExpr(const WhileExpr& expr) {
    if (expr.condition) expr.condition->accept(*this);
    if (expr.body) expr.body->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
TypeInferer::visitForExpr(const ForExpr& expr) {
    if (expr.initializer) expr.initializer->accept(*this);
    if (expr.condition) expr.condition->accept(*this);
    if (expr.increment) expr.increment->accept(*this);
    if (expr.body) expr.body->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
TypeInferer::visitBlockExpr(const BlockExpr& expr) {
    for (const auto& e : expr.expressions) {
        if (e) e->accept(*this);
    }
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
TypeInferer::visitCallExpr(const CallExpr& expr) {
    if (expr.callee) expr.callee->accept(*this);
    for (const auto& arg : expr.arguments) {
        if (arg) arg->accept(*this);
    }
    return 0.0;
}

} // namespace hulk