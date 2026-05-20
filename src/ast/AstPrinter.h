// src/ast/AstPrinter.hpp
#ifndef HULK_AST_PRINTER_H
#define HULK_AST_PRINTER_H

#include <string>
#include <sstream>
#include "Expr.hpp"
#include "Stmt.hpp"

/**
 * AstPrinter - Visitor pattern para imprimir el AST en formato legible.
 * 
 * Este es un debugging tool que muestra la estructura del árbol sintáctico
 * en un formato similar a Lisp, útil para verificar que el parser está
 * generando el AST correcto.
 * 
 * Ejemplo de salida:
 *   (+ 1 (* 2 3))                    para 1 + 2 * 3
 *   (let (x 42) (print x))           para let x = 42 in print x
 *   (if (cond) (then) (elif ...) (else ...))
 */
class AstPrinter : public ExprVisitor, public StmtVisitor {
public:
    // Puntos de entrada principales
    std::string print(const Expr& expr);
    std::string print(const Stmt& stmt);
    std::string print(const std::vector<std::unique_ptr<Stmt>>& statements);

    // ============================================================
    // Visitadores para Expresiones (ExprVisitor)
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

    // ============================================================
    // Visitadores para Statements (StmtVisitor)
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

private:
    std::stringstream output;
    int indentLevel = 0;
    
    // Helpers
    void indent();
    void parenthesize(const std::string& name, const std::vector<std::string>& parts);
    std::string literalToString(const std::variant<double, std::string, bool, std::nullptr_t>& value);
    std::string tokenToString(const Token& token);
};

#endif // HULK_AST_PRINTER_HPP