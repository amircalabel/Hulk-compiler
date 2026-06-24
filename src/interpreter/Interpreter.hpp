// src/interpreter/Interpreter.hpp
#ifndef HULK_INTERPRETER_HPP
#define HULK_INTERPRETER_HPP

#include <memory>
#include <vector>
#include <unordered_map>
#include <variant>
#include <iostream>
#include "ast/Expr.hpp"
#include "ast/Stmt.hpp"

namespace hulk {

// Forward declarations
class Environment;
struct FunctionObject;

// ============================================================
// Interpreter - Evalúa el AST (tree-walk interpreter)
// ============================================================
class Interpreter : public ExprVisitor, public StmtVisitor {
public:
    Interpreter();
    ~Interpreter();
    
    // Punto de entrada principal
    void interpret(const std::vector<std::unique_ptr<Stmt>>& statements);
    
    // Para el Resolver (opcional)
    void resolve(Expr& expr, int depth);
    
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
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment;
    std::unordered_map<const Expr*, int> locals; // resolution depth map
    
    // Helpers
    void execute(Stmt& stmt);
    std::variant<double, std::string, bool, std::nullptr_t> evaluate(Expr& expr);
    bool isTruthy(const std::variant<double, std::string, bool, std::nullptr_t>& value) const;
    bool isEqual(const std::variant<double, std::string, bool, std::nullptr_t>& a,
                 const std::variant<double, std::string, bool, std::nullptr_t>& b) const;
    std::string stringify(const std::variant<double, std::string, bool, std::nullptr_t>& value) const;
    
    void runtimeError(const std::string& message);

    // function call helper
    std::variant<double, std::string, bool, std::nullptr_t> callFunction(const std::shared_ptr<FunctionObject>& fn,
                                                                          const std::vector<std::variant<double, std::string, bool, std::nullptr_t>>& args);
};

// ============================================================
// Environment - Entorno de variables
// ============================================================
class Environment : public std::enable_shared_from_this<Environment> {
public:
    explicit Environment(std::shared_ptr<Environment> enclosing = nullptr);
    
    void define(const std::string& name, const std::variant<double, std::string, bool, std::nullptr_t>& value);
    std::variant<double, std::string, bool, std::nullptr_t> get(const std::string& name) const;
    void assign(const std::string& name, const std::variant<double, std::string, bool, std::nullptr_t>& value);
    
    // for resolver: ancestor/getAt/assignAt
    std::shared_ptr<Environment> ancestor(int distance) const;
    std::variant<double, std::string, bool, std::nullptr_t> getAt(int distance, const std::string& name) const;
    void assignAt(int distance, const std::string& name, const std::variant<double, std::string, bool, std::nullptr_t>& value);

    // function storage (user-defined functions)
    void defineFunction(const std::string& name, std::shared_ptr<FunctionObject> fn);
    std::shared_ptr<FunctionObject> getFunction(const std::string& name) const;

private:
    std::shared_ptr<Environment> enclosing;
    std::unordered_map<std::string, std::variant<double, std::string, bool, std::nullptr_t>> values;
    std::unordered_map<std::string, std::shared_ptr<FunctionObject>> functions;
};

// Minimal FunctionObject (closure)
struct FunctionObject {
    const FunctionDeclStmt* decl = nullptr;
    std::shared_ptr<Environment> closure;
};

} // namespace hulk

#endif // HULK_INTERPRETER_HPP
