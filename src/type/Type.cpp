// src/type/Type.cpp
#include "Type.hpp"
#include <sstream>

namespace hulk {

bool Type::equals(const Type& other) const {
    if (kind != other.kind) return false;
    
    switch (kind) {
        case TypeKind::NUMBER:
        case TypeKind::STRING:
        case TypeKind::BOOLEAN:
        case TypeKind::NIL:
        case TypeKind::OBJECT:
        case TypeKind::UNKNOWN:
            return true;
            
        case TypeKind::CLASS:
            return name == other.name;
            
        case TypeKind::PROTOCOL:
            return name == other.name;
            
        case TypeKind::FUNCTION:
            if (functionInfo->parameterTypes.size() != other.functionInfo->parameterTypes.size())
                return false;
            for (size_t i = 0; i < functionInfo->parameterTypes.size(); i++) {
                if (!functionInfo->parameterTypes[i]->equals(*other.functionInfo->parameterTypes[i]))
                    return false;
            }
            return functionInfo->returnType->equals(*other.functionInfo->returnType);
            
        case TypeKind::GENERIC:
            if (genericInfo->baseName != other.genericInfo->baseName) return false;
            if (genericInfo->typeArguments.size() != other.genericInfo->typeArguments.size())
                return false;
            for (size_t i = 0; i < genericInfo->typeArguments.size(); i++) {
                if (!genericInfo->typeArguments[i]->equals(*other.genericInfo->typeArguments[i]))
                    return false;
            }
            return true;
            
        case TypeKind::TYPE_VAR:
            return name == other.name;
            
        case TypeKind::ERROR:
            return true;
    }
    return false;
}

bool Type::conformsTo(const Type& other) const {
    // Reglas de conformidad de tipos (A.10.4)
    
    // Un tipo siempre conforma a sí mismo
    if (equals(other)) return true;
    
    // nil conforma a cualquier tipo (nil es subtipo de todo)
    if (kind == TypeKind::NIL) return true;
    
    // Number, String, Boolean son subtipos de Object (A.7.3)
    if (other.kind == TypeKind::OBJECT) {
        return kind == TypeKind::NUMBER || kind == TypeKind::STRING || 
               kind == TypeKind::BOOLEAN || kind == TypeKind::NIL;
    }
    
    // Clase -> su superclase
    if (kind == TypeKind::CLASS && other.kind == TypeKind::CLASS) {
        if (classInfo && classInfo->superclass) {
            return classInfo->superclass->conformsTo(other);
        }
        return false;
    }
    
    // Protocolo (A.10.2, A.10.4)
    if (kind == TypeKind::CLASS && other.kind == TypeKind::PROTOCOL) {
        // Una clase conforma a un protocolo si implementa todos sus métodos
        if (other.protocolInfo && classInfo) {
            for (const auto& method : other.protocolInfo->methods) {
                bool found = false;
                for (const auto& classMethod : classInfo->methods) {
                    if (classMethod.first == method.first) {
                        found = true;
                        break;
                    }
                }
                if (!found) return false;
            }
            return true;
        }
        return false;
    }
    
    // Protocolo extiende otro protocolo
    if (kind == TypeKind::PROTOCOL && other.kind == TypeKind::PROTOCOL) {
        if (protocolInfo && other.protocolInfo) {
            const ProtocolType* current = protocolInfo.get();
            while (current != nullptr) {
                if (current->name == other.protocolInfo->name) {
                    return true;
                }
                current = current->extends.get();
            }
        }
        return false;
    }
    
    // Covarianza/Contravarianza en funciones (A.10.3)
    if (kind == TypeKind::FUNCTION && other.kind == TypeKind::FUNCTION) {
        if (functionInfo->parameterTypes.size() != other.functionInfo->parameterTypes.size())
            return false;
        // Los parámetros son contravariantes
        for (size_t i = 0; i < functionInfo->parameterTypes.size(); i++) {
            if (!other.functionInfo->parameterTypes[i]->conformsTo(*functionInfo->parameterTypes[i]))
                return false;
        }
        // El retorno es covariante
        return functionInfo->returnType->conformsTo(*other.functionInfo->returnType);
    }
    
    return false;
}

std::string Type::toString() const {
    switch (kind) {
        case TypeKind::NUMBER: return "Number";
        case TypeKind::STRING: return "String";
        case TypeKind::BOOLEAN: return "Boolean";
        case TypeKind::NIL: return "Nil";
        case TypeKind::OBJECT: return "Object";
        case TypeKind::CLASS: return "class " + name;
        case TypeKind::PROTOCOL: return "protocol " + name;
        case TypeKind::FUNCTION: {
            std::stringstream ss;
            ss << "(";
            for (size_t i = 0; i < functionInfo->parameterTypes.size(); i++) {
                if (i > 0) ss << ", ";
                ss << functionInfo->parameterTypes[i]->toString();
            }
            ss << ") -> " << functionInfo->returnType->toString();
            return ss.str();
        }
        case TypeKind::GENERIC:
            return genericInfo->baseName + "<" + genericInfo->typeArguments[0]->toString() + ">";
        case TypeKind::TYPE_VAR:
            return name;
        case TypeKind::UNKNOWN:
            return "?";
        case TypeKind::ERROR:
            return "<error>";
    }
    return "unknown";
}

} // namespace hulk
