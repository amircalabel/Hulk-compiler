// src/backend/CodeGenerator.hpp
#ifndef HULK_CODE_GENERATOR_HPP
#define HULK_CODE_GENERATOR_HPP

#include <string>
#include <vector>
#include <memory>
#include <set>
#include <map>
#include "ast/Stmt.hpp"
#include "ast/Expr.hpp"
#include "scanner/Token.hpp"

namespace hulk::backend {

class CodeGenerator {
public:
    // Analiza semánticamente el programa. Devuelve false y reporta (por
    // semanticError) si encuentra un error semántico.
    bool checkSemantics(const std::vector<std::unique_ptr<Stmt>>& statements);

    // Transpila el AST a C++, lo compila y produce el binario en outputPath.
    bool generate(const std::vector<std::unique_ptr<Stmt>>& statements, const std::string& outputPath);

private:
    int counter = 0;
    std::set<std::string> functionNames;
    std::set<std::string> classNames;
    bool semanticOk = true;

    std::string fresh(const std::string& prefix);

    // Generación
    std::string genStmt(const Stmt& stmt, const std::string& env);
    std::string genStmts(const std::vector<std::unique_ptr<Stmt>>& stmts, const std::string& env);
    std::string genExpr(const Expr& expr, const std::string& env);
    std::string genFunction(const FunctionDeclStmt& fn);
    std::string genClass(const ClassDeclStmt& cls);

    // Análisis semántico
    void collectDeclarations(const std::vector<std::unique_ptr<Stmt>>& statements);
    void checkStmt(const Stmt& stmt);
    void checkExpr(const Expr& expr);

    // Utilidades
    std::string literalToValueExpr(const std::variant<double, std::string, bool, std::nullptr_t>& value);
    std::string escapeString(const std::string& str);
    bool compileToBinary(const std::string& sourcePath, const std::string& outputPath);
    static bool isBuiltin(const std::string& name);
};

} // namespace hulk::backend

#endif
