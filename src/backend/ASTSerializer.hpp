// src/backend/ASTSerializer.hpp
#ifndef HULK_AST_SERIALIZER_HPP
#define HULK_AST_SERIALIZER_HPP

#include <string>
#include <vector>
#include <memory>
#include "ast/Expr.hpp"
#include "ast/Stmt.hpp"

namespace hulk::backend {

class ASTSerializer {
public:
    std::string serialize(const std::vector<std::unique_ptr<Stmt>>& statements);
    
private:
    std::string serializeStmt(const Stmt& stmt);
    std::string serializeExpr(const Expr& expr);
    std::string escapeString(const std::string& str);
    std::string literalToString(const std::variant<double, std::string, bool, std::nullptr_t>& value);
    
    int indentLevel = 0;
    std::string indent();
};

} // namespace hulk::backend

#endif