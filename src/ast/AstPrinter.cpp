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
// Método auxiliar para imprimir expresiones sin afectar el output principal
// ============================================================
std::string AstPrinter::printExpr(const Expr& expr) {
    std::stringstream saved;
    saved << output.str();
    
    output.str("");
    output.clear();
    
    expr.accept(*this);
    std::string result = output.str();
    
    output.str("");
    output.clear();
    output << saved.str();
    
    return result;
}

// ============================================================
// Métodos públicos
// ============================================================

std::string AstPrinter::print(const Expr& expr) {
    output.str("");
    output.clear();
    return printExpr(expr);
}

std::string AstPrinter::print(const Stmt& stmt) {
    output.str("");
    output.clear();
    stmt.accept(*this);
    return output.str();
}

std::string AstPrinter::print(const std::vector<std::unique_ptr<Stmt>>& statements) {
    output.str("");
    output.clear();
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
    std::string left = printExpr(*expr.left);
    std::string right = printExpr(*expr.right);
    output << "(" << expr.op.lexeme << " " << left << " " << right << ")";
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitUnaryExpr(const UnaryExpr& expr) {
    std::string right = printExpr(*expr.right);
    output << "(" << expr.op.lexeme << " " << right << ")";
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitGroupingExpr(const GroupingExpr& expr) {
    std::string inner = printExpr(*expr.expression);
    output << "(group " << inner << ")";
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitVariableExpr(const VariableExpr& expr) {
    output << expr.name.lexeme;
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitAssignExpr(const AssignExpr& expr) {
    std::string value = printExpr(*expr.value);
    output << "(:= " << expr.name.lexeme << " " << value << ")";
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitLetExpr(const LetExpr& expr) {
    std::string bindings;
    for (size_t i = 0; i < expr.bindings.size(); i++) {
        if (i > 0) bindings += " ";
        bindings += "(" + expr.bindings[i].name.lexeme + " " + printExpr(*expr.bindings[i].initializer) + ")";
    }
    output << "(let (" << bindings << ") " << printExpr(*expr.body) << ")";
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitIfExpr(const IfExpr& expr) {
    output << "(if " << printExpr(*expr.condition) << " " << printExpr(*expr.thenBranch);
    if (expr.elseBranch) {
        output << " else " << printExpr(*expr.elseBranch);
    }
    output << ")";
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitWhileExpr(const WhileExpr& expr) {
    output << "(while " << printExpr(*expr.condition) << " " << printExpr(*expr.body) << ")";
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitForExpr(const ForExpr& expr) {
    output << "(for ";
    output << (expr.initializer ? printExpr(*expr.initializer) : "()") << " ";
    output << (expr.condition ? printExpr(*expr.condition) : "()") << " ";
    output << (expr.increment ? printExpr(*expr.increment) : "()") << " ";
    output << printExpr(*expr.body) << ")";
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitBlockExpr(const BlockExpr& expr) {
    output << "(block";
    for (const auto& e : expr.expressions) {
        output << " " << printExpr(*e);
    }
    output << ")";
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitCallExpr(const CallExpr& expr) {
    output << "(call " << printExpr(*expr.callee);
    for (const auto& arg : expr.arguments) {
        output << " " << printExpr(*arg);
    }
    output << ")";
    return 0.0;
}

// ============================================================
// Visitadores para Statements
// ============================================================

void AstPrinter::visitExpressionStmt(const ExpressionStmt& stmt) {
    output << printExpr(*stmt.expression) << ";";
}

void AstPrinter::visitPrintStmt(const PrintStmt& stmt) {
    output << "(print " << printExpr(*stmt.expression) << ");";
}

void AstPrinter::visitReturnStmt(const ReturnStmt& stmt) {
    if (stmt.value) {
        output << "(return " << printExpr(*stmt.value) << ");";
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
        output << " = " << printExpr(*stmt.initializer);
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
    output << "(if " << printExpr(*stmt.condition) << " " << print(*stmt.thenBranch);
    if (stmt.elseBranch) {
        output << " else " << print(*stmt.elseBranch);
    }
    output << ")";
}

void AstPrinter::visitWhileStmt(const WhileStmt& stmt) {
    output << "(while " << printExpr(*stmt.condition) << " " << print(*stmt.body) << ")";
}

void AstPrinter::visitForStmt(const ForStmt& stmt) {
    output << "(for ...)";
}

} // namespace hulk
