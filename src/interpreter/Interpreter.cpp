// src/interpreter/Interpreter.cpp
#include "Interpreter.hpp"
#include <cmath>
#include <sstream>

namespace hulk {

// ============================================================
// Interpreter Implementation
// ============================================================

Interpreter::Interpreter() {
    globals = std::make_shared<Environment>();
    environment = globals;
}

Interpreter::~Interpreter() = default;

void Interpreter::interpret(const std::vector<std::unique_ptr<Stmt>>& statements) {
    try {
        for (const auto& stmt : statements) {
            if (stmt) {
                execute(*stmt);
            }
        }
    } catch (const std::exception& e) {
        runtimeError(e.what());
    }
}

void Interpreter::resolve(Expr& expr, int depth) {
    locals[&expr] = depth;
}

void Interpreter::execute(Stmt& stmt) {
    stmt.accept(*this);
}

std::variant<double, std::string, bool, std::nullptr_t> Interpreter::evaluate(Expr& expr) {
    return expr.accept(*this);
}

bool Interpreter::isTruthy(const std::variant<double, std::string, bool, std::nullptr_t>& value) const {
    if (std::holds_alternative<std::nullptr_t>(value)) return false;
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value);
    return true;
}

bool Interpreter::isEqual(const std::variant<double, std::string, bool, std::nullptr_t>& a,
                          const std::variant<double, std::string, bool, std::nullptr_t>& b) const {
    if (std::holds_alternative<std::nullptr_t>(a) && std::holds_alternative<std::nullptr_t>(b)) return true;
    if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b)) return std::get<bool>(a) == std::get<bool>(b);
    if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b)) return std::get<double>(a) == std::get<double>(b);
    if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b)) return std::get<std::string>(a) == std::get<std::string>(b);
    return false;
}

std::string Interpreter::stringify(const std::variant<double, std::string, bool, std::nullptr_t>& value) const {
    if (std::holds_alternative<std::nullptr_t>(value)) return "nil";
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value) ? "true" : "false";
    if (std::holds_alternative<double>(value)) {
        double num = std::get<double>(value);
        if (num == static_cast<int>(num)) return std::to_string(static_cast<int>(num));
        return std::to_string(num);
    }
    if (std::holds_alternative<std::string>(value)) return std::get<std::string>(value);
    return "unknown";
}

void Interpreter::runtimeError(const std::string& message) {
    throw std::runtime_error(message);
}

// ============================================================
// Statements
// ============================================================

void Interpreter::visitExpressionStmt(const ExpressionStmt& stmt) {
    if (stmt.expression) {
        evaluate(*stmt.expression);
    }
}

void Interpreter::visitPrintStmt(const PrintStmt& stmt) {
    if (stmt.expression) {
        auto value = evaluate(*stmt.expression);
        std::cout << stringify(value) << std::endl;
    }
}

void Interpreter::visitReturnStmt(const ReturnStmt& stmt) {
    if (stmt.value) {
        evaluate(*stmt.value);
    }
}

void Interpreter::visitBlockStmt(const BlockStmt& stmt) {
    auto previous = environment;
    environment = std::make_shared<Environment>(environment);
    
    for (const auto& s : stmt.statements) {
        if (s) {
            execute(*s);
        }
    }
    
    environment = previous;
}

void Interpreter::visitVarDeclStmt(const VarDeclStmt& stmt) {
    std::variant<double, std::string, bool, std::nullptr_t> value = nullptr;
    if (stmt.initializer) {
        value = evaluate(*stmt.initializer);
    }
    environment->define(stmt.name.lexeme, value);
}

void Interpreter::visitFunctionDeclStmt(const FunctionDeclStmt& stmt) {
    // Por ahora, solo almacenamos que la función existe
    environment->define(stmt.name.lexeme, nullptr);
}

void Interpreter::visitClassDeclStmt(const ClassDeclStmt& stmt) {
    environment->define(stmt.name.lexeme, nullptr);
}

void Interpreter::visitProtocolDeclStmt(const ProtocolDeclStmt& stmt) {
    environment->define(stmt.name.lexeme, nullptr);
}

void Interpreter::visitMacroDeclStmt(const MacroDeclStmt& stmt) {
    environment->define(stmt.name.lexeme, nullptr);
}

void Interpreter::visitIfStmt(const IfStmt& stmt) {
    auto condition = evaluate(*stmt.condition);
    if (isTruthy(condition)) {
        execute(*stmt.thenBranch);
    } else if (stmt.elseBranch) {
        execute(*stmt.elseBranch);
    }
}

void Interpreter::visitWhileStmt(const WhileStmt& stmt) {
    auto condition = evaluate(*stmt.condition);
    while (isTruthy(condition)) {
        execute(*stmt.body);
        condition = evaluate(*stmt.condition);
    }
}

void Interpreter::visitForStmt(const ForStmt& stmt) {
    // Por ahora, simplificado
    if (stmt.body) {
        execute(*stmt.body);
    }
}

// ============================================================
// Expressions
// ============================================================

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitLiteralExpr(const LiteralExpr& expr) {
    return expr.value;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitBinaryExpr(const BinaryExpr& expr) {
    auto left = evaluate(*expr.left);
    auto right = evaluate(*expr.right);
    
    switch (expr.op.type) {
        case TokenType::TOKEN_PLUS:
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                return std::get<double>(left) + std::get<double>(right);
            }
            if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
                return std::get<std::string>(left) + std::get<std::string>(right);
            }
            runtimeError("Operands must be numbers or strings");
            return nullptr;
            
        case TokenType::TOKEN_MINUS:
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                return std::get<double>(left) - std::get<double>(right);
            }
            runtimeError("Operands must be numbers");
            return nullptr;
            
        case TokenType::TOKEN_STAR:
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                return std::get<double>(left) * std::get<double>(right);
            }
            runtimeError("Operands must be numbers");
            return nullptr;
            
        case TokenType::TOKEN_SLASH:
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                if (std::get<double>(right) == 0) runtimeError("Division by zero");
                return std::get<double>(left) / std::get<double>(right);
            }
            runtimeError("Operands must be numbers");
            return nullptr;
        case TokenType::TOKEN_PERCENT:
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                if (std::get<double>(right) == 0) runtimeError("Modulo by zero");
                return std::fmod(std::get<double>(left), std::get<double>(right));
            }
            runtimeError("Operands must be numbers");
            return nullptr;
            
        case TokenType::TOKEN_EQUAL_EQUAL:
            return isEqual(left, right);
            
        case TokenType::TOKEN_BANG_EQUAL:
            return !isEqual(left, right);
            
        case TokenType::TOKEN_GREATER:
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                return std::get<double>(left) > std::get<double>(right);
            }
            runtimeError("Operands must be numbers");
            return nullptr;
        case TokenType::TOKEN_GREATER_EQUAL:
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                return std::get<double>(left) >= std::get<double>(right);
            }
            runtimeError("Operands must be numbers");
            return nullptr;
            
        case TokenType::TOKEN_LESS:
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                return std::get<double>(left) < std::get<double>(right);
            }
            runtimeError("Operands must be numbers");
            return nullptr;
        case TokenType::TOKEN_LESS_EQUAL:
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                return std::get<double>(left) <= std::get<double>(right);
            }
            runtimeError("Operands must be numbers");
            return nullptr;
        case TokenType::TOKEN_AND:
            return isTruthy(left) && isTruthy(right);
        case TokenType::TOKEN_OR:
            return isTruthy(left) || isTruthy(right);
            
        default:
            runtimeError("Unknown operator");
            return nullptr;
    }
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitUnaryExpr(const UnaryExpr& expr) {
    auto right = evaluate(*expr.right);
    
    switch (expr.op.type) {
        case TokenType::TOKEN_MINUS:
            if (std::holds_alternative<double>(right)) {
                return -std::get<double>(right);
            }
            runtimeError("Operand must be a number");
            return nullptr;
        case TokenType::TOKEN_BANG:
        case TokenType::TOKEN_NOT:
            return !isTruthy(right);
        default:
            runtimeError("Unknown operator");
            return nullptr;
    }
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitGroupingExpr(const GroupingExpr& expr) {
    return evaluate(*expr.expression);
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitVariableExpr(const VariableExpr& expr) {
    auto value = environment->get(expr.name.lexeme);
    return value;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitAssignExpr(const AssignExpr& expr) {
    auto value = evaluate(*expr.value);
    environment->assign(expr.name.lexeme, value);
    return value;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitLetExpr(const LetExpr& expr) {
    auto previous = environment;
    environment = std::make_shared<Environment>(environment);
    
    for (const auto& binding : expr.bindings) {
        auto value = evaluate(*binding.initializer);
        environment->define(binding.name.lexeme, value);
    }
    
    auto result = evaluate(*expr.body);
    environment = previous;
    return result;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitIfExpr(const IfExpr& expr) {
    auto condition = evaluate(*expr.condition);
    if (isTruthy(condition)) {
        return evaluate(*expr.thenBranch);
    }
    if (expr.elseBranch) {
        return evaluate(*expr.elseBranch);
    }
    return nullptr;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitWhileExpr(const WhileExpr& expr) {
    auto condition = evaluate(*expr.condition);
    while (isTruthy(condition)) {
        evaluate(*expr.body);
        condition = evaluate(*expr.condition);
    }
    return nullptr;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitForExpr(const ForExpr& expr) {
    if (expr.initializer) {
        evaluate(*expr.initializer);
    }
    
    auto condition = expr.condition ? evaluate(*expr.condition) : true;
    
    while (isTruthy(condition)) {
        if (expr.body) {
            evaluate(*expr.body);
        }
        if (expr.increment) {
            evaluate(*expr.increment);
        }
        if (expr.condition) {
            condition = evaluate(*expr.condition);
        }
    }
    return nullptr;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitBlockExpr(const BlockExpr& expr) {
    auto previous = environment;
    environment = std::make_shared<Environment>(environment);
    
    std::variant<double, std::string, bool, std::nullptr_t> result = nullptr;
    for (const auto& e : expr.expressions) {
        result = evaluate(*e);
    }
    
    environment = previous;
    return result;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitCallExpr(const CallExpr& expr) {
    // Por ahora, las llamadas a funciones retornan nil
    evaluate(*expr.callee);
    for (const auto& arg : expr.arguments) {
        evaluate(*arg);
    }
    return nullptr;
}

// ============================================================
// Environment Implementation
// ============================================================

Environment::Environment(std::shared_ptr<Environment> enclosing)
    : enclosing(std::move(enclosing)) {}

void Environment::define(const std::string& name, const std::variant<double, std::string, bool, std::nullptr_t>& value) {
    values[name] = value;
}

std::variant<double, std::string, bool, std::nullptr_t> Environment::get(const std::string& name) const {
    auto it = values.find(name);
    if (it != values.end()) {
        return it->second;
    }
    
    if (enclosing) {
        return enclosing->get(name);
    }
    
    return nullptr;
}

void Environment::assign(const std::string& name, const std::variant<double, std::string, bool, std::nullptr_t>& value) {
    auto it = values.find(name);
    if (it != values.end()) {
        it->second = value;
        return;
    }
    
    if (enclosing) {
        enclosing->assign(name, value);
        return;
    }
    
    throw std::runtime_error("Undefined variable '" + name + "'");
}

std::shared_ptr<Environment> Environment::ancestor(int distance) const {
    std::shared_ptr<Environment> env = const_cast<Environment*>(this)->shared_from_this();
    for (int i = 0; i < distance; i++) {
        env = env->enclosing;
    }
    return env;
}

std::variant<double, std::string, bool, std::nullptr_t> Environment::getAt(int distance, const std::string& name) const {
    return ancestor(distance)->values.at(name);
}

void Environment::assignAt(int distance, const std::string& name, const std::variant<double, std::string, bool, std::nullptr_t>& value) {
    ancestor(distance)->values[name] = value;
}

} // namespace hulk