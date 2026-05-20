// src/inferer/TypeInferer.cpp
#include "TypeInferer.hpp"
#include <iostream>
#include <algorithm>

namespace hulk {

TypeInferer::TypeInferer(Resolver& resolver) : resolver(resolver) {}

void TypeInferer::infer(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& stmt : statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
}

std::shared_ptr<Type> TypeInferer::getType(const Expr& expr) const {
    auto it = exprTypes.find(&expr);
    if (it != exprTypes.end()) {
        return it->second;
    }
    return Type::unknown();
}

// ============================================================
// Reglas de inferencia (sección A.9.2)
// ============================================================

std::shared_ptr<Type> TypeInferer::inferLiteral(const LiteralExpr& expr) {
    if (std::holds_alternative<double>(expr.value)) {
        return Type::number();
    }
    if (std::holds_alternative<std::string>(expr.value)) {
        return Type::string();
    }
    if (std::holds_alternative<bool>(expr.value)) {
        return Type::boolean();
    }
    return Type::nil();
}

std::shared_ptr<Type> TypeInferer::inferBinary(const BinaryExpr& expr) {
    auto leftType = getType(*expr.left);
    auto rightType = getType(*expr.right);
    
    switch (expr.op.type) {
        case TokenType::TOKEN_PLUS:
            // + puede ser suma numérica o concatenación de strings (sección A.2.2)
            if (leftType->kind == TypeKind::NUMBER && rightType->kind == TypeKind::NUMBER) {
                return Type::number();
            }
            if (leftType->kind == TypeKind::STRING && rightType->kind == TypeKind::STRING) {
                return Type::string();
            }
            reportTypeError(expr.op, "Number or String", leftType->toString());
            return Type::error();
            
        case TokenType::TOKEN_MINUS:
        case TokenType::TOKEN_STAR:
        case TokenType::TOKEN_SLASH:
        case TokenType::TOKEN_CARET:  // Potencia
            if (leftType->kind == TypeKind::NUMBER && rightType->kind == TypeKind::NUMBER) {
                return Type::number();
            }
            reportTypeError(expr.op, "Number", leftType->toString());
            return Type::error();
            
        case TokenType::TOKEN_AT:
        case TokenType::TOKEN_AT_AT:  // Concatenación con espacio
            if (leftType->kind == TypeKind::STRING && rightType->kind == TypeKind::STRING) {
                return Type::string();
            }
            reportTypeError(expr.op, "String", leftType->toString());
            return Type::error();
            
        case TokenType::TOKEN_EQUAL_EQUAL:
        case TokenType::TOKEN_BANG_EQUAL:
            // == y != aceptan cualquier tipo, retornan Boolean
            return Type::boolean();
            
        case TokenType::TOKEN_LESS:
        case TokenType::TOKEN_LESS_EQUAL:
        case TokenType::TOKEN_GREATER:
        case TokenType::TOKEN_GREATER_EQUAL:
            // Comparaciones solo con números
            if (leftType->kind == TypeKind::NUMBER && rightType->kind == TypeKind::NUMBER) {
                return Type::boolean();
            }
            reportTypeError(expr.op, "Number", leftType->toString());
            return Type::error();
            
        default:
            return Type::unknown();
    }
}

std::shared_ptr<Type> TypeInferer::inferUnary(const UnaryExpr& expr) {
    auto rightType = getType(*expr.right);
    
    switch (expr.op.type) {
        case TokenType::TOKEN_MINUS:
            if (rightType->kind == TypeKind::NUMBER) {
                return Type::number();
            }
            reportTypeError(expr.op, "Number", rightType->toString());
            return Type::error();
            
        case TokenType::TOKEN_BANG:
            // ! retorna Boolean para cualquier tipo (basado en truthiness, sección A.5)
            return Type::boolean();
            
        default:
            return Type::unknown();
    }
}

std::shared_ptr<Type> TypeInferer::inferIf(const IfExpr& expr) {
    auto condType = getType(*expr.condition);
    
    // La condición debe ser Boolean (o convertible)
    if (condType->kind != TypeKind::BOOLEAN) {
        // En HULK, cualquier valor es truthy excepto false y nil (A.5)
        // Por lo tanto, no es un error, pero emitimos warning
        // std::cerr << "Warning: condition should be Boolean, got " << condType->toString() << std::endl;
    }
    
    std::vector<std::shared_ptr<Type>> branchTypes;
    
    // Then branch
    branchTypes.push_back(getType(*expr.thenBranch));
    
    // Elif branches
    for (const auto& elif : expr.elifBranches) {
        branchTypes.push_back(getType(*elif.second));
    }
    
    // Else branch (si existe)
    if (expr.elseBranch) {
        branchTypes.push_back(getType(*expr.elseBranch));
    }
    
    // El tipo del if es el LCA de todas las ramas (A.9.2)
    return lowestCommonAncestor(branchTypes);
}

std::shared_ptr<Type> TypeInferer::inferBlock(const BlockExpr& expr) {
    if (expr.expressions.empty()) {
        return Type::nil();  // Bloque vacío retorna nil (A.2.4)
    }
    
    // El tipo del bloque es el tipo de la última expresión (A.9.2)
    return getType(*expr.expressions.back());
}

std::shared_ptr<Type> TypeInferer::inferLet(const LetExpr& expr) {
    // Inicializar variables en el scope
    for (const auto& binding : expr.bindings) {
        auto initType = getType(*binding.initializer);
        
        // Si hay anotación de tipo, verificar conformidad (A.8.1)
        if (binding.typeAnnotation.type != TokenType::TOKEN_ERROR) {
            // TODO: Buscar tipo anotado en symbolTable
            // validateType(*binding.initializer, annotatedType);
        }
        
        // Registrar tipo de la variable
        SymbolInfo info;
        info.name = binding.name;
        info.inferredType = initType;
        symbolTable[binding.name.lexeme] = info;
    }
    
    // El tipo del let es el tipo de su cuerpo (A.9.2)
    return getType(*expr.body);
}

std::shared_ptr<Type> TypeInferer::inferCall(const CallExpr& expr) {
    auto calleeType = getType(*expr.callee);
    
    if (calleeType->kind != TypeKind::FUNCTION) {
        reportTypeError(expr.paren, "Function", calleeType->toString());
        return Type::error();
    }
    
    // Verificar cantidad de argumentos
    if (expr.arguments.size() != calleeType->functionInfo->parameterTypes.size()) {
        // TODO: Reportar error de aridad
        return Type::error();
    }
    
    // Verificar tipos de argumentos
    for (size_t i = 0; i < expr.arguments.size(); i++) {
        auto argType = getType(*expr.arguments[i]);
        auto paramType = calleeType->functionInfo->parameterTypes[i];
        
        if (!argType->conformsTo(*paramType)) {
            // Reportar error de tipo
        }
    }
    
    return calleeType->functionInfo->returnType;
}

// Encontrar el ancestro común más bajo (LCA) en la jerarquía de tipos
std::shared_ptr<Type> TypeInferer::lowestCommonAncestor(const std::vector<std::shared_ptr<Type>>& types) {
    if (types.empty()) return Type::nil();
    if (types.size() == 1) return types[0];
    
    // Todos los tipos son iguales -> retornar ese tipo
    bool allEqual = true;
    for (size_t i = 1; i < types.size(); i++) {
        if (!types[0]->equals(*types[i])) {
            allEqual = false;
            break;
        }
    }
    if (allEqual) return types[0];
    
    // Verificar si hay nil (nil es subtipo de todo)
    for (const auto& t : types) {
        if (t->kind == TypeKind::NIL) {
            // Buscar el tipo no-nil más específico
            for (const auto& other : types) {
                if (other->kind != TypeKind::NIL) {
                    return other;
                }
            }
        }
    }
    
    // Si no, retornar Object (la raíz de la jerarquía, A.7.3)
    return Type::object();
}

// Sintetizar protocolos (A.9.5)
std::shared_ptr<Type> TypeInferer::synthesizeProtocol(
    const std::string& baseName,
    const std::vector<std::pair<std::string, std::shared_ptr<Type>>>& requiredMethods) {
    
    auto protocol = Type::protocolType("_Synthesized_" + baseName);
    protocol->protocolInfo = std::make_shared<ProtocolType>();
    protocol->protocolInfo->name = protocol->name;
    
    for (const auto& method : requiredMethods) {
        FunctionType funcType;
        funcType.returnType = method.second;
        protocol->protocolInfo->methods.push_back({method.first, funcType});
    }
    
    return protocol;
}

// Unificación de tipos (para variables de tipo)
bool TypeInferer::unify(std::shared_ptr<Type>& a, std::shared_ptr<Type>& b) {
    if (a->kind == TypeKind::UNKNOWN) {
        a = b;
        return true;
    }
    if (b->kind == TypeKind::UNKNOWN) {
        b = a;
        return true;
    }
    if (a->kind == TypeKind::TYPE_VAR) {
        a = b;
        return true;
    }
    if (b->kind == TypeKind::TYPE_VAR) {
        b = a;
        return true;
    }
    return a->equals(*b);
}

std::shared_ptr<Type> TypeInferer::substitute(
    const Type& type,
    const std::unordered_map<std::string, std::shared_ptr<Type>>& substitutions) {
    
    if (type.kind == TypeKind::TYPE_VAR) {
        auto it = substitutions.find(type.name);
        if (it != substitutions.end()) {
            return it->second;
        }
    }
    
    // Copiar el tipo
    auto result = std::make_shared<Type>(type);
    
    // Sustituir recursivamente en tipos anidados
    if (type.kind == TypeKind::FUNCTION && type.functionInfo) {
        for (auto& param : result->functionInfo->parameterTypes) {
            param = substitute(*param, substitutions);
        }
        result->functionInfo->returnType = substitute(*result->functionInfo->returnType, substitutions);
    }
    
    return result;
}

void TypeInferer::validateType(const Expr& expr, std::shared_ptr<Type> expected) {
    auto actual = getType(expr);
    if (!actual->conformsTo(*expected)) {
        reportTypeError(Token{}, expected->toString(), actual->toString());
    }
}

void TypeInferer::reportTypeError(const Token& token, const std::string& expected, const std::string& found) {
    std::cerr << "[line " << token.line << "] Type error: expected " 
              << expected << ", found " << found << std::endl;
    // hadError = true;
}

// ============================================================
// Visitadores para Statements
// ============================================================

void TypeInferer::visitExpressionStmt(const ExpressionStmt& stmt) {
    if (stmt.expression) {
        stmt.expression->accept(*this);
    }
}

void TypeInferer::visitPrintStmt(const PrintStmt& stmt) {
    if (stmt.expression) {
        stmt.expression->accept(*this);
        // print puede recibir cualquier tipo
    }
}

void TypeInferer::visitReturnStmt(const ReturnStmt& stmt) {
    if (stmt.value) {
        stmt.value->accept(*this);
    }
}

void TypeInferer::visitBlockStmt(const BlockStmt& stmt) {
    for (const auto& s : stmt.statements) {
        if (s) {
            s->accept(*this);
        }
    }
}

void TypeInferer::visitVarDeclStmt(const VarDeclStmt& stmt) {
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
        auto initType = getType(*stmt.initializer);
        
        // Registrar tipo inferido
        SymbolInfo info;
        info.name = stmt.name;
        info.inferredType = initType;
        symbolTable[stmt.name.lexeme] = info;
    }
}

void TypeInferer::visitFunctionDeclStmt(const FunctionDeclStmt& stmt) {
    // Inferir tipos de parámetros y retorno (A.9.3)
    std::vector<std::shared_ptr<Type>> paramTypes;
    
    for (const auto& param : stmt.parameters) {
        // Por ahora, asumimos Number como default
        // La inferencia real requiere análisis de uso (A.9.3)
        paramTypes.push_back(Type::number());
    }
    
    // Inferir tipo de retorno del cuerpo
    std::shared_ptr<Type> returnType = Type::nil();
    
    for (const auto& bodyStmt : stmt.body) {
        if (bodyStmt) {
            bodyStmt->accept(*this);
        }
    }
    
    // Si la función tiene return explícito, usar ese tipo
    // (Implementación simplificada)
    
    auto funcType = Type::functionType(paramTypes, returnType);
    
    SymbolInfo info;
    info.name = stmt.name;
    info.inferredType = funcType;
    symbolTable[stmt.name.lexeme] = info;
}

void TypeInferer::visitClassDeclStmt(const ClassDeclStmt& stmt) {
    auto classType = Type::classType(stmt.name.lexeme);
    classType->classInfo = std::make_shared<ClassType>();
    classType->classInfo->name = stmt.name.lexeme;
    
    symbolTable[stmt.name.lexeme].inferredType = classType;
    
    for (const auto& method : stmt.methods) {
        if (method) {
            method->accept(*this);
        }
    }
}

void TypeInferer::visitProtocolDeclStmt(const ProtocolDeclStmt& stmt) {
    auto protocolType = Type::protocolType(stmt.name.lexeme);
    protocolType->protocolInfo = std::make_shared<ProtocolType>();
    protocolType->protocolInfo->name = stmt.name.lexeme;
    
    symbolTable[stmt.name.lexeme].inferredType = protocolType;
}

void TypeInferer::visitMacroDeclStmt(const MacroDeclStmt& stmt) {
    // Las macros son expandidas en tiempo de compilación
    // No necesitan inferencia de tipos en tiempo de ejecución
    if (stmt.body) {
        stmt.body->accept(*this);
    }
}

// ============================================================
// Visitadores para Expresiones
// ============================================================

std::variant<double, std::string, bool, std::nullptr_t> TypeInferer::visitLiteralExpr(const LiteralExpr& expr) {
    auto type = inferLiteral(expr);
    exprTypes[&expr] = type;
    return expr.value;
}

std::variant<double, std::string, bool, std::nullptr_t> TypeInferer::visitBinaryExpr(const BinaryExpr& expr) {
    if (expr.left) expr.left->accept(*this);
    if (expr.right) expr.right->accept(*this);
    
    auto type = inferBinary(expr);
    exprTypes[&expr] = type;
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> TypeInferer::visitUnaryExpr(const UnaryExpr& expr) {
    if (expr.right) expr.right->accept(*this);
    
    auto type = inferUnary(expr);
    exprTypes[&expr] = type;
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> TypeInferer::visitGroupingExpr(const GroupingExpr& expr) {
    if (expr.expression) {
        expr.expression->accept(*this);
        exprTypes[&expr] = getType(*expr.expression);
    }
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> TypeInferer::visitVariableExpr(const VariableExpr& expr) {
    // Buscar tipo de la variable en la symbol table
    auto it = symbolTable.find(expr.name.lexeme);
    if (it != symbolTable.end()) {
        exprTypes[&expr] = it->second.inferredType;
    } else {
        // Variable global sin tipo inferido aún
        exprTypes[&expr] = Type::unknown();
    }
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> TypeInferer::visitAssignExpr(const AssignExpr& expr) {
    if (expr.value) expr.value->accept(*this);
    
    auto valueType = getType(*expr.value);
    exprTypes[&expr] = valueType;  // Assignment retorna el valor asignado
    
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> TypeInferer::visitLetExpr(const LetExpr& expr) {
    for (const auto& binding : expr.bindings) {
        if (binding.initializer) {
            binding.initializer->accept(*this);
        }
    }
    
    if (expr.body) expr.body->accept(*this);
    
    auto type = inferLet(expr);
    exprTypes[&expr] = type;
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> TypeInferer::visitIfExpr(const IfExpr& expr) {
    if (expr.condition) expr.condition->accept(*this);
    if (expr.thenBranch) expr.thenBranch->accept(*this);
    
    for (const auto& elif : expr.elifBranches) {
        if (elif.first) elif.first->accept(*this);
        if (elif.second) elif.second->accept(*this);
    }
    
    if (expr.elseBranch) expr.elseBranch->accept(*this);
    
    auto type = inferIf(expr);
    exprTypes[&expr] = type;
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> TypeInferer::visitWhileExpr(const WhileExpr& expr) {
    if (expr.condition) expr.condition->accept(*this);
    if (expr.body) expr.body->accept(*this);
    
    // while retorna nil (A.6.1)
    exprTypes[&expr] = Type::nil();
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> TypeInferer::visitForExpr(const ForExpr& expr) {
    if (expr.initializer) expr.initializer->accept(*this);
    if (expr.condition) expr.condition->accept(*this);
    if (expr.increment) expr.increment->accept(*this);
    if (expr.body) expr.body->accept(*this);
    
    // for retorna el valor de su cuerpo (A.6.2)
    exprTypes[&expr] = getType(*expr.body);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> TypeInferer::visitBlockExpr(const BlockExpr& expr) {
    for (const auto& e : expr.expressions) {
        if (e) e->accept(*this);
    }
    
    auto type = inferBlock(expr);
    exprTypes[&expr] = type;
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> TypeInferer::visitCallExpr(const CallExpr& expr) {
    if (expr.callee) expr.callee->accept(*this);
    
    for (const auto& arg : expr.arguments) {
        if (arg) arg->accept(*this);
    }
    
    auto type = inferCall(expr);
    exprTypes[&expr] = type;
    return 0.0;
}

} // namespace hulk