// src/backend/banner/BannerGenerator.cpp
#include "BannerGenerator.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace hulk::backend {

// ============================================================
// Constructor
// ============================================================

BannerGenerator::BannerGenerator(Resolver& resolver, TypeInferer& inferer)
    : resolver(resolver), inferer(inferer) {}

// ============================================================
// Generación principal
// ============================================================

BannerProgram BannerGenerator::generate(const std::vector<std::unique_ptr<Stmt>>& statements) {
    // Inicializar programa BANNER
    program = BannerProgram{};
    
    // Generar código para todas las declaraciones
    for (const auto& stmt : statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
    
    // Si no hay función main, crear una vacía
    bool hasMain = false;
    for (const auto& func : program.functions) {
        if (func.name == "main" || func.name == "_entry") {
            hasMain = true;
            break;
        }
    }
    
    if (!hasMain) {
        // Crear función de entrada vacía
        BannerFunction entry;
        entry.name = "_entry";
        program.functions.push_back(entry);
        program.entryFunction = "_entry";
    }
    
    return program;
}

// ============================================================
// Emisión de instrucciones
// ============================================================

void BannerGenerator::emit(const BannerInstr& instr) {
    if (ctx.currentFunction) {
        ctx.currentFunction->instructions.push_back(instr);
    }
}

void BannerGenerator::emitLabel(const std::string& name) {
    BannerInstr instr;
    instr.kind = BannerInstr::Kind::LABEL;
    instr.label = name;
    emit(instr);
}

std::string BannerGenerator::newLabel() {
    return "L" + std::to_string(ctx.labelCounter++);
}

void BannerGenerator::beginFunction(const std::string& name, const std::vector<std::string>& params) {
    BannerFunction func;
    func.name = name;
    
    // PARAM declarations (sección B.1.3 de BANNER)
    func.parameters = params;
    
    program.functions.push_back(std::move(func));
    ctx.currentFunction = &program.functions.back();
    ctx.nextLocalSlot = 0;
    ctx.nextParamSlot = 0;
    ctx.localSlots.clear();
    ctx.paramSlots.clear();
    
    // Registrar parámetros como slots
    for (const auto& param : params) {
        declareParameter(param);
    }
}

void BannerGenerator::endFunction() {
    // Emitir RETURN al final si no hay return explícito
    BannerInstr ret;
    ret.kind = BannerInstr::Kind::RETURN;
    emit(ret);
    
    ctx.currentFunction = nullptr;
}

void BannerGenerator::declareLocal(const std::string& name) {
    ctx.localSlots[name] = ctx.nextLocalSlot++;
    if (ctx.currentFunction) {
        ctx.currentFunction->locals.push_back(name);
    }
}

void BannerGenerator::declareParameter(const std::string& name) {
    ctx.paramSlots[name] = ctx.nextParamSlot++;
}

int BannerGenerator::getLocalSlot(const std::string& name) {
    auto it = ctx.localSlots.find(name);
    if (it != ctx.localSlots.end()) {
        return it->second;
    }
    
    auto pit = ctx.paramSlots.find(name);
    if (pit != ctx.paramSlots.end()) {
        return pit->second;
    }
    
    return -1;
}

void BannerGenerator::emitLoadLocal(const std::string& name) {
    int slot = getLocalSlot(name);
    BannerInstr instr;
    instr.kind = BannerInstr::Kind::LOAD;
    if (slot >= 0) {
        instr.varName = name;
        instr.index = slot;
    } else {
        // Variable global
        instr.varName = name;
    }
    emit(instr);
}

void BannerGenerator::emitStoreLocal(const std::string& name) {
    int slot = getLocalSlot(name);
    BannerInstr instr;
    instr.kind = BannerInstr::Kind::STORE;
    instr.varName = name;
    if (slot >= 0) {
        instr.index = slot;
    }
    emit(instr);
}

void BannerGenerator::emitLoadConstant(double value) {
    BannerInstr instr;
    instr.kind = BannerInstr::Kind::LOAD;
    instr.number = value;
    emit(instr);
}

void BannerGenerator::emitLoadConstant(const std::string& str) {
    std::string label = internString(str);
    BannerInstr instr;
    instr.kind = BannerInstr::Kind::LOAD;
    instr.label = label;
    emit(instr);
}

void BannerGenerator::emitLoadConstant(bool value) {
    BannerInstr instr;
    instr.kind = BannerInstr::Kind::LOAD;
    instr.number = value ? 1.0 : 0.0;
    emit(instr);
}

void BannerGenerator::emitLoadConstant(std::nullptr_t) {
    BannerInstr instr;
    instr.kind = BannerInstr::Kind::LOAD;
    instr.number = 0.0;
    emit(instr);
}

std::string BannerGenerator::internString(const std::string& str) {
    // Buscar si ya existe
    for (const auto& data : program.data) {
        if (data.value == str) {
            return data.label;
        }
    }
    
    // Crear nueva entrada en .DATA
    BannerDataEntry entry;
    entry.label = "s" + std::to_string(program.data.size());
    entry.value = str;
    program.data.push_back(entry);
    
    return entry.label;
}

// ============================================================
// Generación de .TYPES (clases y objetos)
// ============================================================

void BannerGenerator::generateTypeLayout(const ClassDeclStmt& stmt) {
    BannerTypeEntry typeEntry;
    typeEntry.name = stmt.name.lexeme;
    
    // Recolectar atributos (incluyendo herencia)
    for (const auto& attr : stmt.attributes) {
        typeEntry.attributes.push_back(attr.first.lexeme);
    }
    
    // Recolectar métodos
    for (const auto& method : stmt.methods) {
        typeEntry.methods[method->name.lexeme] = method->name.lexeme + "_" + stmt.name.lexeme;
    }
    
    program.types.push_back(typeEntry);
}

void BannerGenerator::generateVTable(const ClassDeclStmt& stmt) {
    // Generar vtable para el tipo (para llamadas virtuales)
    // Esto se usa en instrucciones VCALL
    // La implementación puede variar según la VM
}

// ============================================================
// Visitadores para Statements
// ============================================================

void BannerGenerator::visitExpressionStmt(const ExpressionStmt& stmt) {
    if (stmt.expression) {
        stmt.expression->accept(*this);
        // Discard result (POP en BANNER)
        BannerInstr pop;
        pop.kind = BannerInstr::Kind::STORE;
        pop.varName = "_";
        emit(pop);
    }
}

void BannerGenerator::visitPrintStmt(const PrintStmt& stmt) {
    if (stmt.expression) {
        stmt.expression->accept(*this);
        BannerInstr print;
        print.kind = BannerInstr::Kind::PRINT;
        emit(print);
    }
}

void BannerGenerator::visitReturnStmt(const ReturnStmt& stmt) {
    if (stmt.value) {
        stmt.value->accept(*this);
    } else {
        emitLoadConstant(nullptr);
    }
    
    BannerInstr ret;
    ret.kind = BannerInstr::Kind::RETURN;
    emit(ret);
}

void BannerGenerator::visitBlockStmt(const BlockStmt& stmt) {
    for (const auto& s : stmt.statements) {
        if (s) {
            s->accept(*this);
        }
    }
}

void BannerGenerator::visitVarDeclStmt(const VarDeclStmt& stmt) {
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    } else {
        emitLoadConstant(nullptr);
    }
    
    emitStoreLocal(stmt.name.lexeme);
    declareLocal(stmt.name.lexeme);
}

void BannerGenerator::visitFunctionDeclStmt(const FunctionDeclStmt& stmt) {
    // Recolectar nombres de parámetros
    std::vector<std::string> paramNames;
    for (const auto& param : stmt.parameters) {
        paramNames.push_back(param.name.lexeme);
    }
    
    beginFunction(stmt.name.lexeme, paramNames);
    
    // Registrar parámetros como locales
    for (const auto& param : stmt.parameters) {
        declareLocal(param.name.lexeme);
    }
    
    // Generar código para el cuerpo
    for (const auto& bodyStmt : stmt.body) {
        if (bodyStmt) {
            bodyStmt->accept(*this);
        }
    }
    
    endFunction();
}

void BannerGenerator::visitClassDeclStmt(const ClassDeclStmt& stmt) {
    // Registrar el tipo en .TYPES
    generateTypeLayout(stmt);
    
    // Si tiene superclase, registrar relación
    if (stmt.superclass.type != TokenType::TOKEN_ERROR) {
        // Buscar superclase en tipos registrados
        for (auto& type : program.types) {
            if (type.name == stmt.superclass.lexeme) {
                // Heredar atributos y métodos
                for (const auto& attr : type.attributes) {
                    // Verificar si ya existe en la subclase
                    bool exists = false;
                    for (const auto& a : stmt.attributes) {
                        if (a.first.lexeme == attr) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        // Agregar atributo heredado
                        // Esto se maneja en la VM en tiempo de ejecución
                    }
                }
                break;
            }
        }
    }
    
    // Generar métodos
    for (const auto& method : stmt.methods) {
        if (method) {
            method->accept(*this);
        }
    }
}

void BannerGenerator::visitProtocolDeclStmt(const ProtocolDeclStmt& stmt) {
    // Los protocolos son solo para type checking
    // No generan código BANNER directamente
    // Se usan en el TypeInferer
}

void BannerGenerator::visitMacroDeclStmt(const MacroDeclStmt& stmt) {
    // Las macros se expanden en tiempo de compilación
    // Por ahora, generar el cuerpo como una función normal
    std::vector<std::string> paramNames;
    for (const auto& param : stmt.parameters) {
        paramNames.push_back(param.name.lexeme);
    }
    
    beginFunction(stmt.name.lexeme, paramNames);
    
    if (stmt.body) {
        stmt.body->accept(*this);
    }
    
    endFunction();
}

// ============================================================
// Visitadores para Expresiones
// ============================================================

std::variant<double, std::string, bool, std::nullptr_t> 
BannerGenerator::visitLiteralExpr(const LiteralExpr& expr) {
    if (std::holds_alternative<double>(expr.value)) {
        emitLoadConstant(std::get<double>(expr.value));
    } else if (std::holds_alternative<std::string>(expr.value)) {
        emitLoadConstant(std::get<std::string>(expr.value));
    } else if (std::holds_alternative<bool>(expr.value)) {
        emitLoadConstant(std::get<bool>(expr.value));
    } else {
        emitLoadConstant(nullptr);
    }
    return expr.value;
}

std::variant<double, std::string, bool, std::nullptr_t> 
BannerGenerator::visitBinaryExpr(const BinaryExpr& expr) {
    expr.left->accept(*this);
    expr.right->accept(*this);
    
    BannerInstr instr;
    
    switch (expr.op.type) {
        case TokenType::TOKEN_PLUS:
            instr.kind = BannerInstr::Kind::ADD;
            break;
        case TokenType::TOKEN_MINUS:
            instr.kind = BannerInstr::Kind::SUB;
            break;
        case TokenType::TOKEN_STAR:
            instr.kind = BannerInstr::Kind::MUL;
            break;
        case TokenType::TOKEN_SLASH:
            instr.kind = BannerInstr::Kind::DIV;
            break;
        case TokenType::TOKEN_CARET:
            instr.kind = BannerInstr::Kind::POW;
            break;
        case TokenType::TOKEN_EQUAL_EQUAL:
            // Implementar como comparación
            instr.kind = BannerInstr::Kind::CALL;
            instr.varName = "__eq";
            break;
        default:
            return 0.0;
    }
    
    emit(instr);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
BannerGenerator::visitUnaryExpr(const UnaryExpr& expr) {
    expr.right->accept(*this);
    
    BannerInstr instr;
    
    switch (expr.op.type) {
        case TokenType::TOKEN_MINUS:
            instr.kind = BannerInstr::Kind::SUB;
            instr.label = "0";
            // Negación: 0 - valor
            emitLoadConstant(0.0);
            emit(instr);
            break;
        case TokenType::TOKEN_BANG:
            // NOT lógico
            instr.kind = BannerInstr::Kind::CALL;
            instr.varName = "__not";
            emit(instr);
            break;
        default:
            break;
    }
    
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
BannerGenerator::visitGroupingExpr(const GroupingExpr& expr) {
    if (expr.expression) {
        expr.expression->accept(*this);
    }
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
BannerGenerator::visitVariableExpr(const VariableExpr& expr) {
    emitLoadLocal(expr.name.lexeme);
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t>
BannerGenerator::visitAssignExpr(const AssignExpr& expr) {
    // Evaluate RHS once
    expr.value->accept(*this);

    // STORE pops the value into the variable
    BannerInstr store;
    store.kind    = BannerInstr::Kind::STORE;
    store.varName = expr.name.lexeme;
    emit(store);

    // ISSUE-18 fix: don't re-evaluate RHS (would double side-effects).
    // Reload the stored value so the assignment expression leaves a result on the stack.
    emitLoadLocal(expr.name.lexeme);

    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
BannerGenerator::visitLetExpr(const LetExpr& expr) {
    beginScope();
    
    for (const auto& binding : expr.bindings) {
        if (binding.initializer) {
            binding.initializer->accept(*this);
        } else {
            emitLoadConstant(nullptr);
        }
        emitStoreLocal(binding.name.lexeme);
        declareLocal(binding.name.lexeme);
    }
    
    if (expr.body) {
        expr.body->accept(*this);
    }
    
    endScope();
    
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
BannerGenerator::visitIfExpr(const IfExpr& expr) {
    std::string elseLabel = newLabel();
    std::string endLabel = newLabel();
    
    // Condición
    expr.condition->accept(*this);
    
    // IF_GOTO para saltar al else si condición es falsa
    BannerInstr ifGoto;
    ifGoto.kind = BannerInstr::Kind::IF_GOTO;
    ifGoto.label = elseLabel;
    emit(ifGoto);
    
    // Then branch
    expr.thenBranch->accept(*this);
    
    // GOTO al final
    BannerInstr gotoEnd;
    gotoEnd.kind = BannerInstr::Kind::GOTO;
    gotoEnd.label = endLabel;
    emit(gotoEnd);
    
    // Else branch
    emitLabel(elseLabel);
    if (expr.elseBranch) {
        expr.elseBranch->accept(*this);
    } else {
        emitLoadConstant(nullptr);
    }
    
    // Elif branches (como else if anidados)
    for (const auto& elif : expr.elifBranches) {
        std::string nextElseLabel = newLabel();
        std::string nextEndLabel = newLabel();
        
        elif.first->accept(*this);
        
        BannerInstr elifGoto;
        elifGoto.kind = BannerInstr::Kind::IF_GOTO;
        elifGoto.label = nextElseLabel;
        emit(elifGoto);
        
        elif.second->accept(*this);
        
        BannerInstr gotoNextEnd;
        gotoNextEnd.kind = BannerInstr::Kind::GOTO;
        gotoNextEnd.label = nextEndLabel;
        emit(gotoNextEnd);
        
        emitLabel(nextElseLabel);
        // Continuar con el siguiente elif o else
    }
    
    emitLabel(endLabel);
    
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
BannerGenerator::visitWhileExpr(const WhileExpr& expr) {
    std::string loopLabel = newLabel();
    std::string exitLabel = newLabel();
    
    ctx.breakLabels.push(exitLabel);
    ctx.continueLabels.push(loopLabel);
    
    emitLabel(loopLabel);
    
    // Condición
    expr.condition->accept(*this);
    
    // IF_GOTO para salir si condición es falsa
    BannerInstr ifGoto;
    ifGoto.kind = BannerInstr::Kind::IF_GOTO;
    ifGoto.label = exitLabel;
    emit(ifGoto);
    
    // Cuerpo
    expr.body->accept(*this);
    
    // GOTO al inicio
    BannerInstr gotoLoop;
    gotoLoop.kind = BannerInstr::Kind::GOTO;
    gotoLoop.label = loopLabel;
    emit(gotoLoop);
    
    emitLabel(exitLabel);
    
    ctx.breakLabels.pop();
    ctx.continueLabels.pop();
    
    // while retorna nil
    emitLoadConstant(nullptr);
    
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
BannerGenerator::visitForExpr(const ForExpr& expr) {
    beginScope();
    
    // Inicializador
    if (expr.initializer) {
        expr.initializer->accept(*this);
        BannerInstr pop;
        pop.kind = BannerInstr::Kind::STORE;
        pop.varName = "_";
        emit(pop);
    }
    
    std::string loopLabel = newLabel();
    std::string exitLabel = newLabel();
    std::string incrementLabel = newLabel();
    
    ctx.breakLabels.push(exitLabel);
    ctx.continueLabels.push(incrementLabel);
    
    emitLabel(loopLabel);
    
    // Condición (si no hay, es bucle infinito)
    if (expr.condition) {
        expr.condition->accept(*this);
        
        BannerInstr ifGoto;
        ifGoto.kind = BannerInstr::Kind::IF_GOTO;
        ifGoto.label = exitLabel;
        emit(ifGoto);
    }
    
    // Cuerpo
    if (expr.body) {
        expr.body->accept(*this);
        // Descartar resultado del cuerpo
        BannerInstr pop;
        pop.kind = BannerInstr::Kind::STORE;
        pop.varName = "_";
        emit(pop);
    }
    
    // Incremento
    if (expr.increment) {
        emitLabel(incrementLabel);
        expr.increment->accept(*this);
        BannerInstr pop;
        pop.kind = BannerInstr::Kind::STORE;
        pop.varName = "_";
        emit(pop);
    }
    
    // GOTO al inicio
    BannerInstr gotoLoop;
    gotoLoop.kind = BannerInstr::Kind::GOTO;
    gotoLoop.label = loopLabel;
    emit(gotoLoop);
    
    emitLabel(exitLabel);
    
    ctx.breakLabels.pop();
    ctx.continueLabels.pop();
    
    endScope();
    
    // for retorna nil
    emitLoadConstant(nullptr);
    
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
BannerGenerator::visitBlockExpr(const BlockExpr& expr) {
    beginScope();
    
    for (const auto& e : expr.expressions) {
        if (e) {
            e->accept(*this);
        }
    }
    
    endScope();
    
    return 0.0;
}

std::variant<double, std::string, bool, std::nullptr_t> 
BannerGenerator::visitCallExpr(const CallExpr& expr) {
    // Evaluar callee
    if (expr.callee) {
        expr.callee->accept(*this);
    }
    
    // PARAM para cada argumento (en orden inverso para stack)
    for (const auto& arg : expr.arguments) {
        if (arg) {
            arg->accept(*this);
            BannerInstr param;
            param.kind = BannerInstr::Kind::PARAM;
            param.varName = "arg";
            emit(param);
        }
    }
    
    // CALL o VCALL según el tipo
    BannerInstr call;
    
    // Determinar si es llamada virtual o estática
    // Por simplicidad, usamos CALL por ahora
    call.kind = BannerInstr::Kind::CALL;
    
    if (expr.callee) {
        // Extraer nombre de la función si es VariableExpr
        // Esto es simplificado
        call.varName = "__call";
    }
    
    emit(call);
    
    return 0.0;
}

// ============================================================
// Helpers de ámbito
// ============================================================

void BannerGenerator::beginScope() {
    ctx.scopeDepth++;
    // Record the slot watermark so endScope knows which slots belong to this scope
    ctx.scopeSlotStack.push(ctx.nextLocalSlot);
}

void BannerGenerator::endScope() {
    // ISSUE-27 fix: use the saved watermark to identify locals from this scope
    int scopeBase = ctx.scopeSlotStack.empty() ? 0 : ctx.scopeSlotStack.top();
    if (!ctx.scopeSlotStack.empty()) ctx.scopeSlotStack.pop();

    std::vector<std::string> toRemove;
    for (const auto& [name, slot] : ctx.localSlots) {
        if (slot >= scopeBase) {
            toRemove.push_back(name);
        }
    }

    for (const auto& name : toRemove) {
        ctx.localSlots.erase(name);
    }

    ctx.nextLocalSlot = scopeBase;
    ctx.scopeDepth--;
}

// ============================================================
// Manejo de errores
// ============================================================

void BannerGenerator::error(const Token& token, const std::string& message) {
    std::cerr << "[line " << token.line << "] Banner Generation Error: " << message << std::endl;
    // Marcar error global
}

} // namespace hulk::backend