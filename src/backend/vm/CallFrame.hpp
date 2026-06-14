// src/backend/vm/CallFrame.hpp
#ifndef HULK_CALL_FRAME_HPP
#define HULK_CALL_FRAME_HPP

#include "Value.hpp"
#include <vector>

namespace hulk::backend {

// Forward declaration
struct ObjClosure;

/**
 * CallFrame - Representa una invocación de función activa (capítulo 24)
 * 
 * Cada vez que se llama a una función, se crea un CallFrame en la pila.
 * Contiene el closure siendo ejecutado, el instruction pointer (IP),
 * y la ventana de stack donde viven sus variables locales.
 */
struct CallFrame {
    ObjClosure* closure;           // Función siendo ejecutada
    const uint8_t* ip;             // Instruction pointer (bytecode offset)
    Value* slots;                  // Base de la ventana de stack
    int slotCount;                 // Número de slots usados
    
    CallFrame();
    CallFrame(ObjClosure* closure, Value* slots);
    
    void reset();
    
    // Leer byte actual y avanzar IP
    uint8_t readByte();
    
    // Leer short (16 bits) y avanzar IP
    uint16_t readShort();
    
    // Leer constante de la piscina de constantes
    Value readConstant();
    
    // Leer string constante
    std::string readString();
};

} // namespace hulk::backend

#endif // HULK_CALL_FRAME_HPP