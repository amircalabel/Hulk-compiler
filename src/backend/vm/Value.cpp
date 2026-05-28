// src/backend/vm/Value.cpp
#include "Value.hpp"
#include <cstring>
#include <sstream>

namespace hulk::backend {

// ============================================================
// Value implementation
// ============================================================

Value::Value(Obj* obj) {
    uint64_t rawObj = reinterpret_cast<uint64_t>(obj);
    bits = SIGN_BIT | QNAN | rawObj;
}

Obj* Value::asObj() const {
    uint64_t rawObj = bits & ~(SIGN_BIT | QNAN);
    return reinterpret_cast<Obj*>(rawObj);
}

bool Value::operator==(const Value& other) const {
    if (isNumber() && other.isNumber()) {
        return asNumber() == other.asNumber();
    }
    return bits == other.bits;
}

std::string Value::toString() const {
    if (isBool()) {
        return asBool() ? "true" : "false";
    }
    if (isNil()) {
        return "nil";
    }
    if (isNumber()) {
        double num = asNumber();
        if (num == static_cast<int64_t>(num)) {
            return std::to_string(static_cast<int64_t>(num));
        }
        return std::to_string(num);
    }
    if (isObj()) {
        Obj* obj = asObj();
        switch (obj->type) {
            case ObjType::OBJ_STRING:
                return static_cast<ObjString*>(obj)->chars;
            case ObjType::OBJ_FUNCTION:
                return "<fn " + static_cast<ObjFunction*>(obj)->name + ">";
            case ObjType::OBJ_CLOSURE:
                return "<fn>";
            case ObjType::OBJ_CLASS:
                return static_cast<ObjClass*>(obj)->name;
            case ObjType::OBJ_INSTANCE:
                return static_cast<ObjInstance*>(obj)->klass->name + " instance";
            default:
                return "<obj>";
        }
    }
    return "unknown";
}

// ============================================================
// ObjString implementation
// ============================================================

ObjString::ObjString(const std::string& str)
    : Obj(ObjType::OBJ_STRING), chars(str), length(str.size()) {
    hash = computeHash();
}

ObjString::ObjString(const char* chars, size_t length)
    : Obj(ObjType::OBJ_STRING), chars(chars, length), length(length) {
    hash = computeHash();
}

uint32_t ObjString::computeHash() const {
    uint32_t hash = 2166136261u;
    for (char c : chars) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619;
    }
    return hash;
}

// ============================================================
// ObjFunction implementation
// ============================================================

ObjFunction::ObjFunction() : Obj(ObjType::OBJ_FUNCTION) {}

// ============================================================
// ObjClosure implementation
// ============================================================

ObjClosure::ObjClosure(ObjFunction* function)
    : Obj(ObjType::OBJ_CLOSURE), function(function) {}

// ============================================================
// ObjUpvalue implementation
// ============================================================

ObjUpvalue::ObjUpvalue(Value* slot)
    : Obj(ObjType::OBJ_UPVALUE), location(slot), closed(Value::makeNil()) {}

// ============================================================
// ObjClass implementation
// ============================================================

ObjClass::ObjClass(const std::string& name)
    : Obj(ObjType::OBJ_CLASS), name(name), superclass(nullptr) {}

ObjFunction* ObjClass::findMethod(const std::string& name) const {
     // 'methods' es el nombre del campo, pero está bien definido en el struct
    // Si el error persiste, asegúrate de que el struct ObjClass tenga:
    // std::unordered_map<std::string, ObjFunction*> methods;
    
    auto it = this->methods.find(name);  // ← usa 'this->' explícitamente
    if (it != this->methods.end()) {
        return it->second;
    }
    if (superclass) {
        return superclass->findMethod(name);
    }
    return nullptr;
}

// ============================================================
// ObjInstance implementation
// ============================================================

ObjInstance::ObjInstance(ObjClass* klass)
    : Obj(ObjType::OBJ_INSTANCE), klass(klass) {}

} // namespace hulk::backend