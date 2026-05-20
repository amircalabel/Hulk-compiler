# Hulk-compiler

# Compilador de HULK en C++

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## 📖 Descripción

Este proyecto es una implementación del **frontend** del compilador para el lenguaje de programación **HULK** (Havana University Language for Kompiers), desarrollado en C++ moderno (C++17/20).

HULK es un lenguaje didáctico, incremental, orientado a objetos y con tipado estático opcional, diseñado para el curso de Compiladores en la Universidad de La Habana.

### Características actualmente implementadas

- ✅ **Scanner (Lexer)**: Convierte código fuente en tokens
- ✅ **Parser**: Construye el AST (Abstract Syntax Tree) usando Pratt parsing
- ✅ **AST completo**: Soporta todas las expresiones y statements de HULK
- ✅ **Sistema de errores**: Reporte de errores sintácticos con ubicación

### Características del lenguaje HULK soportadas

| Categoría | Características |
|-----------|-----------------|
| **Expresiones** | Aritméticas (`+`, `-`, `*`, `/`, `^`), strings (`@`, `@@`), booleanas (`&`, `\|`, `!`), comparación (`==`, `!=`, `<`, `>`, `<=`, `>=`) |
| **Variables** | `let ... in ...` (expresión), `var` (statement global), asignación destructiva `:=` |
| **Control de flujo** | `if` / `elif` / `else`, `while`, `for` (estilo C) |
| **Funciones** | Inline (`=>`) y full-form (`{ ... }`), parámetros, retorno, anotaciones de tipo opcionales |
| **Clases** | `type`, atributos, métodos, `self`, `base`, herencia (`inherits`) |
| **Protocolos** | `protocol`, herencia de protocolos, métodos sin implementación |
| **Macros** | `def`, parámetros simbólicos (`@`), placeholders (`$`), pattern matching |

## 🏗️ Arquitectura del Frontend

```
Código fuente HULK
       ↓
┌──────────────┐
│   Scanner    │ → Tokens
└──────────────┘
       ↓
┌──────────────┐
│   Parser     │ → AST (Expr + Stmt)
└──────────────┘
       ↓
┌──────────────┐
│   Resolver   │ (Próximamente) → Ámbitos resueltos
└──────────────┘
       ↓
┌──────────────┐
│ Type Inferer │ (Próximamente) → AST tipado
└──────────────┘
       ↓
┌──────────────┐
│   Backend    │ (Futuro) → Código BANNER / VM
└──────────────┘
```

## 📁 Estructura del Proyecto

```
hulk/
├── CMakeLists.txt              # Configuración de build
├── README.md                   # Este archivo
├── src/
│   ├── main.cpp                # Punto de entrada (REPL + ejecución de archivos)
│   ├── scanner/
│   │   ├── Token.hpp           # Definición de Token y TokenType
│   │   ├── Token.cpp
│   │   ├── Scanner.hpp         # Lexer
│   │   └── Scanner.cpp
│   ├── ast/
│   │   ├── Expr.hpp            # Nodos del AST para expresiones
│   │   ├── Expr.cpp
│   │   ├── Stmt.hpp            # Nodos del AST para statements
│   │   └── Stmt.cpp
│   └── parser/
│       ├── Parser.hpp          # Parser recursivo descendente con Pratt
│       └── Parser.cpp
└── tests/                      # Pruebas (próximamente)
```

## 🚀 Cómo Compilar y Ejecutar

### Requisitos

- Compilador C++17 (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+

### Compilación

```bash
# Clonar el repositorio
git clone https://github.com/tu-usuario/hulk-compiler.git
cd hulk-compiler

# Crear directorio de build
mkdir build && cd build

# Configurar y compilar
cmake ..
make

# El ejecutable se llamará 'hulk'
```

### Uso

```bash
# Modo REPL (interactivo)
./hulk

# Ejecutar un archivo .hulk
./hulk programa.hulk
```

### Ejemplo de código HULK

```hulk
// Función factorial recursiva
function factorial(n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

// Clase Punto
type Point(x, y) {
    x = x;
    y = y;
    
    getX() => self.x;
    getY() => self.y;
    
    distance() {
        return sqrt(self.x ^ 2 + self.y ^ 2);
    }
}

// Programa principal
let pt = new Point(3, 4) in {
    print pt.getX();   // 3
    print pt.getY();   // 4
    print pt.distance(); // 5
}
```

## 🔧 Componentes Técnicos

### Scanner (Lexer)

- Reconoce todos los tokens de HULK: palabras clave, operadores, literales, identificadores
- Manejo de strings multi-línea
- Comentarios de línea (`//`)
- Reporte de errores léxicos (strings sin cerrar, caracteres inválidos)

### Parser

- **Pratt parsing** (top-down operator precedence) para expresiones
- Manejo de precedencia de 10+ niveles de operadores
- Soporte para operadores infix, prefix y (futuro) postfix
- Recuperación de errores en modo pánico

### AST (Abstract Syntax Tree)

- **Expresiones**: `Literal`, `Binary`, `Unary`, `Grouping`, `Variable`, `Assign`, `Let`, `If`, `While`, `For`, `Block`, `Call`
- **Statements**: `Expression`, `Print`, `Return`, `Block`, `VarDecl`, `FunctionDecl`, `ClassDecl`, `ProtocolDecl`, `MacroDecl`
- **Visitor pattern** para operaciones sobre el AST

## 📚 Inspiración y Referencias

Este proyecto está fuertemente inspirado en:

- **[Crafting Interpreters](https://craftinginterpreters.com/)** de Robert Nystrom
  - Arquitectura del frontend (Scanner → Parser → AST → Resolver)
  - Técnicas de Pratt parsing y recursive descent
  - Manejo de errores y recuperación

- **Especificación de HULK** (documento proporcionado)
  - Gramática completa del lenguaje
  - Reglas de tipado e inferencia
  - Protocolos, functores y macros

## 🗺️ Hoja de Ruta

- [x] Scanner (Lexer)
- [x] Parser (AST)
- [x] Sistema de errores básico
- [ ] **AstPrinter** (debugging del AST)
- [ ] **Resolver** (análisis de ámbitos)
- [ ] **Type Inferer** (inferencia de tipos, sección A.9)
- [ ] **Type Checker** (verificación de tipos)
- [ ] **Backend**: Generación de código BANNER IR
- [ ] **Virtual Machine**: Ejecución de bytecode
- [ ] **Garbage Collector** (mark-sweep)
- [ ] **Optimizaciones** (NaN boxing, hash table improvements)

## 👥 Autores

- Amircal Abel Metelis Matui - *Implementación inicial*

## 🙏 Agradecimientos

- Robert Nystrom por *Crafting Interpreters*, una obra maestra que hizo accesible el mundo de los compiladores
- Prof. Alejandro Piad Morffis por la especificación de HULK
- La comunidad de University of Havana por el diseño del lenguaje

---

*"HULK no es solo un lenguaje, es una forma de pensar la computación."* — Piad, 2026
```

