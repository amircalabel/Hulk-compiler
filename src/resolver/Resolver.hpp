// src/resolver/Resolver.hpp
#ifndef HULK_RESOLVER_HPP
#define HULK_RESOLVER_HPP

#include <stack>
#include <unordered_map>
#include <memory>
#include <vector>
#include "ast/Expr.hpp"
#include "ast/Stmt.hpp"
#include "type/Type.hpp"

namespace hulk {

// Tipos de función (para verificar returns, this, etc.)
enum class FunctionType {
    NONE,           // No estamos en una función
    FUNCTION,       // Función normal
    INITIALIZER,    // Inicializador de clase (init)
    METHOD          // Método de clase
};

// Tipos de clase (para verificar super, this, etc.)
enum class ClassType {
    NONE,           // No estamos en una clase
    CLASS,          // Clase normal
    SUBCLASS        // Subclase (tiene superclase)
};

// Información de una variable local durante la resolución
struct LocalInfo {
    Token name;
    int depth;           // Profundidad del scope (-1 = no inicializada)
    bool isCaptured;     // Si es capturada por un closure
    std::shared_ptr<Type> inferredType;  // Tipo inferido (opcional)
};

// Forward declaration
class Interpreter;  // Lo implementaremos después en el backend

/**
 * Resolver - Análisis de ámbito (capítulo 11 del libro)
 * 
 * Responsabilidades:
 * 1. Resolver qué declaración se corresponde con cada uso de variable
 * 2. Detectar errores de ámbito (variables no definidas, usos antes de inicialización)
 * 3. Capturar variables para closures (upvalues)
 * 4. Anotar el AST con la distancia de resolución
 */
class Resolver : public ExprVisitor, public StmtVisitor {
public:
    explicit Resolver(Interpreter& interpreter);
    
    // Punto de entrada
    void resolve(const std::vector<std::unique_ptr<Stmt>>& statements);
    
    // Almacenar resolución para una expresión
    void resolve(Expr& expr, int depth);
    
    // Obtener la distancia resuelta para una expresión
    std::optional<int> getResolvedDepth(const Expr& expr) const;
    
    // ============================================================
    // Visitadores para Statements (resolución de ámbito)
    // ============================================================
    void visitExpressionStmt(const ExpressionStmt& stmt) override;
    void visitPrintStmt(const PrintStmt& stmt) override;
    void visitReturnStmt(const ReturnStmt& stmt) override;
    void visitBlockStmt(const BlockStmt& stmt) override;
    void visitVarDeclStmt(const VarDeclStmt& stmt) override;
    void visitFunctionDeclStmt(const FunctionDeclStmt& stmt) override;
    void visitClassDeclStmt(const ClassDeclStmt& stmt) override;
    void visitProtocolDeclStmt(const ProtocolDeclStmt& stmt) override;
    void visitMacroDeclStmt(const MacroDeclStmt& stmt) override;
    
    // ============================================================
    // Visitadores para Expresiones
    // ============================================================
    std::variant<double, std::string, bool, std::nullptr_t> visitLiteralExpr(const LiteralExpr& expr) override;
    std::variant<double, std::string, bool, std::nullptr_t> visitBinaryExpr(const BinaryExpr& expr) override;
    std::variant<double, std::string, bool, std::nullptr_t> visitUnaryExpr(const UnaryExpr& expr) override;
    std::variant<double, std::string, bool, std::nullptr_t> visitGroupingExpr(const GroupingExpr& expr) override;
    std::variant<double, std::string, bool, std::nullptr_t> visitVariableExpr(const VariableExpr& expr) override;
    std::variant<double, std::string, bool, std::nullptr_t> visitAssignExpr(const AssignExpr& expr) override;
    std::variant<double, std::string, bool, std::nullptr_t> visitLetExpr(const LetExpr& expr) override;
    std::variant<double, std::string, bool, std::nullptr_t> visitIfExpr(const IfExpr& expr) override;
    std::variant<double, std::string, bool, std::nullptr_t> visitWhileExpr(const WhileExpr& expr) override;
    std::variant<double, std::string, bool, std::nullptr_t> visitForExpr(const ForExpr& expr) override;
    std::variant<double, std::string, bool, std::nullptr_t> visitBlockExpr(const BlockExpr& expr) override;
    std::variant<double, std::string, bool, std::nullptr_t> visitCallExpr(const CallExpr& expr) override;

private:
    Interpreter& interpreter;
    std::stack<std::unordered_map<std::string, LocalInfo>> scopes;
    FunctionType currentFunction = FunctionType::NONE;
    ClassType currentClass = ClassType::NONE;
    
    // Mapa de resoluciones (expresión -> distancia en la pila de scopes)
    std::unordered_map<const Expr*, int> resolvedDepths;
    
    // Mapa de tipos inferidos (expresión -> tipo)
    std::unordered_map<const Expr*, std::shared_ptr<Type>> inferredTypes;
    
    // Helpers
    void beginScope();
    void endScope();
    void declare(const Token& name);
    void define(const Token& name, std::shared_ptr<Type> type = nullptr);
    void resolveLocal(const Expr& expr, const Token& name);
    int resolveUpvalue(Resolver& enclosing, const Token& name);
    void resolveFunction(const FunctionDeclStmt& function, FunctionType type);
    void markInitialized();
    
    // Validaciones específicas de HULK
    void validateSelfUsage(const Token& token);
    void validateBaseUsage(const Token& token);
    void validateReturnType(const ReturnStmt& stmt);
    
    // Para debugging
    void printScope() const;
};

} // namespace hulk

#endif // HULK_RESOLVER_HPP