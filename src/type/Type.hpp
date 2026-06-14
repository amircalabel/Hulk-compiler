// src/type/Type.hpp
#ifndef HULK_TYPE_HPP
#define HULK_TYPE_HPP

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>

namespace hulk {

// Tipos primitivos de HULK (sección A.7, A.10)
enum class TypeKind {
    NUMBER,     // Number
    STRING,     // String
    BOOLEAN,    // Boolean
    NIL,        // nil
    OBJECT,     // Object (raíz de la jerarquía, A.7.3)
    CLASS,      // Clase definida por usuario
    PROTOCOL,   // Protocolo (A.10)
    FUNCTION,   // Tipo función
    GENERIC,    // Tipo genérico (para vectores: Number* = GENERIC con base NUMBER)
    TYPE_VAR,   // Variable de tipo (para inferencia)
    UNKNOWN,    // Tipo desconocido (para inferencia pendiente)
    ERROR       // Tipo de error
};

// Forward declaration
struct Type;

// Parámetros de tipo (para clases genéricas)
struct TypeParameter {
    std::string name;
    std::optional<std::shared_ptr<Type>> constraint;  // protocolo que debe cumplir
};

// Tipo función
struct FunctionType {
    std::vector<std::shared_ptr<Type>> parameterTypes;
    std::shared_ptr<Type> returnType;
};

// Tipo clase
struct ClassType {
    std::string name;
    std::vector<TypeParameter> typeParameters;
    std::shared_ptr<Type> superclass;  // nullptr si no tiene
    std::vector<std::pair<std::string, std::shared_ptr<Type>>> attributes;
    std::vector<std::pair<std::string, FunctionType>> methods;
};

// Tipo protocolo (A.10)
struct ProtocolType {
    std::string name;
    std::shared_ptr<ProtocolType> extends;  // protocolo del que extiende
    std::vector<std::pair<std::string, FunctionType>> methods;
};

// Tipo genérico (para Vector(T) o T*)
struct GenericType {
    std::string baseName;  // "Vector", "Iterable", etc.
    std::vector<std::shared_ptr<Type>> typeArguments;
};

// Tipo principal - puede ser cualquiera de los anteriores
struct Type {
    TypeKind kind;
    std::string name;  // para CLASS, PROTOCOL, TYPE_VAR
    
    // Campos opcionales según el tipo
    std::shared_ptr<ClassType> classInfo;
    std::shared_ptr<ProtocolType> protocolInfo;
    std::shared_ptr<FunctionType> functionInfo;
    std::shared_ptr<GenericType> genericInfo;
    std::vector<std::shared_ptr<Type>> typeArguments;  // para tipos parametrizados
    
    // Constructores para tipos primitivos
    static std::shared_ptr<Type> number() {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::NUMBER;
        t->name = "Number";
        return t;
    }
    
    static std::shared_ptr<Type> string() {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::STRING;
        t->name = "String";
        return t;
    }
    
    static std::shared_ptr<Type> boolean() {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::BOOLEAN;
        t->name = "Boolean";
        return t;
    }
    
    static std::shared_ptr<Type> nil() {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::NIL;
        t->name = "Nil";
        return t;
    }
    
    static std::shared_ptr<Type> object() {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::OBJECT;
        t->name = "Object";
        return t;
    }
    
    static std::shared_ptr<Type> unknown() {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::UNKNOWN;
        t->name = "?";
        return t;
    }
    
    static std::shared_ptr<Type> typeVar(const std::string& name) {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::TYPE_VAR;
        t->name = name;
        return t;
    }
    
    // Crear tipo clase
    static std::shared_ptr<Type> classType(const std::string& name, 
                                            std::shared_ptr<ClassType> info = nullptr) {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::CLASS;
        t->name = name;
        t->classInfo = info;
        return t;
    }
    
    // Crear tipo protocolo
    static std::shared_ptr<Type> protocolType(const std::string& name,
                                               std::shared_ptr<ProtocolType> info = nullptr) {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::PROTOCOL;
        t->name = name;
        t->protocolInfo = info;
        return t;
    }
    
    // Crear tipo función
    static std::shared_ptr<Type> functionType(std::vector<std::shared_ptr<Type>> params,
                                               std::shared_ptr<Type> ret) {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::FUNCTION;
        t->functionInfo = std::make_shared<FunctionType>();
        t->functionInfo->parameterTypes = params;
        t->functionInfo->returnType = ret;
        return t;
    }
    
    // Crear tipo genérico (ej: Vector(Number))
    static std::shared_ptr<Type> genericType(const std::string& base,
                                              const std::vector<std::shared_ptr<Type>>& args) {
        auto t = std::make_shared<Type>();
        t->kind = TypeKind::GENERIC;
        t->genericInfo = std::make_shared<GenericType>();
        t->genericInfo->baseName = base;
        t->genericInfo->typeArguments = args;
        return t;
    }
    
    // Verificar si dos tipos son iguales
    bool equals(const Type& other) const;
    
    // Verificar si este tipo conforma a otro (A.10.4 - varianza)
    bool conformsTo(const Type& other) const;
    
    std::string toString() const;
};

} // namespace hulk

#endif // HULK_TYPE_HPP