// src/backend/vm/VM.hpp
#ifndef HULK_VM_HPP
#define HULK_VM_HPP

#include <vector>
#include <stack>
#include <unordered_map>
#include "Value.hpp"
#include "CallFrame.hpp"
#include "OpCode.hpp"

namespace hulk::backend {

// Forward declarations
class VM;

// Tipo para funciones nativas
using NativeFn = Value (*)(int argCount, Value* args);

// Resultado de la interpretación
enum class InterpretResult {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

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
    
    // Obtener resultado de la última ejecución
    Value getLastResult() const { return stack.back(); }
    
    // Stack operations
    void push(Value value);
    Value pop();
    Value peek(int distance) const;
    
    // Garbage collection
    void collectGarbage();
    
private:
    // Stack (valores)
    std::vector<Value> stack;
    Value* stackTop;
    
    // Call stack
    std::vector<CallFrame> frames;
    int frameCount;
    
    // Global variables
    std::unordered_map<std::string, Value> globals;
    
    // String interning (todos los strings se internan)
    std::unordered_map<std::string, ObjString*> strings;
    
    // Garbage collector - lista de todos los objetos
    Obj* objects;
    size_t bytesAllocated;
    size_t nextGC;
    
    // Open upvalues (para closures)
    ObjUpvalue* openUpvalues;
    
    // Built-in functions
    std::unordered_map<std::string, NativeFn> natives;
    
    // VM state
    bool hadRuntimeError;
    
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
    void runtimeError(const std::string& format, ...);
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
    
    static Value nativeClock(int argCount, Value* args);
    static Value nativePrint(int argCount, Value* args);
    static Value nativeType(int argCount, Value* args);
    static Value nativeInput(int argCount, Value* args);
};

} // namespace hulk::backend

#endif // HULK_VM_HPP