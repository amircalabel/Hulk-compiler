// src/backend/banner/BannerGenerator.hpp
#ifndef HULK_BANNER_GENERATOR_HPP
#define HULK_BANNER_GENERATOR_HPP

#include <memory>
#include <stack>
#include <unordered_map>
#include "backend/banner/BannerIR.hpp"
#include "ast/Expr.hpp"
#include "ast/Stmt.hpp"
#include "resolver/Resolver.hpp"
#include "inferer/TypeInferer.hpp"

namespace hulk::backend {

/**
 * BannerGenerator - Traduce el AST a BANNER IR
 * 
 * Basado en:
 * - Sección B del PDF de HULK (BANNER Intermediate Representation)
 * - Capítulos 14-17, 21-25 de Crafting Interpreters (compilación a bytecode)
 */
class BannerGenerator : public ExprVisitor, public StmtVisitor {
public:
    explicit BannerGenerator(Resolver& resolver, TypeInferer& inferer);
    
    // Generar programa BANNER completo
    BannerProgram generate(const std::vector<std::unique_ptr<Stmt>>& statements);
    
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
    TypeInferer& inferer;
    
    // Programa BANNER en construcción
    BannerProgram program;
    
    // Contexto actual de compilación
    struct CompilerContext {
        BannerFunction* currentFunction = nullptr;
        std::unordered_map<std::string, int> localSlots;  // var -> slot index
        std::unordered_map<std::string, int> paramSlots;  // param -> slot index
        std::unordered_map<std::string, std::string> upvalues;  // var -> upvalue label
        int nextLocalSlot = 0;
        int nextParamSlot = 0;
        
        int scopeDepth = 0;
        int scopeSize = 0;
        
        // Labels para control flow
        int labelCounter = 0;
        std::stack<std::string> breakLabels;
        std::stack<std::string> continueLabels;
    };
    
    CompilerContext ctx;
    
    // Helpers
    void emit(const BannerInstr& instr);
    void emitLabel(const std::string& name);
    std::string newLabel();
    
    void beginFunction(const std::string& name, const std::vector<std::string>& params);
    void endFunction();
    
    void declareLocal(const std::string& name);
    void declareParameter(const std::string& name);
    
    int getLocalSlot(const std::string& name);
    void emitLoadLocal(const std::string& name);
    void emitStoreLocal(const std::string& name);
    
    void emitLoadConstant(double value);
    void emitLoadConstant(const std::string& stringValue);
    void emitLoadConstant(bool value);
    void emitLoadConstant(std::nullptr_t);
    
    // Generación de .TYPES
    void generateTypeLayout(const ClassDeclStmt& stmt);
    void generateVTable(const ClassDeclStmt& stmt);
    
    // Generación de .DATA
    std::string internString(const std::string& str);
    
    // Manejo de errores
    void error(const Token& token, const std::string& message);

    void beginScope();
    void endScope();
};

} // namespace hulk::backend

#endif // HULK_BANNER_GENERATOR_HPP