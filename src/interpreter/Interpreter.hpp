// src/interpreter/Interpreter.hpp
#ifndef HULK_INTERPRETER_HPP
#define HULK_INTERPRETER_HPP

#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <stdexcept> 
#include "ast/Expr.hpp"
#include "ast/Stmt.hpp"
#include "backend/vm/Value.hpp"
#include "backend/vm/VM.hpp"

namespace hulk {

// Forward declarations
class Environment;

// ============================================================
// Interpreter - Ejecuta el AST (tree-walk interpreter)
// ============================================================
class Interpreter : public ExprVisitor, public StmtVisitor {
public:
    Interpreter();
    ~Interpreter();
    
    void interpret(const std::vector<std::unique_ptr<Stmt>>& statements);
    void executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements, 
                      std::shared_ptr<Environment> environment);
    
    void resolve(Expr& expr, int depth);
    std::optional<int> getResolvedDepth(const Expr& expr) const;
    
    backend::Value getLastValue() const { return lastValue; }
    
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
    
    // ============================================================
    // Visitadores para Expresiones - Retornan variant, no Value
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
    std::unordered_map<const Expr*, int> locals;
    backend::Value lastValue;
    
    bool hasReturnValue = false;
    backend::Value returnValue;
    
    backend::Value evaluate(Expr& expr);
    void execute(Stmt& stmt);
    backend::Value lookUpVariable(const Token& name, const Expr& expr);
    bool isTruthy(const backend::Value& value) const;
    bool isEqual(const backend::Value& a, const backend::Value& b) const;
    std::string stringify(const backend::Value& value) const;
    
    void runtimeError(const Token& token, const std::string& message);
    
    // Convertir variant a Value
    backend::Value variantToValue(const std::variant<double, std::string, bool, std::nullptr_t>& var);
};

// ============================================================
// Environment
// ============================================================
class Environment : public std::enable_shared_from_this<Environment> {
public:
    explicit Environment(std::shared_ptr<Environment> enclosing = nullptr);
    
    void define(const std::string& name, const backend::Value& value);
    backend::Value get(const Token& name) const;
    void assign(const Token& name, const backend::Value& value);
    backend::Value getAt(int distance, const std::string& name) const;
    void assignAt(int distance, const Token& name, const backend::Value& value);
    std::shared_ptr<Environment> ancestor(int distance) const;
    
private:
    std::shared_ptr<Environment> enclosing;
    std::unordered_map<std::string, backend::Value> values;
};

// ============================================================
// Excepción para return
// ============================================================
class ReturnException : public std::runtime_error {
public:
    backend::Value value;
    explicit ReturnException(const backend::Value& val)
        : std::runtime_error("return"), value(val) {}
};

} // namespace hulk

#endif // HULK_INTERPRETER_HPP