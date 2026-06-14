// src/backend/vm/VM.hpp
#ifndef HULK_VM_HPP
#define HULK_VM_HPP

#include <vector>
#include <unordered_map>
#include <functional>
#include "Value.hpp"
#include "CallFrame.hpp"
#include "OpCode.hpp"
#include "../banner/BannerIR.hpp"

namespace hulk::backend {

// Forward declarations
class VM;

// Tipo para funciones nativas (std::function allows captures safely)
using NativeFn = std::function<Value(int argCount, Value* args)>;

// Resultado de la interpretación
enum class InterpretResult {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

// Maximum number of values on the value stack (64 frames * 256 slots)
static constexpr int FRAMES_MAX = 64;
static constexpr int STACK_MAX  = FRAMES_MAX * 256;

/**
 * Virtual Machine - Ejecuta bytecode BANNER (capítulos 14-30)
 *
 * Basada en la implementación de clox del libro Crafting Interpreters
 * Adaptada para el IR BANNER de HULK
 */
class VM {
public:
    VM();
    ~VM();

    // Ejecutar una función (punto de entrada principal)
    InterpretResult interpret(ObjFunction* function);

    // Ejecutar un programa BANNER completo
    InterpretResult interpret(const BannerProgram& program);

    // Obtener resultado de la última ejecución.
    // Returns makeNil() when the stack is empty (e.g. after a runtime error).
    Value getLastResult() const {
        if (stackTop == stack) return Value::makeNil();
        return stackTop[-1];
    }

    // Stack operations
    void push(Value value);
    Value pop();
    Value peek(int distance) const;

    // Garbage collection
    void collectGarbage();

private:
    // Fixed-size value stack avoids dangling-pointer hazard from vector realloc.
    // CallFrame::slots points into this array and remains stable forever.
    Value  stack[STACK_MAX];
    Value* stackTop;   // Points one past the last pushed value

    // Call stack
    std::vector<CallFrame> frames;
    int frameCount;

    // Global variables
    std::unordered_map<std::string, Value> globals;

    // String interning (todos los strings se internan)
    std::unordered_map<std::string, ObjString*> strings;

    // Garbage collector - linked list of all heap objects
    Obj*   objects;
    size_t bytesAllocated;
    size_t nextGC;

    // Open upvalues (para closures)
    ObjUpvalue* openUpvalues;

    // Built-in functions
    std::unordered_map<std::string, NativeFn> natives;

    // VM state
    bool hadRuntimeError;

    // ============================================================
    // GC-aware object allocator
    // Every heap object MUST be created through this helper so that
    // it is registered in the GC linked list from birth.
    // ============================================================
    template<typename T, typename... Args>
    T* allocateObj(Args&&... args) {
        T* obj = new T(std::forward<Args>(args)...);
        obj->next = objects;
        objects = obj;
        bytesAllocated += sizeof(T);
        return obj;
    }

    // ============================================================
    // Bytecode execution
    // ============================================================
    InterpretResult run();

    bool call(ObjClosure* closure, int argCount);
    bool callValue(Value callee, int argCount);
    bool invokeFromClass(ObjClass* klass, const std::string& name, int argCount);
    bool invoke(const std::string& name, int argCount);

    void defineMethod(const std::string& name);

    // ============================================================
    // Upvalues (closures)
    // ============================================================
    ObjUpvalue* captureUpvalue(Value* local);
    void closeUpvalues(Value* last);

    // ============================================================
    // Runtime errors
    // ============================================================
    // Takes a printf-style format string (const char*, not std::string&)
    // to avoid the UB of passing a reference type before '...'.
    void runtimeError(const char* format, ...);
    void resetStack();

    // ============================================================
    // Garbage collection (mark-sweep)
    // ============================================================
    void markRoots();
    void markValue(Value value);
    void markObject(Obj* object);
    void markTable(std::unordered_map<std::string, Value>& table);
    void sweep();
    void freeObject(Obj* object);

    // ============================================================
    // Built-in functions
    // ============================================================
    void defineBuiltins();
};

} // namespace hulk::backend

#endif // HULK_VM_HPP
