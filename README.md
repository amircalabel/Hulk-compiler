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
Código fuente HULK
↓
┌──────────────┐
│ Scanner │ → Tokens
└──────────────┘
↓
┌──────────────┐
│ Parser │ → AST (Expr + Stmt)
└──────────────┘
↓
┌──────────────┐
│ Resolver │ (Próximamente) → Ámbitos resueltos
└──────────────┘
↓
┌──────────────┐
│ Type Inferer │ (Próximamente) → AST tipado
└──────────────┘
↓
┌──────────────┐
│ Backend │ (Futuro) → Código BANNER / VM
└──────────────┘
## 📁 Estructura del Proyecto

hulk/
├── CMakeLists.txt # Configuración de build
├── README.md # Este archivo
├── src/
│ ├── main.cpp # Punto de entrada (REPL + ejecución de archivos)
│ ├── scanner/
│ │ ├── Token.hpp # Definición de Token y TokenType
│ │ ├── Token.cpp
│ │ ├── Scanner.hpp # Lexer
│ │ └── Scanner.cpp
│ ├── ast/
│ │ ├── Expr.hpp # Nodos del AST para expresiones
│ │ ├── Expr.cpp
│ │ ├── Stmt.hpp # Nodos del AST para statements
│ │ └── Stmt.cpp
│ └── parser/
│ ├── Parser.hpp # Parser recursivo descendente con Pratt
│ └── Parser.cpp
└── tests/ # Pruebas (próximamente)


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

# Uso
# Modo REPL (interactivo)
./hulk

# Ejecutar un archivo .hulk
./hulk programa.hulk

