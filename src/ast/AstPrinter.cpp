// src/ast/AstPrinter.cpp
#include "AstPrinter.hpp"
#include <iomanip>

namespace hulk {

std::string AstPrinter::literalToString(const std::variant<double, std::string, bool, std::nullptr_t>& value) {
    if (std::holds_alternative<std::nullptr_t>(value)) {
        return "nil";
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    if (std::holds_alternative<double>(value)) {
        double num = std::get<double>(value);
        if (num == static_cast<int>(num)) {
            return std::to_string(static_cast<int>(num));
        }
        return std::to_string(num);
    }
    if (std::holds_alternative<std::string>(value)) {
        return "\"" + std::get<std::string>(value) + "\"";
    }
    return "unknown";
}

void AstPrinter::parenthesize(const std::string& name, const std::vector<std::string>& parts) {
    output << "(" << name;
    for (const auto& part : parts) {
        output << " " << part;
    }
    output << ")";
}

// ============================================================
// Métodos públicos
// ============================================================

std::string AstPrinter::print(const Expr& expr) {
    output.str("");
    expr.accept(*this);
    return output.str();
}

std::string AstPrinter::print(const Stmt& stmt) {
    output.str("");
    stmt.accept(*this);
    return output.str();
}

std::string AstPrinter::print(const std::vector<std::unique_ptr<Stmt>>& statements) {
    output.str("");
    for (const auto& stmt : statements) {
        if (stmt) {
            stmt->accept(*this);
            output << "\n";
        }
    }
    return output.str();
}

// ============================================================
// Visitadores para Expresiones
// ============================================================

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitLiteralExpr(const LiteralExpr& expr) {
    output << literalToString(expr.value);
    return expr.value;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitBinaryExpr(const BinaryExpr& expr) {
    std::string left = print(*expr.left);
    std::string right = print(*expr.right);
    parenthesize(expr.op.lexeme, {left, right});
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitUnaryExpr(const UnaryExpr& expr) {
    std::string right = print(*expr.right);
    parenthesize(expr.op.lexeme, {right});
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitGroupingExpr(const GroupingExpr& expr) {
    parenthesize("group", {print(*expr.expression)});
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitVariableExpr(const VariableExpr& expr) {
    output << expr.name.lexeme;
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitAssignExpr(const AssignExpr& expr) {
    parenthesize(":=", {expr.name.lexeme, print(*expr.value)});
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitLetExpr(const LetExpr& expr) {
    std::string bindings;
    for (size_t i = 0; i < expr.bindings.size(); i++) {
        if (i > 0) bindings += " ";
        bindings += "(" + expr.bindings[i].name.lexeme + " " + print(*expr.bindings[i].initializer) + ")";
    }
    parenthesize("let", {"(" + bindings + ")", print(*expr.body)});
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitIfExpr(const IfExpr& expr) {
    std::vector<std::string> parts;
    parts.push_back("if");
    parts.push_back(print(*expr.condition));
    parts.push_back(print(*expr.thenBranch));
    if (expr.elseBranch) {
        parts.push_back("else");
        parts.push_back(print(*expr.elseBranch));
    }
    parenthesize("if", parts);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitWhileExpr(const WhileExpr& expr) {
    parenthesize("while", {print(*expr.condition), print(*expr.body)});
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitForExpr(const ForExpr& expr) {
    std::vector<std::string> parts;
    parts.push_back("for");
    parts.push_back(expr.initializer ? print(*expr.initializer) : "()");
    parts.push_back(expr.condition ? print(*expr.condition) : "()");
    parts.push_back(expr.increment ? print(*expr.increment) : "()");
    parts.push_back(print(*expr.body));
    parenthesize("for", parts);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitBlockExpr(const BlockExpr& expr) {
    std::vector<std::string> parts;
    for (const auto& e : expr.expressions) {
        parts.push_back(print(*e));
    }
    parenthesize("block", parts);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitCallExpr(const CallExpr& expr) {
    std::vector<std::string> parts;
    parts.push_back("call");
    parts.push_back(print(*expr.callee));
    for (const auto& arg : expr.arguments) {
        parts.push_back(print(*arg));
    }
    parenthesize("call", parts);
    return 0.0;
}

// ============================================================
// Visitadores para Statements
// ============================================================

void AstPrinter::visitExpressionStmt(const ExpressionStmt& stmt) {
    output << print(*stmt.expression) << ";";
}

void AstPrinter::visitPrintStmt(const PrintStmt& stmt) {
    output << "(print " << print(*stmt.expression) << ");";
}

void AstPrinter::visitReturnStmt(const ReturnStmt& stmt) {
    if (stmt.value) {
        output << "(return " << print(*stmt.value) << ");";
    } else {
        output << "(return);";
    }
}

void AstPrinter::visitBlockStmt(const BlockStmt& stmt) {
    output << "{";
    for (const auto& s : stmt.statements) {
        output << " " << print(*s);
    }
    output << "}";
}

void AstPrinter::visitVarDeclStmt(const VarDeclStmt& stmt) {
    output << "(var " << stmt.name.lexeme;
    if (stmt.typeAnnotation.type != TokenType::TOKEN_ERROR) {
        output << " : " << stmt.typeAnnotation.lexeme;
    }
    if (stmt.initializer) {
        output << " = " << print(*stmt.initializer);
    }
    output << ");";
}

void AstPrinter::visitFunctionDeclStmt(const FunctionDeclStmt& stmt) {
    output << "(function " << stmt.name.lexeme << " (";
    for (size_t i = 0; i < stmt.parameters.size(); i++) {
        if (i > 0) output << ", ";
        output << stmt.parameters[i].name.lexeme;
    }
    output << ") { ... })";
}

void AstPrinter::visitClassDeclStmt(const ClassDeclStmt& stmt) {
    output << "(class " << stmt.name.lexeme << " { ... })";
}

void AstPrinter::visitProtocolDeclStmt(const ProtocolDeclStmt& stmt) {
    output << "(protocol " << stmt.name.lexeme << " { ... })";
}

void AstPrinter::visitMacroDeclStmt(const MacroDeclStmt& stmt) {
    output << "(macro " << stmt.name.lexeme << " ...)";
}

void AstPrinter::visitIfStmt(const IfStmt& stmt) {
    output << "(if " << print(*stmt.condition) << " then else)";
}

void AstPrinter::visitWhileStmt(const WhileStmt& stmt) {
    output << "(while " << print(*stmt.condition) << " do)";
}

void AstPrinter::visitForStmt(const ForStmt& stmt) {
    output << "(for ...)";
}

} // namespace hulk