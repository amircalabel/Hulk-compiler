// src/backend/ASTSerializer.cpp
#include "ASTSerializer.hpp"
#include <sstream>
#include <iomanip>

namespace hulk::backend {

std::string ASTSerializer::indent() {
    return std::string(indentLevel * 2, ' ');
}

std::string ASTSerializer::escapeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string ASTSerializer::literalToString(const std::variant<double, std::string, bool, std::nullptr_t>& value) {
    if (std::holds_alternative<std::nullptr_t>(value)) return "nullptr";
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value) ? "true" : "false";
    if (std::holds_alternative<double>(value)) {
        double num = std::get<double>(value);
        if (num == static_cast<int>(num)) return std::to_string(static_cast<int>(num));
        return std::to_string(num);
    }
    if (std::holds_alternative<std::string>(value)) {
        return "\"" + escapeString(std::get<std::string>(value)) + "\"";
    }
    return "nullptr";
}

std::string ASTSerializer::serializeExpr(const Expr& expr) {
    // Esta es una versión simplificada
    // En una implementación completa, recorrerías todos los tipos de expresiones
    return "nullptr";
}

std::string ASTSerializer::serializeStmt(const Stmt& stmt) {
    // Esta es una versión simplificada
    return "/* statement */";
}

std::string ASTSerializer::serialize(const std::vector<std::unique_ptr<Stmt>>& statements) {
    std::stringstream ss;
    ss << "std::vector<std::unique_ptr<Stmt>> statements;\n";
    
    for (const auto& stmt : statements) {
        if (stmt) {
            ss << "statements.push_back(" << serializeStmt(*stmt) << ");\n";
        }
    }
    
    return ss.str();
}

} // namespace hulk::backend