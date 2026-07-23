// src/inferer/TypeInferer.hpp
#ifndef HULK_TYPE_INFERER_HPP
#define HULK_TYPE_INFERER_HPP

#include <memory>
#include <unordered_map>
#include <vector>
#include "ast/Expr.hpp"
#include "ast/Stmt.hpp"
#include "type/Type.hpp"
#include "resolver/Resolver.hpp"

namespace hulk {

/**
 * TypeInferer - Inferencia de tipos para HULK (sección A.9)
 * 
 * Responsabilidades:
 * 1. Inferir tipos de expresiones según reglas A.9.2
 * 2. Inferir tipos de símbolos no anotados según reglas A.9.3
 * 3. Sintetizar protocolos cuando sea necesario (A.9.5)
 * 4. Unificar tipos y resolver variables de tipo
 */
class TypeInferer : public ExprVisitor, public StmtVisitor {
public:
    explicit TypeInferer(Resolver& resolver);
    
    // Punto de entrada
    void infer(const std::vector<std::unique_ptr<Stmt>>& statements);
    
    // Obtener tipo inferido para una expresión
    std::shared_ptr<Type> getType(const Expr& expr) const;
    
    // ============================================================
    // Visitadores para Statements
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
    void visitIfStmt(const IfStmt& stmt) override;
    void visitWhileStmt(const WhileStmt& stmt) override;
    void visitForStmt(const ForStmt& stmt) override;
    
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
    Resolver& resolver;
    
    // Tabla de símbolos con tipos inferidos
    struct SymbolInfo {
        Token name;
        std::shared_ptr<Type> declaredType;  // Anotación explícita (si existe)
        std::shared_ptr<Type> inferredType;  // Tipo inferido
        bool isInitialized;
    };
    
    std::unordered_map<std::string, SymbolInfo> symbolTable;
    std::unordered_map<const Expr*, std::shared_ptr<Type>> exprTypes;
    
    // Reglas de inferencia (sección A.9.2)
    std::shared_ptr<Type> inferLiteral(const LiteralExpr& expr);
    std::shared_ptr<Type> inferBinary(const BinaryExpr& expr);
    std::shared_ptr<Type> inferUnary(const UnaryExpr& expr);
    std::shared_ptr<Type> inferIf(const IfExpr& expr);
    std::shared_ptr<Type> inferBlock(const BlockExpr& expr);
    std::shared_ptr<Type> inferLet(const LetExpr& expr);
    std::shared_ptr<Type> inferCall(const CallExpr& expr);
    
    // Encontrar el ancestro común más bajo (LCA) para tipos (A.9.2 - if)
    std::shared_ptr<Type> lowestCommonAncestor(const std::vector<std::shared_ptr<Type>>& types);
    
    // Sintetizar protocolos (A.9.5)
    std::shared_ptr<Type> synthesizeProtocol(const std::string& baseName, 
                                              const std::vector<std::pair<std::string, std::shared_ptr<Type>>>& requiredMethods);
    
    // Unificación de tipos (para variables de tipo)
    bool unify(std::shared_ptr<Type>& a, std::shared_ptr<Type>& b);
    
    // Sustitución de variables de tipo
    std::shared_ptr<Type> substitute(const Type& type, 
                                      const std::unordered_map<std::string, std::shared_ptr<Type>>& substitutions);
    
    // Validaciones
    void validateType(const Expr& expr, std::shared_ptr<Type> expected);
    void reportTypeError(const Token& token, const std::string& expected, const std::string& found);
};

} // namespace hulk

#endif // HULK_TYPE_INFERER_HPP
