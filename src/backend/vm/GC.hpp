// src/backend/vm/GC.hpp
#ifndef HULK_GC_HPP
#define HULK_GC_HPP

#include "Value.hpp"

namespace hulk::backend {

/**
 * Garbage Collector - Configuración y constantes (capítulo 26)
 * 
 * Implementación de mark-sweep collector:
 * - Mark: recorre desde las raíces marcando objetos alcanzables
 * - Sweep: libera objetos no marcados
 * 
 * Configuración:
 * - GC_HEAP_GROW_FACTOR: factor de crecimiento del heap (2x)
 * - GC_START_THRESHOLD: umbral inicial (1MB)
 */
class GC {
public:
    static constexpr size_t HEAP_GROW_FACTOR = 2;
    static constexpr size_t START_THRESHOLD = 1024 * 1024;  // 1MB
    
    // Estados del GC (para colección incremental)
    enum class State {
        IDLE,       // No está recolectando
        MARKING,    // En fase de marcado
        SWEEPING    // En fase de barrido
    };
    
    // Estadísticas para debugging
    struct Stats {
        size_t bytesAllocated = 0;
        size_t bytesFreed = 0;
        size_t collections = 0;
        size_t objectsMarked = 0;
        size_t objectsFreed = 0;
        double lastCollectionTime = 0;
    };
};

} // namespace hulk::backend

#endif // HULK_GC_HPP
