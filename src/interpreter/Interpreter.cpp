// src/interpreter/Interpreter.cpp
#include "Interpreter.hpp"
#include <cmath>
#include <sstream>

namespace hulk {

// Simple exception type to handle returns from functions
class ReturnException : public std::exception {
public:
    explicit ReturnException(std::variant<double, std::string, bool, std::nullptr_t> value) : value(std::move(value)) {}
    const char* what() const noexcept override { return "Return"; }
    std::variant<double, std::string, bool, std::nullptr_t> value;
};

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
    if (!stmt.expression) return;

    auto value = [&]() -> std::variant<double, std::string, bool, std::nullptr_t> {
        if (auto grouping = dynamic_cast<GroupingExpr*>(stmt.expression.get())) {
            return evaluate(*grouping->expression);
        }
        return evaluate(*stmt.expression);
    }();

    std::cout << stringify(value) << std::endl;
}

void Interpreter::visitReturnStmt(const ReturnStmt& stmt) {
    std::variant<double, std::string, bool, std::nullptr_t> value = nullptr;
    if (stmt.value) {
        value = evaluate(*stmt.value);
    }
    throw ReturnException(value);
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
    auto fn = std::make_shared<FunctionObject>();
    fn->decl = &stmt;
    fn->closure = environment;
    environment->defineFunction(stmt.name.lexeme, fn);
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
    // For now, simplified
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
    auto it = locals.find(&expr);
    if (it != locals.end()) {
        int distance = it->second;
        return environment->getAt(distance, expr.name.lexeme);
    }
    return environment->get(expr.name.lexeme);
}

std::variant<double, std::string, bool, std::nullptr_t> 
Interpreter::visitAssignExpr(const AssignExpr& expr) {
    auto value = evaluate(*expr.value);
    auto it = locals.find(&expr);
    if (it != locals.end()) {
        environment->assignAt(it->second, expr.name.lexeme, value);
        return value;
    }
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
    for (const auto& elif : expr.elifBranches) {
        if (elif.first) {
            auto cond = evaluate(*elif.first);
            if (isTruthy(cond)) return evaluate(*elif.second);
        }
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
    // If callee is a variable and matches \"print\", do builtin
    if (auto var = dynamic_cast<VariableExpr*>(expr.callee.get())) {
        if (var->name.lexeme == "print") {
            if (!expr.arguments.empty()) {
                auto val = evaluate(*expr.arguments[0]);
                std::cout << stringify(val) << std::endl;
            } else {
                std::cout << std::endl;
            }
            return nullptr;
        }
        // user-defined function lookup
        auto fn = environment->getFunction(var->name.lexeme);
        if (fn) {
            std::vector<std::variant<double, std::string, bool, std::nullptr_t>> args;
            for (const auto& a : expr.arguments) args.push_back(evaluate(*a));
            return callFunction(fn, args);
        }
    }

    // Otherwise evaluate callee and args for side-effects
    evaluate(*expr.callee);
    for (const auto& arg : expr.arguments) {
        evaluate(*arg);
    }
    return nullptr;
}

// ============================================================
// Function call helper
// ============================================================

std::variant<double, std::string, bool, std::nullptr_t>
Interpreter::callFunction(const std::shared_ptr<FunctionObject>& fn,
                          const std::vector<std::variant<double, std::string, bool, std::nullptr_t>>& args) {
    auto previous = environment;
    // New environment enclosing the function's closure
    environment = std::make_shared<Environment>(fn->closure);
    // Bind parameters
    const auto& params = fn->decl->parameters;
    if (args.size() != params.size()) {
        runtimeError("Arity mismatch when calling function '" + fn->decl->name.lexeme + "'");
    }
    for (size_t i = 0; i < params.size(); ++i) {
        environment->define(params[i].name.lexeme, args[i]);
    }

    try {
        for (const auto& s : fn->decl->body) {
            if (s) execute(*s);
        }
    } catch (const ReturnException& ret) {
        environment = previous;
        return ret.value;
    }

    environment = previous;
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

void Environment::defineFunction(const std::string& name, std::shared_ptr<FunctionObject> fn) {
    functions[name] = fn;
}

std::shared_ptr<FunctionObject> Environment::getFunction(const std::string& name) const {
    auto it = functions.find(name);
    if (it != functions.end()) return it->second;
    if (enclosing) return enclosing->getFunction(name);
    return nullptr;
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
    // Walk up `distance` times starting from this environment
    auto env = const_cast<Environment*>(this)->shared_from_this();
    for (int i = 0; i < distance; ++i) {
        if (!env->enclosing) return nullptr;
        env = env->enclosing;
    }
    return env;
}

std::variant<double, std::string, bool, std::nullptr_t> Environment::getAt(int distance, const std::string& name) const {
    auto env = ancestor(distance);
    if (!env) return nullptr;
    auto it = env->values.find(name);
    if (it == env->values.end()) return nullptr;
    return it->second;
}

void Environment::assignAt(int distance, const std::string& name, const std::variant<double, std::string, bool, std::nullptr_t>& value) {
    auto env = ancestor(distance);
    if (!env) throw std::runtime_error("AssignAt: invalid distance");
    env->values[name] = value;
}

} // namespace hulk
