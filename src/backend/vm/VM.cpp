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
    : stackTop(stack), frameCount(0), objects(nullptr),
      bytesAllocated(0), nextGC(1024 * 1024), openUpvalues(nullptr),
      hadRuntimeError(false) {

    frames.reserve(FRAMES_MAX);
    defineBuiltins();
}

VM::~VM() {
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
    if (stackTop - stack >= STACK_MAX) {
        // Cannot call runtimeError here (it calls resetStack which moves stackTop)
        // so just flag the error and return.
        hadRuntimeError = true;
        return;
    }
    *stackTop++ = value;
}

// ISSUE-04 fix: bounds-checked pop
Value VM::pop() {
    if (stackTop == stack) {
        runtimeError("Internal error: stack underflow.");
        return Value::makeNil();
    }
    return *(--stackTop);
}

// ISSUE-04 fix: bounds-checked peek
Value VM::peek(int distance) const {
    if (distance < 0 || stackTop - stack <= distance) {
        return Value::makeNil();
    }
    return stackTop[-1 - distance];
}

// ============================================================
// Bytecode execution (run)
// ============================================================

InterpretResult VM::interpret(ObjFunction* function) {
    // ISSUE-02 fix: use allocateObj so the closure is GC-tracked from birth
    ObjClosure* closure = allocateObj<ObjClosure>(function);
    push(Value::makeObj(closure));

    CallFrame frame(closure, stack);
    frames.push_back(frame);
    frameCount++;

    return run();
}

InterpretResult VM::interpret(const BannerProgram& /*program*/) {
    // TODO: Compilar programa BANNER a bytecode interno
    return InterpretResult::INTERPRET_OK;
}

InterpretResult VM::run() {
    CallFrame* frame = &frames.back();

    // Macros para leer bytecode
    #define READ_BYTE() (*frame->ip++)

    // ISSUE-21 fix: cast bytes to uint8_t before shifting to avoid signed-shift UB
    #define READ_SHORT() \
        (frame->ip += 2, \
         static_cast<uint16_t>( \
             (static_cast<uint16_t>(frame->ip[-2]) << 8) | \
              static_cast<uint16_t>(frame->ip[-1])))

    #define READ_CONSTANT() \
        (frame->closure->function->constants[READ_BYTE()])
    #define READ_STRING() \
        (static_cast<ObjString*>(READ_CONSTANT().asObj())->chars)

    for (;;) {
        #ifdef DEBUG_TRACE_EXECUTION
            std::cout << "Stack: ";
            for (Value* slot = stack; slot < stackTop; slot++) {
                std::cout << slot->toString() << " ";
            }
            std::cout << std::endl;
        #endif

        if (hadRuntimeError) {
            return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }

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
                for (int i = 0; i < count; i++) pop();
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
                        // ISSUE-02 fix: allocate through allocateObj
                        push(Value::makeObj(allocateObj<ObjString>(aStr->chars + bStr->chars)));
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
                if (!peek(0).isNumber() || !peek(1).isNumber()) {
                    runtimeError("Operands must be numbers.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                double b = pop().asNumber();
                double a = pop().asNumber();
                push(Value::makeNumber(a - b));
                break;
            }

            case static_cast<uint8_t>(OpCode::OP_MULTIPLY): {
                if (!peek(0).isNumber() || !peek(1).isNumber()) {
                    runtimeError("Operands must be numbers.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                double b = pop().asNumber();
                double a = pop().asNumber();
                push(Value::makeNumber(a * b));
                break;
            }

            case static_cast<uint8_t>(OpCode::OP_DIVIDE): {
                if (!peek(0).isNumber() || !peek(1).isNumber()) {
                    runtimeError("Operands must be numbers.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                double b = pop().asNumber();
                double a = pop().asNumber();
                if (b == 0) {
                    runtimeError("Division by zero.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(Value::makeNumber(a / b));
                break;
            }

            // ISSUE-12 fix: pop first, check, then push result (single stack operation)
            case static_cast<uint8_t>(OpCode::OP_NEGATE): {
                Value value = pop();
                if (!value.isNumber()) {
                    runtimeError("Operand must be a number.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(Value::makeNumber(-value.asNumber()));
                break;
            }

            // ====================================================
            // Logical
            // ====================================================
            // ISSUE-13 fix: pop the operand so the stack stays balanced
            case static_cast<uint8_t>(OpCode::OP_NOT): {
                push(Value::makeBool(!pop().isTruthy()));
                break;
            }

            case static_cast<uint8_t>(OpCode::OP_EQUAL): {
                Value b = pop();
                Value a = pop();
                push(Value::makeBool(a == b));
                break;
            }

            case static_cast<uint8_t>(OpCode::OP_GREATER): {
                if (!peek(0).isNumber() || !peek(1).isNumber()) {
                    runtimeError("Operands must be numbers.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                double b = pop().asNumber();
                double a = pop().asNumber();
                push(Value::makeBool(a > b));
                break;
            }

            case static_cast<uint8_t>(OpCode::OP_LESS): {
                if (!peek(0).isNumber() || !peek(1).isNumber()) {
                    runtimeError("Operands must be numbers.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
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
            // ISSUE-02 fix: all heap allocations go through allocateObj
            case static_cast<uint8_t>(OpCode::OP_CLASS): {
                std::string name = READ_STRING();
                ObjClass* klass = allocateObj<ObjClass>(name);
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
                ObjClass* super    = static_cast<ObjClass*>(superclass.asObj());

                subclass->superclass = super;
                for (const auto& [mname, method] : super->methods) {
                    if (subclass->methods.find(mname) == subclass->methods.end()) {
                        subclass->methods[mname] = method;
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

                auto it = inst->fields.find(name);
                if (it != inst->fields.end()) {
                    pop();
                    push(it->second);
                    break;
                }

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
                std::string name  = READ_STRING();
                Value value       = peek(0);

                inst->fields[name] = value;
                pop();
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
                // ISSUE-02 fix: allocate through allocateObj
                ObjClosure* closure = allocateObj<ObjClosure>(function);

                for (int i = 0; i < function->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index   = READ_BYTE();
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
                    pop();
                    return InterpretResult::INTERPRET_OK;
                }

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
                closeUpvalues(stackTop - 1);
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
                runtimeError("Unknown opcode %d", static_cast<int>(instruction));
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }

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

    if (frameCount >= FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    // ISSUE-03 fix: stackTop points into the fixed array, so slots pointer is stable
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
                // ISSUE-02 fix: allocate through allocateObj
                ObjInstance* instance = allocateObj<ObjInstance>(klass);

                stackTop[-argCount - 1] = Value::makeObj(instance);

                ObjFunction* initializer = klass->findMethod("init");
                if (initializer) {
                    // ISSUE-02 fix: allocate through allocateObj
                    ObjClosure* closure = allocateObj<ObjClosure>(initializer);
                    return call(closure, argCount);
                } else if (argCount != 0) {
                    runtimeError("Expected 0 arguments but got %d.", argCount);
                    return false;
                }
                return true;
            }
            // ISSUE-11 fix: OBJ_NATIVE is looked up by name via the natives map;
            // native objects are never wrapped in an Obj* in this implementation,
            // so this case should never be reached. Keep as a safe no-op.
            case ObjType::OBJ_NATIVE:
                break;
            default:
                break;
        }
    }

    // Check native function table by name if callee is a string name
    if (callee.isObj() && callee.asObj()->type == ObjType::OBJ_STRING) {
        const std::string& fname = static_cast<ObjString*>(callee.asObj())->chars;
        auto it = natives.find(fname);
        if (it != natives.end()) {
            Value result = it->second(argCount, stackTop - argCount);
            stackTop -= argCount + 1;
            push(result);
            return true;
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

    // ISSUE-02 fix: allocate through allocateObj
    ObjClosure* closure = allocateObj<ObjClosure>(method);
    return call(closure, argCount);
}

// ============================================================
// Upvalues (closures)
// ============================================================

ObjUpvalue* VM::captureUpvalue(Value* local) {
    ObjUpvalue* prev    = nullptr;
    ObjUpvalue* upvalue = openUpvalues;

    while (upvalue && upvalue->location > local) {
        prev    = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue && upvalue->location == local) {
        return upvalue;
    }

    // ISSUE-02 fix: allocate through allocateObj
    ObjUpvalue* created = allocateObj<ObjUpvalue>(local);
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
        upvalue->closed     = *upvalue->location;
        upvalue->location   = &upvalue->closed;
        openUpvalues        = upvalue->next;
    }
}

// ============================================================
// Runtime errors
// ============================================================

// ISSUE-05 fix: signature is (const char*, ...) — no reference type before '...'
void VM::runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");

    // ISSUE-10 fix: guard OOB access into function->lines
    for (int i = frameCount - 1; i >= 0; i--) {
        CallFrame& fr       = frames[i];
        ObjFunction* function = fr.closure->function;

        if (fr.ip > function->code.data()) {
            size_t instruction = static_cast<size_t>(fr.ip - function->code.data() - 1);
            if (instruction < function->lines.size()) {
                fprintf(stderr, "[line %d] in ", function->lines[instruction]);
            } else {
                fprintf(stderr, "[line ?] in ");
            }
        } else {
            fprintf(stderr, "[line ?] in ");
        }

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
    stackTop   = stack;
    frameCount = 0;
    frames.clear();
    openUpvalues = nullptr;
}

// ============================================================
// Garbage Collection (mark-sweep)
// ============================================================

void VM::collectGarbage() {
    #ifdef DEBUG_LOG_GC
        std::cout << "-- gc begin" << std::endl;
        size_t before = bytesAllocated;
    #endif

    markRoots();
    sweep();

    nextGC = bytesAllocated * 2;

    #ifdef DEBUG_LOG_GC
        std::cout << "-- gc end" << std::endl;
        std::cout << "   collected " << (before - bytesAllocated)
                  << " bytes (from " << before << " to " << bytesAllocated
                  << ") next at " << nextGC << std::endl;
    #endif
}

void VM::markRoots() {
    for (Value* slot = stack; slot < stackTop; slot++) {
        markValue(*slot);
    }

    for (int i = 0; i < frameCount; i++) {
        markObject(frames[i].closure);
    }

    for (const auto& [name, value] : globals) {
        markValue(value);
    }

    for (ObjUpvalue* upvalue = openUpvalues; upvalue; upvalue = upvalue->next) {
        markObject(upvalue);
    }

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

    object->isMarked = true;

    switch (object->type) {
        case ObjType::OBJ_CLOSURE: {
            ObjClosure* closure = static_cast<ObjClosure*>(object);
            markObject(closure->function);
            for (ObjUpvalue* uv : closure->upvalues) {
                markObject(uv);
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
    Obj* object   = objects;

    while (object) {
        if (object->isMarked) {
            object->isMarked = false;
            previous         = object;
            object           = object->next;
        } else {
            Obj* unreached = object;
            object         = object->next;

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
        default:
            break;
    }
}

// ============================================================
// Built-in functions
// ============================================================

void VM::defineBuiltins() {
    // ISSUE-34 fix: lambdas assigned to std::function are safe even with captures
    natives["clock"] = [](int /*argCount*/, Value* /*args*/) -> Value {
        return Value::makeNumber(static_cast<double>(clock()) / CLOCKS_PER_SEC);
    };

    natives["print"] = [](int argCount, Value* args) -> Value {
        for (int i = 0; i < argCount; i++) {
            std::cout << args[i].toString();
            if (i < argCount - 1) std::cout << " ";
        }
        std::cout << std::endl;
        return Value::makeNil();
    };

    natives["type"] = [](int argCount, Value* args) -> Value {
        if (argCount < 1) return Value::makeNil();
        if (args[0].isNumber()) return Value::makeNil(); // simplified; real impl needs GC
        if (args[0].isBool())   return Value::makeNil();
        if (args[0].isNil())    return Value::makeNil();
        return Value::makeNil();
    };
}

} // namespace hulk::backend
