// src/ast/AstPrinter.cpp
#include "AstPrinter.hpp"
#include <iomanip>

namespace hulk {

// ============================================================
// Helpers
// ============================================================

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

void AstPrinter::indent() {
    for (int i = 0; i < indentLevel; i++) {
        output << "  ";
    }
}

// ============================================================
// Función auxiliar para imprimir expresiones sin interferir con output
// ============================================================

std::string AstPrinter::printExpr(const Expr& expr) {
    // Guardar el estado actual del output
    std::stringstream oldOutput;
    oldOutput.swap(output);
    
    // Imprimir la expresión
    expr.accept(*this);
    std::string result = output.str();
    
    // Restaurar el output anterior
    output.str("");
    output.clear();
    output << oldOutput.str();
    
    return result;
}

// ============================================================
// Puntos de entrada públicos
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
    output << "(let (";
    
    for (size_t i = 0; i < expr.bindings.size(); i++) {
        const auto& binding = expr.bindings[i];
        if (i > 0) output << " ";
        output << "(" << binding.name.lexeme << " " << printExpr(*binding.initializer) << ")";
    }
    
    output << ") " << printExpr(*expr.body) << ")";
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
AstPrinter::visitIfExpr(const IfExpr& expr) {
    output << "(if " << printExpr(*expr.condition) << " " << printExpr(*expr.thenBranch);
    
    for (const auto& elif : expr.elifBranches) {
        output << " (elif " << printExpr(*elif.first) << " " << printExpr(*elif.second) << ")";
    }
    
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
    
    if (expr.initializer) {
        output << printExpr(*expr.initializer) << " ";
    } else {
        output << "() ";
    }
    
    if (expr.condition) {
        output << printExpr(*expr.condition) << " ";
    } else {
        output << "() ";
    }
    
    if (expr.increment) {
        output << printExpr(*expr.increment) << " ";
    } else {
        output << "() ";
    }
    
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
    indentLevel++;
    for (const auto& s : stmt.statements) {
        output << "\n";
        indent();
        s->accept(*this);
    }
    indentLevel--;
    output << "\n";
    indent();
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
        if (stmt.parameters[i].typeAnnotation.type != TokenType::TOKEN_ERROR) {
            output << " : " << stmt.parameters[i].typeAnnotation.lexeme;
        }
    }
    output << ")";
    
    if (stmt.returnTypeAnnotation.type != TokenType::TOKEN_ERROR) {
        output << " : " << stmt.returnTypeAnnotation.lexeme;
    }
    
    output << " {";
    
    for (const auto& bodyStmt : stmt.body) {
        output << "\n";
        indent();
        bodyStmt->accept(*this);
    }
    
    output << "})";
}

void AstPrinter::visitClassDeclStmt(const ClassDeclStmt& stmt) {
    output << "(type " << stmt.name.lexeme;
    
    if (!stmt.typeArguments.empty()) {
        output << "(";
        for (size_t i = 0; i < stmt.typeArguments.size(); i++) {
            if (i > 0) output << ", ";
            output << stmt.typeArguments[i].lexeme;
        }
        output << ")";
    }
    
    if (stmt.superclass.type != TokenType::TOKEN_ERROR) {
        output << " inherits " << stmt.superclass.lexeme;
    }
    
    output << " {";
    
    for (const auto& attr : stmt.attributes) {
        output << "\n";
        indent();
        output << attr.first.lexeme;
        if (attr.second.type != TokenType::TOKEN_ERROR) {
            output << " : " << attr.second.lexeme;
        }
        output << " = ?;";
    }
    
    for (const auto& method : stmt.methods) {
        output << "\n";
        method->accept(*this);
    }
    
    output << "\n})";
}

void AstPrinter::visitProtocolDeclStmt(const ProtocolDeclStmt& stmt) {
    output << "(protocol " << stmt.name.lexeme;
    
    if (stmt.extends.type != TokenType::TOKEN_ERROR) {
        output << " inherits " << stmt.extends.lexeme;
    }
    
    output << " {";
    
    for (const auto& method : stmt.methods) {
        output << "\n";
        indent();
        output << method.name.lexeme << "(";
        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0) output << ", ";
            output << method.parameters[i].first.lexeme;
            if (method.parameters[i].second.type != TokenType::TOKEN_ERROR) {
                output << " : " << method.parameters[i].second.lexeme;
            }
        }
        output << ")";
        if (method.returnType.type != TokenType::TOKEN_ERROR) {
            output << " : " << method.returnType.lexeme;
        }
        output << ";";
    }
    
    output << "\n})";
}

void AstPrinter::visitMacroDeclStmt(const MacroDeclStmt& stmt) {
    output << "(def " << stmt.name.lexeme << " (";
    
    for (size_t i = 0; i < stmt.parameters.size(); i++) {
        if (i > 0) output << ", ";
        if (stmt.parameters[i].isSymbolic) {
            output << "@";
        } else if (stmt.parameters[i].isPlaceholder) {
            output << "$";
        }
        output << stmt.parameters[i].name.lexeme;
    }
    output << ") => " << printExpr(*stmt.body) << ";)";
}

} // namespace hulk