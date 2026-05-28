// src/backend/vm/Value.hpp
#ifndef HULK_VALUE_HPP
#define HULK_VALUE_HPP

#include <cstdint>
#include <string>
#include <variant>
#include <cstring> 
#include <cmath>
#include <vector>          
#include <unordered_map>

namespace hulk::backend {

// Forward declarations
struct Obj;
struct ObjString;
struct ObjFunction;
struct ObjClosure;
struct ObjClass;
struct ObjInstance;
struct ObjUpvalue;

// ============================================================
// NaN Boxing (capítulo 30 de Crafting Interpreters)
// ============================================================

// NaN constants
constexpr uint64_t SIGN_BIT = 0x8000000000000000ULL;
constexpr uint64_t QNAN = 0x7FFC000000000000ULL;

// Type tags for NaN-boxed values
constexpr uint64_t TAG_NIL = 1;    // 01
constexpr uint64_t TAG_FALSE = 2;  // 10
constexpr uint64_t TAG_TRUE = 3;   // 11

class Value {
public:
    // Constructores
    Value() : bits(QNAN | TAG_NIL) {}
    explicit Value(bool b) : bits(b ? (QNAN | TAG_TRUE) : (QNAN | TAG_FALSE)) {}
    explicit Value(double n) : bits(bitcast<double, uint64_t>(n)) {}
    explicit Value(std::nullptr_t) : bits(QNAN | TAG_NIL) {}
    explicit Value(Obj* obj);
    
    // Type checking
    bool isBool() const { return (bits | 1) == (QNAN | TAG_TRUE); }
    bool isNil() const { return bits == (QNAN | TAG_NIL); }
    bool isNumber() const { return (bits & QNAN) != QNAN; }
    bool isObj() const { return (bits & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT); }
    
    // Type conversion
    bool asBool() const { return bits == (QNAN | TAG_TRUE); }
    double asNumber() const { return bitcast<uint64_t, double>(bits); }
    Obj* asObj() const;
    
    // Truthiness (sección A.5 - false y nil son falsey)
    bool isTruthy() const {
        if (isNil()) return false;
        if (isBool()) return asBool();
        return true;
    }
    
    // Equality
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }
    
    // Factory methods
    static Value makeBool(bool b) { return Value(b); }
    static Value makeNil() { return Value(nullptr); }
    static Value makeNumber(double n) { return Value(n); }
    static Value makeObj(Obj* obj) { return Value(obj); }
    
    // Para debugging
    std::string toString() const;
    
private:
    uint64_t bits;
    
    // Helper para bitcasting (type punning)
    template<typename T, typename U>
    static T bitcast(U u) {
        T t;
        std::memcpy(&t, &u, sizeof(T));
        return t;
    }
};

// ============================================================
// Objeto base (capítulo 19)
// ============================================================

enum class ObjType {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_UPVALUE
};

struct Obj {
    ObjType type;
    bool isMarked = false;
    Obj* next = nullptr;
    
    Obj(ObjType type) : type(type), isMarked(false), next(nullptr) {}
    virtual ~Obj() = default;
};

// ============================================================
// String (capítulo 19)
// ============================================================

struct ObjString : Obj {
    std::string chars;
    size_t length;
    uint32_t hash;
    
    ObjString(const std::string& str);
    ObjString(const char* chars, size_t length);
    
    uint32_t computeHash() const;
};

// ============================================================
// Función (capítulo 24)
// ============================================================

struct ObjFunction : Obj {
    std::string name;
    int arity = 0;
    int upvalueCount = 0;
    std::vector<uint8_t> code;      // Bytecode
    std::vector<Value> constants;   // Constant pool
    std::vector<int> lines;         // Line info for debugging
    
    ObjFunction();
};

// ============================================================
// Closure (capítulo 25)
// ============================================================

struct ObjClosure : Obj {
    ObjFunction* function;
    std::vector<ObjUpvalue*> upvalues;
    
    ObjClosure(ObjFunction* function);
};

// ============================================================
// Upvalue (capítulo 25)
// ============================================================

struct ObjUpvalue : Obj {
    Value* location;
    Value closed;
    ObjUpvalue* next = nullptr;
    
    ObjUpvalue(Value* slot);
};

// ============================================================
// Clase (capítulo 27)
// ============================================================

struct ObjClass : Obj {
    std::string name;
    ObjClass* superclass = nullptr;
    std::unordered_map<std::string, ObjFunction*> methods;
    
    ObjClass(const std::string& name);
    
    ObjFunction* findMethod(const std::string& name) const;
};

// ============================================================
// Instancia (capítulo 27)
// ============================================================

struct ObjInstance : Obj {
    ObjClass* klass;
    std::unordered_map<std::string, Value> fields;
    
    ObjInstance(ObjClass* klass);
};

} // namespace hulk::backend

#endif // HULK_VALUE_HPP