// src/backend/CodeGenerator.cpp
#include "CodeGenerator.hpp"
#include "ASTSerializer.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

namespace hulk::backend {

static const char* RUNTIME_HEADER = R"(#include <iostream>
#include <variant>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <unordered_map>

// ============================================================
// HULK Runtime Library
// ============================================================

struct Obj;
using HulkValue = std::variant<double, std::string, bool, std::nullptr_t, Obj*>;

// Tipos de objetos
enum class ObjType {
    STRING,
    FUNCTION,
    CLASS,
    INSTANCE
};

struct Obj {
    ObjType type;
    virtual ~Obj() = default;
};

struct ObjString : public Obj {
    std::string value;
    ObjString(const std::string& s) : value(s) { type = ObjType::STRING; }
};

// Conversiones
bool isTruthy(const HulkValue& v) {
    if (std::holds_alternative<std::nullptr_t>(v)) return false;
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v);
    return true;
}

std::string stringify(const HulkValue& v) {
    if (std::holds_alternative<std::nullptr_t>(v)) return "nil";
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? "true" : "false";
    if (std::holds_alternative<double>(v)) {
        double d = std::get<double>(v);
        if (d == static_cast<int>(d)) return std::to_string(static_cast<int>(d));
        return std::to_string(d);
    }
    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
    if (std::holds_alternative<Obj*>(v)) {
        Obj* obj = std::get<Obj*>(v);
        if (obj->type == ObjType::STRING) {
            return static_cast<ObjString*>(obj)->value;
        }
    }
    return "unknown";
}

bool valuesEqual(const HulkValue& a, const HulkValue& b) {
    if (std::holds_alternative<std::nullptr_t>(a) && std::holds_alternative<std::nullptr_t>(b)) return true;
    if (std::holds_alternative<bool>(a) && std::holds_alternative<bool>(b)) return std::get<bool>(a) == std::get<bool>(b);
    if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b)) return std::get<double>(a) == std::get<double>(b);
    if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b)) return std::get<std::string>(a) == std::get<std::string>(b);
    return false;
}

// Operaciones aritméticas
HulkValue add(const HulkValue& a, const HulkValue& b) {
    if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b)) {
        return std::get<double>(a) + std::get<double>(b);
    }
    return nullptr;
}

HulkValue subtract(const HulkValue& a, const HulkValue& b) {
    if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b)) {
        return std::get<double>(a) - std::get<double>(b);
    }
    return nullptr;
}

HulkValue multiply(const HulkValue& a, const HulkValue& b) {
    if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b)) {
        return std::get<double>(a) * std::get<double>(b);
    }
    return nullptr;
}

HulkValue divide(const HulkValue& a, const HulkValue& b) {
    if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b)) {
        double divisor = std::get<double>(b);
        if (divisor == 0) return nullptr;
        return std::get<double>(a) / divisor;
    }
    return nullptr;
}

// Entorno de variables
class Environment {
public:
    Environment(std::shared_ptr<Environment> enclosing = nullptr) : enclosing(enclosing) {}
    
    void define(const std::string& name, const HulkValue& value) {
        values[name] = value;
    }
    
    HulkValue get(const std::string& name) const {
        auto it = values.find(name);
        if (it != values.end()) return it->second;
        if (enclosing) return enclosing->get(name);
        return nullptr;
    }
    
    void assign(const std::string& name, const HulkValue& value) {
        auto it = values.find(name);
        if (it != values.end()) {
            it->second = value;
            return;
        }
        if (enclosing) {
            enclosing->assign(name, value);
            return;
        }
    }
    
private:
    std::shared_ptr<Environment> enclosing;
    std::unordered_map<std::string, HulkValue> values;
};

// ============================================================
// Programa principal
// ============================================================

)";

bool CodeGenerator::generate(const std::vector<std::unique_ptr<Stmt>>& statements, const std::string& outputPath) {
    std::string sourcePath = outputPath + ".cpp";
    std::ofstream sourceFile(sourcePath);
    
    if (!sourceFile.is_open()) {
        std::cerr << "Could not create source file: " << sourcePath << std::endl;
        return false;
    }
    
    // Escribir el encabezado runtime
    sourceFile << RUNTIME_HEADER;
    
    // Escribir el código generado a partir del AST
    sourceFile << "int main() {\n";
    sourceFile << "    auto env = std::make_shared<Environment>();\n";
    sourceFile << "    HulkValue result;\n\n";
    
    // Generar código para cada statement
    for (const auto& stmt : statements) {
        if (stmt) {
            sourceFile << generateStatement(*stmt);
        }
    }
    
    sourceFile << "    return 0;\n";
    sourceFile << "}\n";
    
    sourceFile.close();
    
    // Compilar a binario
    return compileToBinary(sourcePath, outputPath);
}

std::string CodeGenerator::generateStatement(const Stmt& stmt) {
    // Versión simplificada - solo maneja PrintStmt
    if (auto printStmt = dynamic_cast<const PrintStmt*>(&stmt)) {
        return "    std::cout << stringify(" + generateExpression(*printStmt->expression) + ") << std::endl;\n";
    }
    
    // ExpressionStmt
    if (auto exprStmt = dynamic_cast<const ExpressionStmt*>(&stmt)) {
        return "    " + generateExpression(*exprStmt->expression) + ";\n";
    }
    
    return "    // Unhandled statement type\n";
}

std::string CodeGenerator::generateExpression(const Expr& expr) {
    // Literal
    if (auto literal = dynamic_cast<const LiteralExpr*>(&expr)) {
        return literalToValueExpr(literal->value);
    }
    
    // Binary expression
    if (auto binary = dynamic_cast<const BinaryExpr*>(&expr)) {
        std::string left = generateExpression(*binary->left);
        std::string right = generateExpression(*binary->right);
        
        switch (binary->op.type) {
            case TokenType::TOKEN_PLUS:  return "add(" + left + ", " + right + ")";
            case TokenType::TOKEN_MINUS: return "subtract(" + left + ", " + right + ")";
            case TokenType::TOKEN_STAR:  return "multiply(" + left + ", " + right + ")";
            case TokenType::TOKEN_SLASH: return "divide(" + left + ", " + right + ")";
            default: return left;
        }
    }
    
    // Variable
    if (auto var = dynamic_cast<const VariableExpr*>(&expr)) {
        return "env->get(\"" + var->name.lexeme + "\")";
    }
    
    return "nullptr";
}

std::string CodeGenerator::literalToValueExpr(const std::variant<double, std::string, bool, std::nullptr_t>& value) {
    if (std::holds_alternative<std::nullptr_t>(value)) return "HulkValue(nullptr)";
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "HulkValue(true)" : "HulkValue(false)";
    }
    if (std::holds_alternative<double>(value)) {
        double num = std::get<double>(value);
        return "HulkValue(static_cast<double>(" + std::to_string(num) + "))";
    }
    if (std::holds_alternative<std::string>(value)) {
        return "HulkValue(std::string(\"" + escapeString(std::get<std::string>(value)) + "\"))";
    }
    return "HulkValue(nullptr)";
}

std::string CodeGenerator::literalToString(const std::variant<double, std::string, bool, std::nullptr_t>& value) {
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

std::string CodeGenerator::escapeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            default: result += c; break;
        }
    }
    return result;
}

bool CodeGenerator::compileToBinary(const std::string& sourcePath, const std::string& outputPath) {
    std::string command = "g++ -std=c++17 -O2 " + sourcePath + " -o " + outputPath;
    int result = std::system(command.c_str());
    
    if (result != 0) {
        std::cerr << "Compilation failed: " << command << std::endl;
        return false;
    }
    
    // Hacer ejecutable
    chmod(outputPath.c_str(), 0755);
    
    // Eliminar archivo fuente temporal
    std::remove(sourcePath.c_str());
    
    return true;
}

} // namespace hulk::backend