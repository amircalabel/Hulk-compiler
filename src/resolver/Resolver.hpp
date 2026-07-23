// src/resolver/Resolver.hpp
#ifndef HULK_RESOLVER_HPP
#define HULK_RESOLVER_HPP

#include <memory>
#include <vector>
#include <unordered_map>
#include "ast/Expr.hpp"
#include "ast/Stmt.hpp"

namespace hulk {

// Forward declaration
class Interpreter;

// Usar nombres diferentes para evitar conflictos
enum class ResolverFunctionType {
    NONE,
    FUNCTION,
    INITIALIZER,
    METHOD
};

enum class ResolverClassType {
    NONE,
    CLASS,
    SUBCLASS
};

class Resolver : public ExprVisitor, public StmtVisitor {
public:
    explicit Resolver(Interpreter& interpreter);
    
    void resolve(const std::vector<std::unique_ptr<Stmt>>& statements);
    
    // Visitadores para Statements
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
    
    // Visitadores para Expresiones
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
    // Use vector so we can compute distances to scopes easily (index 0 = outermost)
    std::vector<std::unordered_map<std::string, bool>> scopes;
    ResolverFunctionType currentFunction = ResolverFunctionType::NONE;
    ResolverClassType currentClass = ResolverClassType::NONE;
    
    void beginScope();
    void endScope();
    void declare(const Token& name);
    void define(const Token& name);
    void resolveLocal(const Expr& expr, const Token& name);
};

} // namespace hulk

#endif // HULK_RESOLVER_HPP
