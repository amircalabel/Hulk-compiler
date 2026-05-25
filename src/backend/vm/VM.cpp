// src/backend/vm/VM.cpp
#include "VM.hpp"
#include <iostream>
#include <cmath>
#include <cstdarg>
#include <ctime>

namespace hulk::backend {

// ============================================================
// Constructor / Destructor
// ============================================================

VM::VM() 
    : stackTop(nullptr), frameCount(0), objects(nullptr),
      bytesAllocated(0), nextGC(1024 * 1024), openUpvalues(nullptr),
      hadRuntimeError(false) {
    
    // Inicializar stack
    stack.reserve(256);
    stackTop = stack.data();
    
    // Inicializar frames
    frames.reserve(64);
    
    // Definir built-ins
    defineBuiltins();
}

VM::~VM() {
    // Liberar todos los objetos (GC)
    Obj* obj = objects;
    while (obj) {
        Obj* next = obj->next;
        freeObject(obj);
        obj = next;
    }
}

// ============================================================
// Stack operations
// ============================================================

void VM::push(Value value) {
    if (stack.size() >= stack.capacity()) {
        stack.reserve(stack.capacity() * 2);
        // Actualizar stackTop después de realloc
        stackTop = stack.data() + stack.size();
    }
    stack.push_back(value);
    stackTop = stack.data() + stack.size();
}

Value VM::pop() {
    Value value = stack.back();
    stack.pop_back();
    stackTop = stack.data() + stack.size();
    return value;
}

Value VM::peek(int distance) const {
    return stack[stack.size() - 1 - distance];
}

// ============================================================
// Bytecode execution (run)
// ============================================================

InterpretResult VM::interpret(ObjFunction* function) {
    // Crear closure para la función
    ObjClosure* closure = new ObjClosure(function);
    push(Value::makeObj(closure));
    
    // Crear CallFrame para el top-level
    CallFrame frame(closure, stack.data());
    frames.push_back(frame);
    frameCount++;
    
    return run();
}

InterpretResult VM::interpret(const BannerProgram& program) {
    // TODO: Compilar programa BANNER a bytecode interno
    // Por ahora, placeholder
    return InterpretResult::INTERPRET_OK;
}

InterpretResult VM::run() {
    CallFrame* frame = &frames.back();
    
    // Macros para leer bytecode (como en clox)
    #define READ_BYTE() (*frame->ip++)
    #define READ_SHORT() \
        (frame->ip += 2, \
         (static_cast<uint16_t>((frame->ip[-2] << 8) | frame->ip[-1])))
    #define READ_CONSTANT() \
        (frame->closure->function->constants[READ_BYTE()])
    #define READ_STRING() \
        (static_cast<ObjString*>(READ_CONSTANT().asObj())->chars)
    
    for (;;) {
        #ifdef DEBUG_TRACE_EXECUTION
            // Disassembler para debugging
            std::cout << "Stack: ";
            for (size_t i = 0; i < stack.size(); i++) {
                std::cout << stack[i].toString() << " ";
            }
            std::cout << std::endl;
        #endif
        
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            
            // ====================================================
            // Constants
            // ====================================================
            case static_cast<uint8_t>(OpCode::OP_CONSTANT): {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_NIL):
                push(Value::makeNil());
                break;
                
            case static_cast<uint8_t>(OpCode::OP_TRUE):
                push(Value::makeBool(true));
                break;
                
            case static_cast<uint8_t>(OpCode::OP_FALSE):
                push(Value::makeBool(false));
                break;
            
            // ====================================================
            // Stack manipulation
            // ====================================================
            case static_cast<uint8_t>(OpCode::OP_POP):
                pop();
                break;
                
            case static_cast<uint8_t>(OpCode::OP_POPN): {
                int count = READ_BYTE();
                for (int i = 0; i < count; i++) {
                    pop();
                }
                break;
            }
            
            // ====================================================
            // Arithmetic
            // ====================================================
            case static_cast<uint8_t>(OpCode::OP_ADD): {
                Value b = pop();
                Value a = pop();
                
                if (a.isNumber() && b.isNumber()) {
                    push(Value::makeNumber(a.asNumber() + b.asNumber()));
                } else if (a.isObj() && b.isObj()) {
                    ObjString* aStr = static_cast<ObjString*>(a.asObj());
                    ObjString* bStr = static_cast<ObjString*>(b.asObj());
                    if (aStr->type == ObjType::OBJ_STRING && 
                        bStr->type == ObjType::OBJ_STRING) {
                        push(Value::makeObj(new ObjString(aStr->chars + bStr->chars)));
                    } else {
                        runtimeError("Operands must be numbers or strings.");
                        return InterpretResult::INTERPRET_RUNTIME_ERROR;
                    }
                } else {
                    runtimeError("Operands must be numbers or strings.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_SUBTRACT): {
                double b = pop().asNumber();
                double a = pop().asNumber();
                push(Value::makeNumber(a - b));
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_MULTIPLY): {
                double b = pop().asNumber();
                double a = pop().asNumber();
                push(Value::makeNumber(a * b));
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_DIVIDE): {
                double b = pop().asNumber();
                double a = pop().asNumber();
                if (b == 0) {
                    runtimeError("Division by zero.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(Value::makeNumber(a / b));
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_NEGATE): {
                Value value = peek(0);
                if (!value.isNumber()) {
                    runtimeError("Operand must be a number.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(Value::makeNumber(-pop().asNumber()));
                break;
            }
            
            // ====================================================
            // Logical
            // ====================================================
            case static_cast<uint8_t>(OpCode::OP_NOT): {
                push(Value::makeBool(!peek(0).isTruthy()));
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_EQUAL): {
                Value b = pop();
                Value a = pop();
                push(Value::makeBool(a == b));
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_GREATER): {
                double b = pop().asNumber();
                double a = pop().asNumber();
                push(Value::makeBool(a > b));
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_LESS): {
                double b = pop().asNumber();
                double a = pop().asNumber();
                push(Value::makeBool(a < b));
                break;
            }
            
            // ====================================================
            // Variables (globales y locales)
            // ====================================================
            case static_cast<uint8_t>(OpCode::OP_GET_GLOBAL): {
                std::string name = READ_STRING();
                auto it = globals.find(name);
                if (it == globals.end()) {
                    runtimeError("Undefined variable '%s'.", name.c_str());
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(it->second);
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_DEFINE_GLOBAL): {
                std::string name = READ_STRING();
                globals[name] = peek(0);
                pop();
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_SET_GLOBAL): {
                std::string name = READ_STRING();
                auto it = globals.find(name);
                if (it == globals.end()) {
                    runtimeError("Undefined variable '%s'.", name.c_str());
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                it->second = peek(0);
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_GET_LOCAL): {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_SET_LOCAL): {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_GET_UPVALUE): {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_SET_UPVALUE): {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            
            // ====================================================
            // Classes and objects
            // ====================================================
            case static_cast<uint8_t>(OpCode::OP_CLASS): {
                std::string name = READ_STRING();
                ObjClass* klass = new ObjClass(name);
                push(Value::makeObj(klass));
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_INHERIT): {
                Value superclass = peek(1);
                if (!superclass.isObj() || 
                    superclass.asObj()->type != ObjType::OBJ_CLASS) {
                    runtimeError("Superclass must be a class.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                ObjClass* subclass = static_cast<ObjClass*>(peek(0).asObj());
                ObjClass* super = static_cast<ObjClass*>(superclass.asObj());
                
                // Copy down inheritance (optimization from clox)
                subclass->superclass = super;
                for (const auto& [name, method] : super->methods) {
                    if (subclass->methods.find(name) == subclass->methods.end()) {
                        subclass->methods[name] = method;
                    }
                }
                pop();
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_METHOD): {
                std::string name = READ_STRING();
                Value method = peek(0);
                ObjClass* klass = static_cast<ObjClass*>(peek(1).asObj());
                klass->methods[name] = static_cast<ObjFunction*>(method.asObj());
                pop();
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_GET_PROPERTY): {
                Value instance = peek(0);
                if (!instance.isObj() || 
                    instance.asObj()->type != ObjType::OBJ_INSTANCE) {
                    runtimeError("Only instances have properties.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                
                ObjInstance* inst = static_cast<ObjInstance*>(instance.asObj());
                std::string name = READ_STRING();
                
                // Check field first (fields shadow methods)
                auto it = inst->fields.find(name);
                if (it != inst->fields.end()) {
                    pop();
                    push(it->second);
                    break;
                }
                
                // Then check method
                ObjFunction* method = inst->klass->findMethod(name);
                if (method) {
                    pop();
                    push(Value::makeObj(method));
                    break;
                }
                
                runtimeError("Undefined property '%s'.", name.c_str());
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }
            
            case static_cast<uint8_t>(OpCode::OP_SET_PROPERTY): {
                Value instance = peek(1);
                if (!instance.isObj() || 
                    instance.asObj()->type != ObjType::OBJ_INSTANCE) {
                    runtimeError("Only instances have fields.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                
                ObjInstance* inst = static_cast<ObjInstance*>(instance.asObj());
                std::string name = READ_STRING();
                Value value = peek(0);
                
                inst->fields[name] = value;
                pop();  // Remove instance
                // Value stays on stack (assignment expression)
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_INVOKE): {
                std::string name = READ_STRING();
                int argCount = READ_BYTE();
                if (!invoke(name, argCount)) {
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                frame = &frames.back();
                break;
            }
            
            // ====================================================
            // Functions and calls
            // ====================================================
            case static_cast<uint8_t>(OpCode::OP_CLOSURE): {
                ObjFunction* function = static_cast<ObjFunction*>(READ_CONSTANT().asObj());
                ObjClosure* closure = new ObjClosure(function);
                
                // Load upvalues
                for (int i = 0; i < function->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues.push_back(captureUpvalue(frame->slots + index));
                    } else {
                        closure->upvalues.push_back(frame->closure->upvalues[index]);
                    }
                }
                
                push(Value::makeObj(closure));
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_CALL): {
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                frame = &frames.back();
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_RETURN): {
                Value result = pop();
                frames.pop_back();
                frameCount--;
                
                if (frames.empty()) {
                    pop();  // Remove closure
                    return InterpretResult::INTERPRET_OK;
                }
                
                // Restore stack top
                stackTop = stack.data() + stack.size();
                push(result);
                frame = &frames.back();
                break;
            }
            
            // ====================================================
            // Control flow
            // ====================================================
            case static_cast<uint8_t>(OpCode::OP_JUMP): {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_JUMP_IF_FALSE): {
                uint16_t offset = READ_SHORT();
                if (!peek(0).isTruthy()) {
                    frame->ip += offset;
                }
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_LOOP): {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            
            case static_cast<uint8_t>(OpCode::OP_CLOSE_UPVALUE): {
                closeUpvalues(stack.data() + stack.size() - 1);
                pop();
                break;
            }
            
            // ====================================================
            // I/O
            // ====================================================
            case static_cast<uint8_t>(OpCode::OP_PRINT): {
                Value value = pop();
                std::cout << value.toString() << std::endl;
                break;
            }
            
            // ====================================================
            // VM control
            // ====================================================
            case static_cast<uint8_t>(OpCode::OP_HALT):
                return InterpretResult::INTERPRET_OK;
                
            default:
                runtimeError("Unknown opcode %d", instruction);
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        
        // Check for garbage collection
        if (bytesAllocated > nextGC) {
            collectGarbage();
        }
    }
    
    #undef READ_BYTE
    #undef READ_SHORT
    #undef READ_CONSTANT
    #undef READ_STRING
}

// ============================================================
// Function calls
// ============================================================

bool VM::call(ObjClosure* closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d.",
                     closure->function->arity, argCount);
        return false;
    }
    
    if (frameCount >= 64) {
        runtimeError("Stack overflow.");
        return false;
    }
    
    // Create new CallFrame
    CallFrame frame(closure, stackTop - argCount - 1);
    frames.push_back(frame);
    frameCount++;
    
    return true;
}

bool VM::callValue(Value callee, int argCount) {
    if (callee.isObj()) {
        Obj* obj = callee.asObj();
        switch (obj->type) {
            case ObjType::OBJ_CLOSURE:
                return call(static_cast<ObjClosure*>(obj), argCount);
            case ObjType::OBJ_CLASS: {
                ObjClass* klass = static_cast<ObjClass*>(obj);
                ObjInstance* instance = new ObjInstance(klass);
                
                // Store instance in slot 0
                stackTop[-argCount - 1] = Value::makeObj(instance);
                
                // Look for initializer
                ObjFunction* initializer = klass->findMethod("init");
                if (initializer) {
                    ObjClosure* closure = new ObjClosure(initializer);
                    return call(closure, argCount);
                } else if (argCount != 0) {
                    runtimeError("Expected 0 arguments but got %d.", argCount);
                    return false;
                }
                return true;
            }
            case ObjType::OBJ_NATIVE: {
                auto it = natives.find(static_cast<ObjString*>(obj)->chars);
                if (it != natives.end()) {
                    Value result = it->second(argCount, stackTop - argCount);
                    stackTop -= argCount + 1;
                    push(result);
                    return true;
                }
                break;
            }
            default:
                break;
        }
    }
    
    runtimeError("Can only call functions and classes.");
    return false;
}

bool VM::invoke(const std::string& name, int argCount) {
    Value receiver = peek(argCount);
    if (!receiver.isObj() || receiver.asObj()->type != ObjType::OBJ_INSTANCE) {
        runtimeError("Only instances have methods.");
        return false;
    }
    
    ObjInstance* instance = static_cast<ObjInstance*>(receiver.asObj());
    
    // Check field first (field can shadow method)
    auto it = instance->fields.find(name);
    if (it != instance->fields.end()) {
        stackTop[-argCount - 1] = it->second;
        return callValue(it->second, argCount);
    }
    
    return invokeFromClass(instance->klass, name, argCount);
}

bool VM::invokeFromClass(ObjClass* klass, const std::string& name, int argCount) {
    ObjFunction* method = klass->findMethod(name);
    if (!method) {
        runtimeError("Undefined method '%s'.", name.c_str());
        return false;
    }
    
    ObjClosure* closure = new ObjClosure(method);
    return call(closure, argCount);
}

// ============================================================
// Upvalues (closures)
// ============================================================

ObjUpvalue* VM::captureUpvalue(Value* local) {
    // Check if upvalue already exists
    ObjUpvalue* prev = nullptr;
    ObjUpvalue* upvalue = openUpvalues;
    
    while (upvalue && upvalue->location > local) {
        prev = upvalue;
        upvalue = upvalue->next;
    }
    
    if (upvalue && upvalue->location == local) {
        return upvalue;
    }
    
    // Create new upvalue
    ObjUpvalue* created = new ObjUpvalue(local);
    created->next = upvalue;
    
    if (prev) {
        prev->next = created;
    } else {
        openUpvalues = created;
    }
    
    return created;
}

void VM::closeUpvalues(Value* last) {
    while (openUpvalues && openUpvalues->location >= last) {
        ObjUpvalue* upvalue = openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        openUpvalues = upvalue->next;
    }
}

// ============================================================
// Runtime errors
// ============================================================

void VM::runtimeError(const std::string& format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format.c_str(), args);
    va_end(args);
    fprintf(stderr, "\n");
    
    // Print stack trace
    for (int i = frameCount - 1; i >= 0; i--) {
        CallFrame& frame = frames[i];
        ObjFunction* function = frame.closure->function;
        size_t instruction = frame.ip - function->code.data() - 1;
        
        fprintf(stderr, "[line %d] in ",
                function->lines[instruction]);
        
        if (function->name.empty()) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name.c_str());
        }
    }
    
    hadRuntimeError = true;
    resetStack();
}

void VM::resetStack() {
    stack.clear();
    stackTop = stack.data();
    frames.clear();
    frameCount = 0;
}

// ============================================================
// Garbage Collection (mark-sweep, capítulo 26)
// ============================================================

void VM::collectGarbage() {
    #ifdef DEBUG_LOG_GC
        std::cout << "-- gc begin" << std::endl;
        size_t before = bytesAllocated;
    #endif
    
    markRoots();
    sweep();
    
    // Adjust next GC threshold
    nextGC = bytesAllocated * 2;
    
    #ifdef DEBUG_LOG_GC
        std::cout << "-- gc end" << std::endl;
        std::cout << "   collected " << (before - bytesAllocated)
                  << " bytes (from " << before << " to " << bytesAllocated
                  << ") next at " << nextGC << std::endl;
    #endif
}

void VM::markRoots() {
    // Mark stack
    for (Value* slot = stack.data(); slot < stackTop; slot++) {
        markValue(*slot);
    }
    
    // Mark frames
    for (int i = 0; i < frameCount; i++) {
        markObject(frames[i].closure);
    }
    
    // Mark global variables
    for (const auto& [name, value] : globals) {
        markValue(value);
    }
    
    // Mark open upvalues
    for (ObjUpvalue* upvalue = openUpvalues; upvalue; upvalue = upvalue->next) {
        markObject(upvalue);
    }
    
    // Mark string intern table
    for (const auto& [str, obj] : strings) {
        markObject(obj);
    }
}

void VM::markValue(Value value) {
    if (value.isObj()) {
        markObject(value.asObj());
    }
}

void VM::markObject(Obj* object) {
    if (!object || object->isMarked) return;
    
    #ifdef DEBUG_LOG_GC
        std::cout << "mark " << object << std::endl;
    #endif
    
    object->isMarked = true;
    
    // Mark recursively based on object type
    switch (object->type) {
        case ObjType::OBJ_CLOSURE: {
            ObjClosure* closure = static_cast<ObjClosure*>(object);
            markObject(closure->function);
            for (ObjUpvalue* upvalue : closure->upvalues) {
                markObject(upvalue);
            }
            break;
        }
        case ObjType::OBJ_FUNCTION: {
            ObjFunction* function = static_cast<ObjFunction*>(object);
            for (Value constant : function->constants) {
                markValue(constant);
            }
            break;
        }
        case ObjType::OBJ_UPVALUE: {
            ObjUpvalue* upvalue = static_cast<ObjUpvalue*>(object);
            markValue(upvalue->closed);
            break;
        }
        case ObjType::OBJ_CLASS: {
            ObjClass* klass = static_cast<ObjClass*>(object);
            markObject(klass->superclass);
            for (const auto& [name, method] : klass->methods) {
                markObject(method);
            }
            break;
        }
        case ObjType::OBJ_INSTANCE: {
            ObjInstance* instance = static_cast<ObjInstance*>(object);
            markObject(instance->klass);
            for (const auto& [name, value] : instance->fields) {
                markValue(value);
            }
            break;
        }
        default:
            break;
    }
}

void VM::markTable(std::unordered_map<std::string, Value>& table) {
    for (const auto& [key, value] : table) {
        markValue(value);
    }
}

void VM::sweep() {
    Obj* previous = nullptr;
    Obj* object = objects;
    
    while (object) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;
            object = object->next;
            
            if (previous) {
                previous->next = object;
            } else {
                objects = object;
            }
            
            freeObject(unreached);
        }
    }
}

void VM::freeObject(Obj* object) {
    #ifdef DEBUG_LOG_GC
        std::cout << "free " << object << std::endl;
    #endif
    
    switch (object->type) {
        case ObjType::OBJ_STRING: {
            delete static_cast<ObjString*>(object);
            bytesAllocated -= sizeof(ObjString);
            break;
        }
        case ObjType::OBJ_FUNCTION: {
            ObjFunction* func = static_cast<ObjFunction*>(object);
            bytesAllocated -= sizeof(ObjFunction) + 
                              func->code.capacity() + 
                              func->constants.capacity() * sizeof(Value);
            delete func;
            break;
        }
        case ObjType::OBJ_CLOSURE: {
            ObjClosure* closure = static_cast<ObjClosure*>(object);
            bytesAllocated -= sizeof(ObjClosure) + 
                              closure->upvalues.capacity() * sizeof(ObjUpvalue*);
            delete closure;
            break;
        }
        case ObjType::OBJ_UPVALUE: {
            delete static_cast<ObjUpvalue*>(object);
            bytesAllocated -= sizeof(ObjUpvalue);
            break;
        }
        case ObjType::OBJ_CLASS: {
            ObjClass* klass = static_cast<ObjClass*>(object);
            bytesAllocated -= sizeof(ObjClass);
            delete klass;
            break;
        }
        case ObjType::OBJ_INSTANCE: {
            ObjInstance* instance = static_cast<ObjInstance*>(object);
            bytesAllocated -= sizeof(ObjInstance);
            delete instance;
            break;
        }
    }
}

// ============================================================
// Built-in functions
// ============================================================

void VM::defineBuiltins() {
    // Definir función clock()
    auto clockNative = [](int argCount, Value* args) -> Value {
        return Value::makeNumber(static_cast<double>(clock()) / CLOCKS_PER_SEC);
    };
    natives["clock"] = clockNative;
    
    // Definir función print()
    auto printNative = [](int argCount, Value* args) -> Value {
        for (int i = 0; i < argCount; i++) {
            std::cout << args[i].toString();
            if (i < argCount - 1) std::cout << " ";
        }
        std::cout << std::endl;
        return Value::makeNil();
    };
    natives["print"] = printNative;
    
    // Definir función type()
    auto typeNative = [](int argCount, Value* args) -> Value {
        if (argCount < 1) return Value::makeNil();
        if (args[0].isNumber()) return Value::makeObj(new ObjString("Number"));
        if (args[0].isBool()) return Value::makeObj(new ObjString("Boolean"));
        if (args[0].isNil()) return Value::makeObj(new ObjString("Nil"));
        if (args[0].isObj()) {
            Obj* obj = args[0].asObj();
            switch (obj->type) {
                case ObjType::OBJ_STRING: return Value::makeObj(new ObjString("String"));
                case ObjType::OBJ_CLASS: return Value::makeObj(new ObjString("Class"));
                case ObjType::OBJ_INSTANCE: return Value::makeObj(new ObjString("Instance"));
                case ObjType::OBJ_FUNCTION: return Value::makeObj(new ObjString("Function"));
                default: return Value::makeObj(new ObjString("Object"));
            }
        }
        return Value::makeNil();
    };
    natives["type"] = typeNative;
}

} // namespace hulk::backend