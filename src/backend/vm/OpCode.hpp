// src/backend/vm/OpCode.hpp
#ifndef HULK_OPCODE_HPP
#define HULK_OPCODE_HPP

#include <cstdint>

namespace hulk::backend {

// Opcodes para la VM (basados en clox)
enum class OpCode : uint8_t {
    // Constantes
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    
    // Stack manipulation
    OP_POP,
    OP_POPN,
    
    // Arithmetic
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    
    // Logical
    OP_NOT,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    
    // Variables
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    
    // Objects and properties
    OP_CLASS,
    OP_INHERIT,
    OP_METHOD,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_GET_SUPER,
    OP_INVOKE,
    OP_SUPER_INVOKE,
    
    // Functions and calls
    OP_CLOSURE,
    OP_CALL,
    OP_RETURN,
    
    // Control flow
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CLOSE_UPVALUE,
    
    // I/O
    OP_PRINT,
    
    // VM control
    OP_HALT
};

} // namespace hulk::backend

#endif // HULK_OPCODE_HPP