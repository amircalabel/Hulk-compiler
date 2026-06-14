// src/interpreter/Interpreter.cpp
#include "Interpreter.hpp"
#include <iostream>
#include <cmath>

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
    } catch (const ReturnException& e) {
        runtimeError(Token{}, "Can't return from top-level code.");
    }
}

void Interpreter::executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements, 
                                std::shared_ptr<Environment> newEnvironment) {
    std::shared_ptr<Environment> previous = environment;
    try {
        environment = newEnvironment;
        for (const auto& stmt : statements) {
            if (stmt) {
                execute(*stmt);
            }
        }
    } catch (...) {
        environment = previous;
        throw;
    }
    environment = previous;
}

void Interpreter::resolve(Expr& expr, int depth) {
    locals[&expr] = depth;
}

std::optional<int> Interpreter::getResolvedDepth(const Expr& expr) const {
    auto it = locals.find(&expr);
    if (it != locals.end()) {
        return it->second;
    }
    return std::nullopt;
}

backend::Value Interpreter::evaluate(Expr& expr) {
    auto result = expr.accept(*this);
    return variantToValue(result);
}

void Interpreter::execute(Stmt& stmt) {
    stmt.accept(*this);
}

backend::Value Interpreter::lookUpVariable(const Token& name, const Expr& expr) {
    auto distance = getResolvedDepth(expr);
    if (distance.has_value()) {
        return environment->getAt(distance.value(), name.lexeme);
    } else {
        return globals->get(name);
    }
}

bool Interpreter::isTruthy(const backend::Value& value) const {
    if (value.isNil()) return false;
    if (value.isBool()) return value.asBool();
    return true;
}

bool Interpreter::isEqual(const backend::Value& a, const backend::Value& b) const {
    if (a.isNil() && b.isNil()) return true;
    if (a.isNil()) return false;
    return a == b;
}

std::string Interpreter::stringify(const backend::Value& value) const {
    if (value.isNil()) return "nil";
    if (value.isBool()) return value.asBool() ? "true" : "false";
    if (value.isNumber()) {
        double num = value.asNumber();
        if (num == static_cast<int64_t>(num)) {
            return std::to_string(static_cast<int64_t>(num));
        }
        return std::to_string(num);
    }
    if (value.isObj()) {
        backend::Obj* obj = value.asObj();
        if (obj->type == backend::ObjType::OBJ_STRING) {
            return static_cast<backend::ObjString*>(obj)->chars;
        }
    }
    return value.toString();
}

backend::Value Interpreter::variantToValue(const std::variant<double, std::string, bool, std::nullptr_t>& var) {
    if (std::holds_alternative<double>(var)) {
        return backend::Value::makeNumber(std::get<double>(var));
    } else if (std::holds_alternative<std::string>(var)) {
        return backend::Value::makeObj(new backend::ObjString(std::get<std::string>(var)));
    } else if (std::holds_alternative<bool>(var)) {
        return backend::Value::makeBool(std::get<bool>(var));
    } else {
        return backend::Value::makeNil();
    }
}

void Interpreter::runtimeError(const Token& token, const std::string& message) {
    std::cerr << "[line " << token.line << "] Runtime Error: " << message << std::endl;
    throw std::runtime_error(message);
}

// ============================================================
// Statements
// ============================================================

void Interpreter::visitExpressionStmt(const ExpressionStmt& stmt) {
    if (stmt.expression) {
        lastValue = evaluate(*stmt.expression);
    }
}

void Interpreter::visitPrintStmt(const PrintStmt& stmt) {
    if (stmt.expression) {
        auto value = evaluate(*stmt.expression);
        std::cout << stringify(value) << std::endl;
    }
}

void Interpreter::visitReturnStmt(const ReturnStmt& stmt) {
    backend::Value value = backend::Value::makeNil();
    if (stmt.value) {
        value = evaluate(*stmt.value);
    }
    throw ReturnException(value);
}

void Interpreter::visitBlockStmt(const BlockStmt& stmt) {
    auto newEnvironment = std::make_shared<Environment>(environment);
    executeBlock(stmt.statements, newEnvironment);
}

void Interpreter::visitVarDeclStmt(const VarDeclStmt& stmt) {
    backend::Value value = backend::Value::makeNil();
    if (stmt.initializer) {
        value = evaluate(*stmt.initializer);
    }
    environment->define(stmt.name.lexeme, value);
}

void Interpreter::visitFunctionDeclStmt(const FunctionDeclStmt& stmt) {
    environment->define(stmt.name.lexeme, backend::Value::makeNil());
}

void Interpreter::visitClassDeclStmt(const ClassDeclStmt& stmt) {
    environment->define(stmt.name.lexeme, backend::Value::makeNil());
}

void Interpreter::visitProtocolDeclStmt(const ProtocolDeclStmt& stmt) {
    environment->define(stmt.name.lexeme, backend::Value::makeNil());
}

void Interpreter::visitMacroDeclStmt(const MacroDeclStmt& stmt) {
    environment->define(stmt.name.lexeme, backend::Value::makeNil());
}

// ============================================================
// Expressions - Versiones CORRECTAS (sin .accept() en Value)
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
            if (left.isNumber() && right.isNumber()) {
                return left.asNumber() + right.asNumber();
            }
            return stringify(left) + stringify(right);
            
        case TokenType::TOKEN_MINUS:
            if (left.isNumber() && right.isNumber()) {
                return left.asNumber() - right.asNumber();
            }
            runtimeError(expr.op, "Operands must be numbers.");
            return nullptr;
            
        case TokenType::TOKEN_STAR:
            if (left.isNumber() && right.isNumber()) {
                return left.asNumber() * right.asNumber();
            }
            runtimeError(expr.op, "Operands must be numbers.");
            return nullptr;
            
        case TokenType::TOKEN_SLASH:
            if (left.isNumber() && right.isNumber()) {
                if (right.asNumber() == 0) {
                    runtimeError(expr.op, "Division by zero.");
                    return nullptr;
                }
                return left.asNumber() / right.asNumber();
            }
            runtimeError(expr.op, "Operands must be numbers.");
            return nullptr;
            
        case TokenType::TOKEN_CARET:
            if (left.isNumber() && right.isNumber()) {
                return std::pow(left.asNumber(), right.asNumber());
            }
            runtimeError(expr.op, "Operands must be numbers.");
            return nullptr;
            
        case TokenType::TOKEN_AT:
        case TokenType::TOKEN_AT_AT:
            return stringify(left) + stringify(right);
            
        case TokenType::TOKEN_EQUAL_EQUAL:
            return isEqual(left, right);
            
        case TokenType::TOKEN_BANG_EQUAL:
            return !isEqual(left, right);
            
        case TokenType::TOKEN_GREATER:
            if (left.isNumber() && right.isNumber()) {
                return left.asNumber() > right.asNumber();
            }
            runtimeError(expr.op, "Operands must be numbers.");
            return nullptr;
            
        case TokenType::TOKEN_GREATER_EQUAL:
            if (left.isNumber() && right.isNumber()) {
                return left.asNumber() >= right.asNumber();
            }
            runtimeError(expr.op, "Operands must be numbers.");
            return nullptr;
            
        case TokenType::TOKEN_LESS:
            if (left.isNumber() && right.isNumber()) {
                return left.asNumber() < right.asNumber();
            }
            runtimeError(expr.op, "Operands must be numbers.");
            return nullptr;
            
        case TokenType::TOKEN_LESS_EQUAL:
            if (left.isNumber() && right.isNumber()) {
                return left.asNumber() <= right.asNumber();
            }
            runtimeError(expr.op, "Operands must be numbers.");
            return nullptr;
            
        default:
            runtimeError(expr.op, "Unknown operator.");
            return nullptr;
    }
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitUnaryExpr(const UnaryExpr& expr) {
    auto right = evaluate(*expr.right);
    
    switch (expr.op.type) {
        case TokenType::TOKEN_MINUS:
            if (right.isNumber()) {
                return -right.asNumber();
            }
            runtimeError(expr.op, "Operand must be a number.");
            return nullptr;
        case TokenType::TOKEN_BANG:
            return !isTruthy(right);
        default:
            runtimeError(expr.op, "Unknown operator.");
            return nullptr;
    }
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitGroupingExpr(const GroupingExpr& expr) {
    auto value = evaluate(*expr.expression);
    // Convertir Value a variant directamente
    if (value.isNumber()) return value.asNumber();
    if (value.isBool()) return value.asBool();
    if (value.isNil()) return nullptr;
    if (value.isObj()) {
        auto* obj = value.asObj();
        if (obj->type == backend::ObjType::OBJ_STRING) {
            return static_cast<backend::ObjString*>(obj)->chars;
        }
    }
    return nullptr;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitVariableExpr(const VariableExpr& expr) {
    auto value = lookUpVariable(expr.name, expr);
    if (value.isNumber()) return value.asNumber();
    if (value.isBool()) return value.asBool();
    if (value.isNil()) return nullptr;
    if (value.isObj()) {
        auto* obj = value.asObj();
        if (obj->type == backend::ObjType::OBJ_STRING) {
            return static_cast<backend::ObjString*>(obj)->chars;
        }
    }
    return nullptr;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitAssignExpr(const AssignExpr& expr) {
    auto value = evaluate(*expr.value);
    
    auto distance = getResolvedDepth(expr);
    if (distance.has_value()) {
        environment->assignAt(distance.value(), expr.name, value);
    } else {
        globals->assign(expr.name, value);
    }
    
    if (value.isNumber()) return value.asNumber();
    if (value.isBool()) return value.asBool();
    if (value.isNil()) return nullptr;
    if (value.isObj()) {
        auto* obj = value.asObj();
        if (obj->type == backend::ObjType::OBJ_STRING) {
            return static_cast<backend::ObjString*>(obj)->chars;
        }
    }
    return nullptr;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitLetExpr(const LetExpr& expr) {
    auto previous = environment;
    environment = std::make_shared<Environment>(environment);
    
    for (const auto& binding : expr.bindings) {
        backend::Value value = backend::Value::makeNil();
        if (binding.initializer) {
            value = evaluate(*binding.initializer);
        }
        environment->define(binding.name.lexeme, value);
    }
    
    auto result = evaluate(*expr.body);
    environment = previous;
    
    if (result.isNumber()) return result.asNumber();
    if (result.isBool()) return result.asBool();
    if (result.isNil()) return nullptr;
    if (result.isObj()) {
        auto* obj = result.asObj();
        if (obj->type == backend::ObjType::OBJ_STRING) {
            return static_cast<backend::ObjString*>(obj)->chars;
        }
    }
    return nullptr;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitIfExpr(const IfExpr& expr) {
    auto condition = evaluate(*expr.condition);
    
    if (isTruthy(condition)) {
        auto result = evaluate(*expr.thenBranch);
        if (result.isNumber()) return result.asNumber();
        if (result.isBool()) return result.asBool();
        if (result.isNil()) return nullptr;
        if (result.isObj()) {
            auto* obj = result.asObj();
            if (obj->type == backend::ObjType::OBJ_STRING) {
                return static_cast<backend::ObjString*>(obj)->chars;
            }
        }
        return nullptr;
    }
    
    for (const auto& elif : expr.elifBranches) {
        auto elifCond = evaluate(*elif.first);
        if (isTruthy(elifCond)) {
            auto result = evaluate(*elif.second);
            if (result.isNumber()) return result.asNumber();
            if (result.isBool()) return result.asBool();
            if (result.isNil()) return nullptr;
            if (result.isObj()) {
                auto* obj = result.asObj();
                if (obj->type == backend::ObjType::OBJ_STRING) {
                    return static_cast<backend::ObjString*>(obj)->chars;
                }
            }
            return nullptr;
        }
    }
    
    if (expr.elseBranch) {
        auto result = evaluate(*expr.elseBranch);
        if (result.isNumber()) return result.asNumber();
        if (result.isBool()) return result.asBool();
        if (result.isNil()) return nullptr;
        if (result.isObj()) {
            auto* obj = result.asObj();
            if (obj->type == backend::ObjType::OBJ_STRING) {
                return static_cast<backend::ObjString*>(obj)->chars;
            }
        }
        return nullptr;
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
    
    auto condition = expr.condition ? evaluate(*expr.condition) : backend::Value::makeBool(true);
    
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
    
    backend::Value result = backend::Value::makeNil();
    for (const auto& e : expr.expressions) {
        result = evaluate(*e);
    }
    
    environment = previous;
    
    if (result.isNumber()) return result.asNumber();
    if (result.isBool()) return result.asBool();
    if (result.isNil()) return nullptr;
    if (result.isObj()) {
        auto* obj = result.asObj();
        if (obj->type == backend::ObjType::OBJ_STRING) {
            return static_cast<backend::ObjString*>(obj)->chars;
        }
    }
    return nullptr;
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitCallExpr(const CallExpr& expr) {
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

void Environment::define(const std::string& name, const backend::Value& value) {
    values[name] = value;
}

backend::Value Environment::get(const Token& name) const {
    auto it = values.find(name.lexeme);
    if (it != values.end()) {
        return it->second;
    }
    
    if (enclosing) {
        return enclosing->get(name);
    }
    
    return backend::Value::makeNil();
}

void Environment::assign(const Token& name, const backend::Value& value) {
    auto it = values.find(name.lexeme);
    if (it != values.end()) {
        it->second = value;
        return;
    }
    
    if (enclosing) {
        enclosing->assign(name, value);
        return;
    }
}

backend::Value Environment::getAt(int distance, const std::string& name) const {
    // ISSUE-08 fix: use find() so a resolver bug doesn't throw an uncaught exception
    auto env = ancestor(distance);
    if (!env) return backend::Value::makeNil();
    auto it = env->values.find(name);
    if (it == env->values.end()) return backend::Value::makeNil();
    return it->second;
}

void Environment::assignAt(int distance, const Token& name, const backend::Value& value) {
    ancestor(distance)->values[name.lexeme] = value;
}

std::shared_ptr<Environment> Environment::ancestor(int distance) const {
    // ISSUE-09 fix: avoid const_cast and guard against null enclosing
    // (bad resolver distance would dereference nullptr without this check)
    std::shared_ptr<Environment> env = enclosing
        ? std::const_pointer_cast<Environment>(shared_from_this())
        : nullptr;
    // Use shared_from_this on the const version via const_pointer_cast
    env = std::const_pointer_cast<Environment>(shared_from_this());
    for (int i = 0; i < distance; i++) {
        if (!env || !env->enclosing) return nullptr;
        env = env->enclosing;
    }
    return env;
}

} // namespace hulk