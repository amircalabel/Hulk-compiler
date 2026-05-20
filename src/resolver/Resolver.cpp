// src/resolver/Resolver.cpp
#include "Resolver.hpp"
#include "interpreter/Interpreter.hpp"  // Pendiente
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

void Resolver::resolve(Expr& expr, int depth) {
    resolvedDepths[&expr] = depth;
}

std::optional<int> Resolver::getResolvedDepth(const Expr& expr) const {
    auto it = resolvedDepths.find(&expr);
    if (it != resolvedDepths.end()) {
        return it->second;
    }
    return std::nullopt;
}

void Resolver::beginScope() {
    scopes.push({});
}

void Resolver::endScope() {
    scopes.pop();
}

void Resolver::declare(const Token& name) {
    if (scopes.empty()) return;
    
    auto& scope = scopes.top();
    if (scope.find(name.lexeme) != scope.end()) {
        error(name, "Already a variable with this name in this scope.");
    }
    
    LocalInfo info;
    info.name = name;
    info.depth = -1;  // -1 = no inicializada aún
    info.isCaptured = false;
    scope[name.lexeme] = info;
}

void Resolver::define(const Token& name, std::shared_ptr<Type> type) {
    if (scopes.empty()) return;
    
    auto& scope = scopes.top();
    auto it = scope.find(name.lexeme);
    if (it != scope.end()) {
        it->second.depth = static_cast<int>(scopes.size());
        it->second.inferredType = type;
    }
}

void Resolver::markInitialized() {
    if (scopes.empty()) return;
    
    auto& scope = scopes.top();
    for (auto& [key, info] : scope) {
        if (info.depth == -1) {
            info.depth = static_cast<int>(scopes.size());
            break;
        }
    }
}

void Resolver::resolveLocal(const Expr& expr, const Token& name) {
    // Buscar desde el scope más interno hacia afuera
    std::stack<std::unordered_map<std::string, LocalInfo>> temp;
    int depth = static_cast<int>(scopes.size()) - 1;
    
    while (!scopes.empty()) {
        auto scope = scopes.top();
        scopes.pop();
        temp.push(scope);
        
        auto it = scope.find(name.lexeme);
        if (it != scope.end()) {
            // Encontramos la variable
            resolve(expr, depth);
            
            // Restaurar scopes
            while (!temp.empty()) {
                scopes.push(temp.top());
                temp.pop();
            }
            return;
        }
        depth--;
    }
    
    // Restaurar scopes si no se encontró
    while (!temp.empty()) {
        scopes.push(temp.top());
        temp.pop();
    }
    
    // No se encontró - es global (no necesita resolución)
}

int Resolver::resolveUpvalue(Resolver& enclosing, const Token& name) {
    // Buscar en la función que nos envuelve
    // Implementación para closures (sección A.13.3, capítulo 25 del libro)
    // Por ahora retornamos -1 (no es upvalue)
    return -1;
}

void Resolver::resolveFunction(const FunctionDeclStmt& function, FunctionType type) {
    FunctionType enclosingFunction = currentFunction;
    currentFunction = type;
    
    beginScope();
    
    // Parámetros de la función
    for (const auto& param : function.parameters) {
        declare(param.name);
        define(param.name, nullptr);  // Tipo será inferido después
    }
    
    // Resolver el cuerpo de la función
    for (const auto& stmt : function.body) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
    
    endScope();
    currentFunction = enclosingFunction;
}

// ============================================================
// Visitadores para Statements
// ============================================================

void Resolver::visitExpressionStmt(const ExpressionStmt& stmt) {
    if (stmt.expression) {
        stmt.expression->accept(*this);
    }
}

void Resolver::visitPrintStmt(const PrintStmt& stmt) {
    if (stmt.expression) {
        stmt.expression->accept(*this);
    }
}

void Resolver::visitReturnStmt(const ReturnStmt& stmt) {
    if (currentFunction == FunctionType::NONE) {
        error(stmt.keyword, "Can't return from top-level code.");
    }
    
    if (stmt.value) {
        if (currentFunction == FunctionType::INITIALIZER) {
            error(stmt.keyword, "Can't return a value from an initializer.");
        }
        stmt.value->accept(*this);
    }
}

void Resolver::visitBlockStmt(const BlockStmt& stmt) {
    beginScope();
    for (const auto& s : stmt.statements) {
        if (s) {
            s->accept(*this);
        }
    }
    endScope();
}

void Resolver::visitVarDeclStmt(const VarDeclStmt& stmt) {
    declare(stmt.name);
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    }
    define(stmt.name);
}

void Resolver::visitFunctionDeclStmt(const FunctionDeclStmt& stmt) {
    declare(stmt.name);
    define(stmt.name);
    resolveFunction(stmt, FunctionType::FUNCTION);
}

void Resolver::visitClassDeclStmt(const ClassDeclStmt& stmt) {
    ClassType enclosingClass = currentClass;
    
    if (stmt.superclass.type != TokenType::TOKEN_ERROR) {
        currentClass = ClassType::SUBCLASS;
        resolveLocal(*stmt.superclass.lexeme, stmt.superclass);
    } else {
        currentClass = ClassType::CLASS;
    }
    
    declare(stmt.name);
    define(stmt.name);
    
    // Resolver métodos
    beginScope();
    
    // Añadir 'self' al scope (sección A.7.1)
    Token selfToken;
    selfToken.type = TokenType::TOKEN_SELF;
    selfToken.lexeme = "self";
    declare(selfToken);
    define(selfToken);
    
    // Si es subclase, añadir 'base' (sección A.7.4)
    if (currentClass == ClassType::SUBCLASS) {
        Token baseToken;
        baseToken.type = TokenType::TOKEN_BASE;
        baseToken.lexeme = "base";
        declare(baseToken);
        define(baseToken);
    }
    
    // Resolver métodos
    for (const auto& method : stmt.methods) {
        FunctionType type = FunctionType::METHOD;
        if (method->name.lexeme == "init") {
            type = FunctionType::INITIALIZER;
        }
        resolveFunction(*method, type);
    }
    
    endScope();
    currentClass = enclosingClass;
}

void Resolver::visitProtocolDeclStmt(const ProtocolDeclStmt& stmt) {
    // Los protocolos no crean scopes en tiempo de ejecución
    // Solo registramos que existe
    declare(stmt.name);
    define(stmt.name);
}

void Resolver::visitMacroDeclStmt(const MacroDeclStmt& stmt) {
    // Las macros se expanden en tiempo de compilación
    // No necesitan resolución de ámbito en tiempo de ejecución
    declare(stmt.name);
    define(stmt.name);
    
    if (stmt.body) {
        stmt.body->accept(*this);
    }
}

// ============================================================
// Visitadores para Expresiones
// ============================================================

std::variant<double, std::string, bool, std::nullptr_t> Resolver::visitLiteralExpr(const LiteralExpr& expr) {
    // Los literales no requieren resolución
    return expr.value;
}

std::variant<double, std::string, bool, std::nullptr_t> Resolver::visitBinaryExpr(const BinaryExpr& expr) {
    if (expr.left) expr.left->accept(*this);
    if (expr.right) expr.right->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> Resolver::visitUnaryExpr(const UnaryExpr& expr) {
    if (expr.right) expr.right->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> Resolver::visitGroupingExpr(const GroupingExpr& expr) {
    if (expr.expression) expr.expression->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> Resolver::visitVariableExpr(const VariableExpr& expr) {
    // Verificar que no se use una variable antes de su inicialización (sección A.4)
    if (!scopes.empty()) {
        auto& scope = scopes.top();
        auto it = scope.find(expr.name.lexeme);
        if (it != scope.end() && it->second.depth == -1) {
            error(expr.name, "Can't read local variable in its own initializer.");
        }
    }
    
    resolveLocal(expr, expr.name);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> Resolver::visitAssignExpr(const AssignExpr& expr) {
    if (expr.value) expr.value->accept(*this);
    resolveLocal(expr, expr.name);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> Resolver::visitLetExpr(const LetExpr& expr) {
    beginScope();
    
    for (const auto& binding : expr.bindings) {
        declare(binding.name);
        if (binding.initializer) {
            binding.initializer->accept(*this);
        }
        define(binding.name);
    }
    
    if (expr.body) expr.body->accept(*this);
    endScope();
    
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> Resolver::visitIfExpr(const IfExpr& expr) {
    if (expr.condition) expr.condition->accept(*this);
    if (expr.thenBranch) expr.thenBranch->accept(*this);
    
    for (const auto& elif : expr.elifBranches) {
        if (elif.first) elif.first->accept(*this);
        if (elif.second) elif.second->accept(*this);
    }
    
    if (expr.elseBranch) expr.elseBranch->accept(*this);
    
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> Resolver::visitWhileExpr(const WhileExpr& expr) {
    if (expr.condition) expr.condition->accept(*this);
    if (expr.body) expr.body->accept(*this);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> Resolver::visitForExpr(const ForExpr& expr) {
    beginScope();
    
    if (expr.initializer) expr.initializer->accept(*this);
    if (expr.condition) expr.condition->accept(*this);
    if (expr.increment) expr.increment->accept(*this);
    if (expr.body) expr.body->accept(*this);
    
    endScope();
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> Resolver::visitBlockExpr(const BlockExpr& expr) {
    beginScope();
    
    for (const auto& e : expr.expressions) {
        if (e) e->accept(*this);
    }
    
    endScope();
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> Resolver::visitCallExpr(const CallExpr& expr) {
    if (expr.callee) expr.callee->accept(*this);
    
    for (const auto& arg : expr.arguments) {
        if (arg) arg->accept(*this);
    }
    
    return 0.0;
}

void Resolver::validateSelfUsage(const Token& token) {
    if (currentClass == ClassType::NONE) {
        error(token, "Can't use 'self' outside of a class.");
    }
}

void Resolver::validateBaseUsage(const Token& token) {
    if (currentClass != ClassType::SUBCLASS) {
        error(token, "Can't use 'base' in a class with no superclass.");
    }
}

void Resolver::validateReturnType(const ReturnStmt& stmt) {
    // Verificar compatibilidad de tipos según A.8.2
    // Implementación pendiente del Type Checker
}

void Resolver::printScope() const {
    std::cerr << "Current scopes:" << std::endl;
    std::stack<std::unordered_map<std::string, LocalInfo>> temp = scopes;
    int depth = 0;
    while (!temp.empty()) {
        auto scope = temp.top();
        temp.pop();
        std::cerr << "  Depth " << depth++ << ": ";
        for (const auto& [name, info] : scope) {
            std::cerr << name << " ";
        }
        std::cerr << std::endl;
    }
}

} // namespace hulk