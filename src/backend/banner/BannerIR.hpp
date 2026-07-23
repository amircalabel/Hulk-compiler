// src/backend/banner/BannerIR.hpp
#ifndef HULK_BANNER_IR_HPP
#define HULK_BANNER_IR_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>

namespace hulk::backend {

// Tipos de datos en BANNER (sección B.1)
enum class BannerType {
    NUMBER,     // 32-bit float (realmente double en nuestra VM)
    STRING,
    BOOLEAN,
    OBJECT,     // Referencia a objeto en heap
    VOID
};

// ============================================================
// Instrucciones BANNER (sección B.2)
// ============================================================

// Instrucciones de movimiento y aritmética
struct BannerInstr {
    enum Kind {
        // Data movement
        LOAD,           // LOAD const_label -> push constante
        STORE,          // STORE var_name -> pop a variable
        COPY,           // COPY src dst -> copiar valor
        
        // Arithmetic (tres direcciones)
        ADD, SUB, MUL, DIV, POW,
        
        // Memory management
        ALLOCATE,       // ALLOCATE type_name -> new objeto
        ARRAY,          // ARRAY size -> new array
        
        // Object interaction
        GETATTR,        // GETATTR obj attr_name -> push atributo
        SETATTR,        // SETATTR obj attr_name value -> setear atributo
        
        // Control flow
        LABEL,          // LABEL name -> punto de salto
        GOTO,           // GOTO label -> salto incondicional
        IF_GOTO,        // IF_GOTO cond label -> salto condicional
        
        // Function calls
        PARAM,          // PARAM value -> push parámetro
        CALL,           // CALL func_name -> llamada a función
        VCALL,          // VCALL type_name method_name -> llamada virtual
        RETURN,         // RETURN -> retornar de función
        
        // Built-ins
        PRINT,          // PRINT -> imprimir valor en TOS
        
        // VM control
        HALT            // HALT -> detener ejecución
    } kind;
    
    // Operandos (varían según instrucción)
    std::string label;      // Para LOAD, LABEL, GOTO, etc.
    std::string varName;    // Para STORE, GETATTR, SETATTR
    std::string typeName;   // Para ALLOCATE, VCALL
    int index;              // Para índices
    double number;          // Para constantes numéricas

    std::string toString() const;
};

// ============================================================
// Secciones BANNER (sección B.1)
// ============================================================

// .TYPES - Layout de objetos (sección B.1.1)
struct BannerTypeEntry {
    std::string name;
    std::vector<std::string> attributes;  // Atributos heredados + propios
    std::unordered_map<std::string, std::string> methods;  // method_name -> function_label
};

// .DATA - Pool de constantes (sección B.1.2)
struct BannerDataEntry {
    std::string label;
    std::string value;  // string literal
};

// .CODE - Código ejecutable (sección B.1.3)
struct BannerFunction {
    std::string name;
    std::vector<std::string> parameters;  // PARAM variables
    std::vector<std::string> locals;      // LOCAL variables
    std::vector<BannerInstr> instructions;
};

// Programa BANNER completo
struct BannerProgram {
    std::vector<BannerTypeEntry> types;
    std::vector<BannerDataEntry> data;
    std::vector<BannerFunction> functions;
    
    // Función de entrada (top-level)
    std::string entryFunction = "main";
    
    std::string toString() const;
};

} // namespace hulk::backend

#endif // HULK_BANNER_IR_HPP
