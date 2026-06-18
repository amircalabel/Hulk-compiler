// src/backend/CodeGenerator.hpp
#ifndef HULK_CODE_GENERATOR_HPP
#define HULK_CODE_GENERATOR_HPP

#include <string>
#include <vector>
#include <memory>
#include "ast/Stmt.hpp"
#include "ast/Expr.hpp"
#include "scanner/Token.hpp"

namespace hulk::backend {

class CodeGenerator {
public:
    bool generate(const std::vector<std::unique_ptr<Stmt>>& statements, const std::string& outputPath);
    
private:
    std::string generateStatement(const Stmt& stmt);
    std::string generateExpression(const Expr& expr);
    std::string literalToValueExpr(const std::variant<double, std::string, bool, std::nullptr_t>& value);
    std::string literalToString(const std::variant<double, std::string, bool, std::nullptr_t>& value);
    std::string escapeString(const std::string& str);
    bool compileToBinary(const std::string& sourcePath, const std::string& outputPath);
};

} // namespace hulk::backend

#endif