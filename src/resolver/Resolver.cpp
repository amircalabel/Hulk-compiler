// src/resolver/Resolver.cpp
#include "Resolver.hpp"
#include "interpreter/Interpreter.hpp"
#include <iostream>

namespace hulk {

Resolver::Resolver(Interpreter& interpreter) : interpreter(interpreter) {}

void Resolver::resolve(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& stmt : statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
}

void Resolver::beginScope() {
    scopes.push_back({});
}

void Resolver::endScope() {
    scopes.pop_back();
}

void Resolver::declare(const Token& name) {
    if (scopes.empty()) return;
    scopes.back()[name.lexeme] = false;
}

void Resolver::define(const Token& name) {
    if (scopes.empty()) return;
    scopes.back()[name.lexeme] = true;
}

// Statements - implementación simple
void Resolver::visitExpressionStmt(const ExpressionStmt& stmt) {
    if (stmt.expression) stmt.expression->accept(*this);
}

void Resolver::visitPrintStmt(const PrintStmt& stmt) {
    if (stmt.expression) stmt.expression->accept(*this);
}

void Resolver::visitReturnStmt(const ReturnStmt& stmt) {
    if (stmt.value) stmt.value->accept(*this);
}

void Resolver::visitBlockStmt(const BlockStmt& stmt) {
    beginScope();
    for (const auto& s : stmt.statements) {
        if (s) s->accept(*this);
    }
    endScope();
}

void Resolver::visitVarDeclStmt(const VarDeclStmt& stmt) {
    declare(stmt.name);
    if (stmt.initializer) stmt.initializer->accept(*this);
    define(stmt.name);
}

void Resolver::visitFunctionDeclStmt(const FunctionDeclStmt& stmt) {
    declare(stmt.name);
    define(stmt.name);
    beginScope();
    for (const auto& param : stmt.parameters) {
        declare(param.name);
        define(param.name);
    }
    for (const auto& body : stmt.body) {
        if (body) body->accept(*this);
    }
    endScope();
}

void Resolver::visitClassDeclStmt(const ClassDeclStmt& stmt) {
    declare(stmt.name);
    define(stmt.name);
}

void Resolver::visitProtocolDeclStmt(const ProtocolDeclStmt& stmt) {
    declare(stmt.name);
    define(stmt.name);
}

void Resolver::visitMacroDeclStmt(const MacroDeclStmt& stmt) {
    declare(stmt.name);
    define(stmt.name);
    if (stmt.body) stmt.body->accept(*this);
}

void Resolver::visitIfStmt(const IfStmt& stmt) {
    if (stmt.condition) stmt.condition->accept(*this);
    if (stmt.thenBranch) stmt.thenBranch->accept(*this);
    if (stmt.elseBranch) stmt.elseBranch->accept(*this);
}

void Resolver::visitWhileStmt(const WhileStmt& stmt) {
    if (stmt.condition) stmt.condition->accept(*this);
    if (stmt.body) stmt.body->accept(*this);
}

void Resolver::visitForStmt(const ForStmt& stmt) {
    beginScope();
    if (stmt.initializer) stmt.initializer->accept(*this);
    if (stmt.condition) stmt.condition->accept(*this);
    if (stmt.increment) stmt.increment->accept(*this);
    if (stmt.body) stmt.body->accept(*this);
    endScope();
}

// Expressions
std::variant<double, std::string, bool, std::nullptr_t> 
Resolver::visitLiteralExpr(const LiteralExpr& expr) {
    return expr.value;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Resolver::visitBinaryExpr(const BinaryExpr& expr) {
    if (expr.left) expr.left->accept(*this);
    if (expr.right) expr.right->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Resolver::visitUnaryExpr(const UnaryExpr& expr) {
    if (expr.right) expr.right->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Resolver::visitGroupingExpr(const GroupingExpr& expr) {
    if (expr.expression) expr.expression->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Resolver::visitVariableExpr(const VariableExpr& expr) {
    resolveLocal(expr, expr.name);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Resolver::visitAssignExpr(const AssignExpr& expr) {
    if (expr.value) expr.value->accept(*this);
    resolveLocal(expr, expr.name);
    return 0.0;
}

void Resolver::resolveLocal(const Expr& expr, const Token& name) {
    for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; --i) {
        if (scopes[i].find(name.lexeme) != scopes[i].end()) {
            int distance = static_cast<int>(scopes.size()) - 1 - i;
            interpreter.resolve(const_cast<Expr&>(expr), distance);
            return;
        }
    }
}

std::variant<double, std::string, bool, std::nullptr_t> 
Resolver::visitLetExpr(const LetExpr& expr) {
    beginScope();
    for (const auto& binding : expr.bindings) {
        declare(binding.name);
        if (binding.initializer) binding.initializer->accept(*this);
        define(binding.name);
    }
    if (expr.body) expr.body->accept(*this);
    endScope();
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Resolver::visitIfExpr(const IfExpr& expr) {
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
Resolver::visitWhileExpr(const WhileExpr& expr) {
    if (expr.condition) expr.condition->accept(*this);
    if (expr.body) expr.body->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Resolver::visitForExpr(const ForExpr& expr) {
    beginScope();
    if (expr.initializer) expr.initializer->accept(*this);
    if (expr.condition) expr.condition->accept(*this);
    if (expr.increment) expr.increment->accept(*this);
    if (expr.body) expr.body->accept(*this);
    endScope();
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Resolver::visitBlockExpr(const BlockExpr& expr) {
    beginScope();
    for (const auto& e : expr.expressions) {
        if (e) e->accept(*this);
    }
    endScope();
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Resolver::visitCallExpr(const CallExpr& expr) {
    if (expr.callee) expr.callee->accept(*this);
    for (const auto& arg : expr.arguments) {
        if (arg) arg->accept(*this);
    }
    return 0.0;
}

} // namespace hulk
