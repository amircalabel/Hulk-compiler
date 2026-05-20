// src/ast/AstPrinter.cpp
#include "AstPrinter.h"
#include <iomanip>

// ============================================================
// Helpers
// ============================================================

void AstPrinter::indent() {
    for (int i = 0; i < indentLevel; i++) {
        output << "  ";
    }
}

void AstPrinter::parenthesize(const std::string& name, const std::vector<std::string>& parts) {
    output << "(" << name;
    for (const auto& part : parts) {
        output << " " << part;
    }
    output << ")";
}

std::string AstPrinter::literalToString(const std::variant<double, std::string, bool, std::nullptr_t>& value) {
    if (std::holds_alternative<std::nullptr_t>(value)) {
        return "nil";
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    if (std::holds_alternative<double>(value)) {
        double num = std::get<double>(value);
        // Eliminar .0 de números enteros para mejor legibilidad
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

std::string AstPrinter::tokenToString(const Token& token) {
    if (token.type == TokenType::TOKEN_ERROR) {
        return "?";
    }
    return token.lexeme;
}

// ============================================================
// Puntos de entrada
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
// Implementación de visitadores - Expresiones
// ============================================================

std::variant<double, std::string, bool, std::nullptr_t> AstPrinter::visitLiteralExpr(const LiteralExpr& expr) {
    output << literalToString(expr.value);
    return expr.value;  // Retornamos el valor para cumplir con el Visitor
}

std::variant<double, std::string, bool, std::nullptr_t> AstPrinter::visitBinaryExpr(const BinaryExpr& expr) {
    std::string left = print(*expr.left);
    std::string right = print(*expr.right);
    parenthesize(tokenToString(expr.op), {left, right});
    return 0.0;  // Valor dummy
}

std::variant<double, std::string, bool, std::nullptr_t> AstPrinter::visitUnaryExpr(const UnaryExpr& expr) {
    std::string right = print(*expr.right);
    parenthesize(tokenToString(expr.op), {right});
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> AstPrinter::visitGroupingExpr(const GroupingExpr& expr) {
    parenthesize("group", {print(*expr.expression)});
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> AstPrinter::visitVariableExpr(const VariableExpr& expr) {
    output << expr.name.lexeme;
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> AstPrinter::visitAssignExpr(const AssignExpr& expr) {
    std::string value = print(*expr.value);
    parenthesize(":=", {expr.name.lexeme, value});
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> AstPrinter::visitLetExpr(const LetExpr& expr) {
    std::vector<std::string> bindings;
    for (const auto& binding : expr.bindings) {
        std::string bindingStr = "(" + binding.name.lexeme;
        if (binding.typeAnnotation.type != TokenType::TOKEN_ERROR) {
            bindingStr += " : " + binding.typeAnnotation.lexeme;
        }
        bindingStr += " = " + print(*binding.initializer) + ")";
        bindings.push_back(bindingStr);
    }
    
    std::vector<std::string> parts;
    parts.push_back("(");
    for (const auto& b : bindings) {
        parts.push_back(b);
    }
    parts.push_back(")");
    parts.push_back(print(*expr.body));
    
    parenthesize("let", {parts[0], parts[1], parts[2]});
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> AstPrinter::visitIfExpr(const IfExpr& expr) {
    std::vector<std::string> parts;
    parts.push_back("if");
    parts.push_back(print(*expr.condition));
    parts.push_back(print(*expr.thenBranch));
    
    for (const auto& elif : expr.elifBranches) {
        parts.push_back("elif");
        parts.push_back(print(*elif.first));
        parts.push_back(print(*elif.second));
    }
    
    if (expr.elseBranch) {
        parts.push_back("else");
        parts.push_back(print(*expr.elseBranch));
    }
    
    parenthesize("if", parts);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> AstPrinter::visitWhileExpr(const WhileExpr& expr) {
    parenthesize("while", {print(*expr.condition), print(*expr.body)});
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> AstPrinter::visitForExpr(const ForExpr& expr) {
    std::vector<std::string> parts;
    parts.push_back("for");
    
    if (expr.initializer) {
        parts.push_back("init=" + print(*expr.initializer));
    } else {
        parts.push_back("init=none");
    }
    
    if (expr.condition) {
        parts.push_back("cond=" + print(*expr.condition));
    } else {
        parts.push_back("cond=none");
    }
    
    if (expr.increment) {
        parts.push_back("inc=" + print(*expr.increment));
    } else {
        parts.push_back("inc=none");
    }
    
    parts.push_back(print(*expr.body));
    
    parenthesize("for", parts);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> AstPrinter::visitBlockExpr(const BlockExpr& expr) {
    std::vector<std::string> parts;
    for (const auto& e : expr.expressions) {
        parts.push_back(print(*e));
    }
    parenthesize("block", parts);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> AstPrinter::visitCallExpr(const CallExpr& expr) {
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
// Implementación de visitadores - Statements
// ============================================================

void AstPrinter::visitExpressionStmt(const ExpressionStmt& stmt) {
    indent();
    output << print(*stmt.expression) << ";";
}

void AstPrinter::visitPrintStmt(const PrintStmt& stmt) {
    indent();
    output << "(print " << print(*stmt.expression) << ");";
}

void AstPrinter::visitReturnStmt(const ReturnStmt& stmt) {
    indent();
    if (stmt.value) {
        output << "(return " << print(*stmt.value) << ");";
    } else {
        output << "(return);";
    }
}

void AstPrinter::visitBlockStmt(const BlockStmt& stmt) {
    indent();
    output << "{";
    indentLevel++;
    for (const auto& s : stmt.statements) {
        output << "\n";
        s->accept(*this);
    }
    indentLevel--;
    output << "\n";
    indent();
    output << "}";
}

void AstPrinter::visitVarDeclStmt(const VarDeclStmt& stmt) {
    indent();
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
    indent();
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
    
    output << " ";
    
    // Imprimir el cuerpo
    indentLevel++;
    for (const auto& bodyStmt : stmt.body) {
        output << "\n";
        bodyStmt->accept(*this);
    }
    indentLevel--;
    output << ")";
}

void AstPrinter::visitClassDeclStmt(const ClassDeclStmt& stmt) {
    indent();
    output << "(type " << stmt.name.lexeme;
    
    // Parámetros de tipo
    if (!stmt.typeArguments.empty()) {
        output << "(";
        for (size_t i = 0; i < stmt.typeArguments.size(); i++) {
            if (i > 0) output << ", ";
            output << stmt.typeArguments[i].lexeme;
        }
        output << ")";
    }
    
    // Superclase
    if (stmt.superclass.type != TokenType::TOKEN_ERROR) {
        output << " inherits " << stmt.superclass.lexeme;
    }
    
    output << " {";
    indentLevel++;
    
    // Atributos
    for (const auto& attr : stmt.attributes) {
        output << "\n";
        indent();
        output << attr.first.lexeme;
        if (attr.second.type != TokenType::TOKEN_ERROR) {
            output << " : " << attr.second.lexeme;
        }
        output << " = ?;";  // Inicialización simplificada
    }
    
    // Métodos
    for (const auto& method : stmt.methods) {
        output << "\n";
        method->accept(*this);
    }
    
    indentLevel--;
    output << "\n";
    indent();
    output << "})";
}

void AstPrinter::visitProtocolDeclStmt(const ProtocolDeclStmt& stmt) {
    indent();
    output << "(protocol " << stmt.name.lexeme;
    
    if (stmt.extends.type != TokenType::TOKEN_ERROR) {
        output << " inherits " << stmt.extends.lexeme;
    }
    
    output << " {";
    indentLevel++;
    
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
    
    indentLevel--;
    output << "\n";
    indent();
    output << "})";
}

void AstPrinter::visitMacroDeclStmt(const MacroDeclStmt& stmt) {
    indent();
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
    output << ") => " << print(*stmt.body) << ";)";
}