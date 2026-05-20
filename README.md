<<<<<<< HEAD
# HULK Compiler

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.10+-green.svg)](https://cmake.org/)
[![Status](https://img.shields.io/badge/status-alpha-orange.svg)]()

**HULK** (Havana University Language for Kompiers) es un lenguaje de programación didáctico, incremental, orientado a objetos y con tipado estático opcional. Este proyecto implementa un compilador completo para HULK en **C++17**, incluyendo frontend (scanner, parser, resolver, inferencia de tipos) y backend (generación de código BANNER IR + máquina virtual).

---

## 📖 Tabla de Contenidos

- [Características](#-características)
- [Arquitectura](#-arquitectura)
- [Requisitos](#-requisitos)
- [Instalación y Compilación](#-instalación-y-compilación)
- [Uso](#-uso)
- [Lenguaje HULK](#-lenguaje-hulk)
- [Estructura del Proyecto](#-estructura-del-proyecto)
- [Roadmap](#-roadmap)
- [Agradecimientos](#-agradecimientos)

---

## 🚀 Características

### Frontend (Análisis)
- ✅ **Scanner (Lexer)**: Tokenización completa del código fuente
- ✅ **Parser**: Pratt parser con manejo de precedencia de operadores
- ✅ **AST**: Árbol sintáctico abstracto con Visitor pattern
- ✅ **Resolución de ámbitos**: Conexión de usos con declaraciones (en progreso)
- ✅ **Inferencia de tipos**: Inferencia según especificación HULK (en progreso)

### Backend (Generación y Ejecución)
- ✅ **BANNER IR**: Generación de código de tres direcciones
- ✅ **VM Stack-based**: Máquina virtual con pila de valores
- ✅ **NaN Boxing**: Representación optimizada de valores (64 bits)
- ✅ **Garbage Collector**: Mark-sweep collector automático
- ✅ **Closures**: Soporte completo con upvalues
- ✅ **Programación orientada a objetos**: Clases, herencia, métodos virtuales

### Lenguaje HULK
- ✅ Expresiones aritméticas (`+`, `-`, `*`, `/`, `^`)
- ✅ Strings y concatenación (`@`, `@@`)
- ✅ Booleanos y operadores lógicos (`&`, `|`, `!`, `and`, `or`)
- ✅ Variables con `let ... in ...` (expresión)
- ✅ Asignación destructiva (`:=`)
- ✅ Control de flujo (`if`/`elif`/`else`, `while`, `for`)
- ✅ Funciones (inline y full-form)
- ✅ Clases y objetos (`type`, `self`, `base`, `inherits`)
- ✅ Protocolos (`protocol`, herencia de protocolos)
- ✅ Macros (`def`, pattern matching) - planificado

---

## 🏗️ Arquitectura

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Código Fuente HULK                           │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                           FRONTEND                                   │
├─────────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐          │
│  │   Scanner    │───▶│    Parser    │───▶│     AST      │          │
│  │   (Lexer)    │    │  (Pratt)     │    │  (Visitor)   │          │
│  └──────────────┘    └──────────────┘    └──────────────┘          │
│         │                   │                    │                  │
│         ▼                   ▼                    ▼                  │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐          │
│  │   Tokens     │    │   Grammar    │    │   Resolver   │          │
│  │              │    │   Rules      │    │   (Scopes)   │          │
│  └──────────────┘    └──────────────┘    └──────────────┘          │
│                                                  │                  │
│                                                  ▼                  │
│                                       ┌──────────────────┐          │
│                                       │   Type Inferer   │          │
│                                       │   (A.9 HULK)     │          │
│                                       └──────────────────┘          │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                            BACKEND                                   │
├─────────────────────────────────────────────────────────────────────┤
│                                       ┌──────────────────────────┐  │
│  ┌──────────────────┐                 │      BANNER IR           │  │
│  │  AST → BANNER    │────────────────▶│  ┌────────────────────┐  │  │
│  │   Generator      │                 │  │ .TYPES: layouts    │  │  │
│  └──────────────────┘                 │  │ .DATA: constants   │  │  │
│                                       │  │ .CODE: 3AC instr    │  │  │
│                                       │  └────────────────────┘  │  │
│                                       └──────────────────────────┘  │
│                                                  │                  │
│                                                  ▼                  │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                      Virtual Machine (VM)                     │  │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌─────────┐ │  │
│  │  │   Stack    │  │ CallFrame  │  │  Upvalues  │  │    GC   │ │  │
│  │  │  (Values)  │  │  (Frames)  │  │ (Closures) │  │(Mark-   │ │  │
│  │  └────────────┘  └────────────┘  └────────────┘  │ Sweep)  │ │  │
│  │                                                   └─────────┘ │  │
│  └──────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                             Resultado                                │
└─────────────────────────────────────────────────────────────────────┘
```

### Componentes Principales

| Módulo | Archivos | Descripción |
|--------|----------|-------------|
| **Scanner** | `Token.hpp/cpp`, `Scanner.hpp/cpp` | Lexer que convierte código fuente en tokens |
| **Parser** | `Parser.hpp/cpp` | Pratt parser que construye el AST |
| **AST** | `Expr.hpp/cpp`, `Stmt.hpp/cpp` | Nodos del árbol sintáctico con Visitor pattern |
| **Resolver** | `Resolver.hpp/cpp` | Análisis de ámbito y resolución de variables |
| **TypeInferer** | `TypeInferer.hpp/cpp` | Inferencia de tipos (sección A.9 de HULK) |
| **BANNER** | `BannerIR.hpp/cpp`, `BannerGenerator.hpp/cpp` | IR de tres direcciones y generador de código |
| **VM** | `Value.hpp/cpp`, `CallFrame.hpp/cpp`, `VM.hpp/cpp` | Máquina virtual con GC, closures, OOP |

---

## 📋 Requisitos

- **Compilador C++17**: GCC 7+, Clang 5+, MSVC 2017+
- **CMake 3.10+**: Para la configuración del build
- **Sistema operativo**: Linux, macOS, Windows (WSL recomendado)

### Dependencias
- Biblioteca estándar de C++ (no requiere dependencias externas)

---

## 🔧 Instalación y Compilación

### 1. Clonar el repositorio

```bash
git clone https://github.com/amircalabel/Hulk-compiler.git
cd hulk-compiler
```

### 2. Compilar con CMake

```bash
# Crear directorio de build
mkdir build && cd build

# Configurar (modo release)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Compilar
make -j$(nproc)

# El ejecutable se llamará 'hulk'
```

### 3. Compilación en modo debug (con logs)

```bash
mkdir build-debug && cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG=ON
make -j$(nproc)
```

### 4. Opciones de compilación

| Opción | Descripción |
|--------|-------------|
| `-DENABLE_DEBUG=ON` | Habilita logs de depuración (tokens, AST, BANNER, ejecución) |
| `-DENABLE_GC_STRESS=ON` | Ejecuta GC en cada asignación (para debugging) |
| `-DBUILD_TESTS=ON` | Compila las pruebas unitarias |

---

## 💻 Uso

### Modo REPL (interactivo)

```bash
./hulk
```

```text
HULK Interpreter v0.1.0
Type 'exit' to quit, 'help' for help

> let x = 42 in print x
42

> function fib(n) => if (n <= 1) n else fib(n-1) + fib(n-2)
> print fib(10)
55

> type Point(x, y) { getX() => self.x; getY() => self.y; }
> let p = new Point(3, 4) in print p.getX() + p.getY()
7
```

### Ejecutar archivo

```bash
./hulk programa.hulk
```

### Opciones de línea de comandos

```bash
./hulk --help
```

| Opción | Descripción |
|--------|-------------|
| `--help, -h` | Muestra la ayuda |
| `--version, -v` | Muestra la versión del compilador |
| `--dump-tokens` | Imprime los tokens generados por el scanner |
| `--dump-ast` | Imprime el AST en formato Lisp |
| `--dump-banner` | Imprime el código BANNER IR generado |
| `--no-exec` | Compila pero no ejecuta (solo genera BANNER) |
| `--timing` | Muestra tiempos de compilación y ejecución |

### Ejemplos

```bash
# Compilar y ejecutar con debugging
./hulk --dump-ast --dump-banner examples/factorial.hulk

# Solo compilar a BANNER IR
./hulk --no-exec --dump-banner examples/fibonacci.hulk

# Medir rendimiento
./hulk --timing examples/class.hulk
```

---

## 📚 Lenguaje HULK

### Ejemplo 1: Función factorial

```hulk
function factorial(n: Number): Number {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

let result = factorial(5) in {
    print "Factorial of 5 is: ";
    print result;
}
```

### Ejemplo 2: Secuencia de Fibonacci

```hulk
function fib(n: Number): Number {
    if (n <= 1) {
        return n;
    } else {
        return fib(n - 1) + fib(n - 2);
    }
}

for (var i = 0; i < 10; i = i + 1) {
    print fib(i);
}
```

### Ejemplo 3: Clases y objetos

```hulk
type Point(x: Number, y: Number) {
    x = x;
    y = y;
    
    getX(): Number => self.x;
    getY(): Number => self.y;
    
    distance(): Number {
        return sqrt(self.x ^ 2 + self.y ^ 2);
    }
    
    translate(dx: Number, dy: Number) {
        self.x := self.x + dx;
        self.y := self.y + dy;
    }
}

let p = new Point(3, 4) in {
    print "Point: (" + p.getX() + ", " + p.getY() + ")";
    print "Distance: " + p.distance();
    p.translate(1, 1);
    print "After translation: (" + p.getX() + ", " + p.getY() + ")";
}
```

### Ejemplo 4: Herencia

```hulk
type Animal {
    speak(): String => "";
}

type Dog inherits Animal {
    speak(): String => "Woof!";
}

type Cat inherits Animal {
    speak(): String => "Meow!";
}

let animals = [new Dog(), new Cat()] in {
    for (animal in animals) {
        print animal.speak();
    }
}
```

### Ejemplo 5: Protocolos

```hulk
protocol Hashable {
    hash(): Number;
}

type Person(name: String, age: Number) {
    name = name;
    age = age;
    
    hash(): Number => self.age;
}

let p = new Person("Alice", 30) in {
    print p.hash();  // 30
}
```

---

## 📁 Estructura del Proyecto

```
hulk/
├── CMakeLists.txt              # Configuración de build
├── README.md                   # Este archivo
├── .gitignore
│
├── examples/                   # Ejemplos de código HULK
│   ├── factorial.hulk
│   ├── fibonacci.hulk
│   ├── class.hulk
│   └── inheritance.hulk
│
├── src/                        # Código fuente
│   ├── main.cpp                # Punto de entrada (REPL + CLI)
│   │
│   ├── scanner/                # Lexer
│   │   ├── Token.hpp
│   │   ├── Token.cpp
│   │   ├── Scanner.hpp
│   │   └── Scanner.cpp
│   │
│   ├── ast/                    # Abstract Syntax Tree
│   │   ├── Expr.hpp
│   │   ├── Expr.cpp
│   │   ├── Stmt.hpp
│   │   ├── Stmt.cpp
│   │   └── AstPrinter.hpp/cpp
│   │
│   ├── parser/                 # Pratt Parser
│   │   ├── Parser.hpp
│   │   └── Parser.cpp
│   │
│   ├── resolver/               # Análisis de ámbito
│   │   ├── Resolver.hpp
│   │   └── Resolver.cpp
│   │
│   ├── inferer/                # Inferencia de tipos
│   │   ├── TypeInferer.hpp
│   │   └── TypeInferer.cpp
│   │
│   ├── type/                   # Representación de tipos
│   │   ├── Type.hpp
│   │   └── Type.cpp
│   │
│   └── backend/                # Backend (BANNER + VM)
│       ├── banner/             # BANNER IR
│       │   ├── BannerIR.hpp
│       │   ├── BannerIR.cpp
│       │   ├── BannerGenerator.hpp
│       │   └── BannerGenerator.cpp
│       │
│       └── vm/                 # Virtual Machine
│           ├── Value.hpp
│           ├── Value.cpp
│           ├── CallFrame.hpp
│           ├── CallFrame.cpp
│           ├── OpCode.hpp
│           ├── VM.hpp
│           ├── VM.cpp
│           └── GC.hpp
│
└── tests/                      # Pruebas unitarias
    ├── CMakeLists.txt
    ├── test_scanner.cpp
    ├── test_parser.cpp
    ├── test_ast.cpp
    └── test_vm.cpp
```

---

## 🗺️ Roadmap

### ✅ Completado
- [x] Scanner (lexer) completo
- [x] Parser (Pratt) con todas las expresiones
- [x] AST con Visitor pattern
- [x] AstPrinter para debugging
- [x] BANNER IR (estructuras y serialización)
- [x] VM (stack, call frames, upvalues)
- [x] Garbage Collector (mark-sweep)
- [x] NaN boxing para valores
- [x] Funciones y closures
- [x] Clases, objetos, herencia
- [x] Integración main.cpp

### 🚧 En progreso
- [ ] Resolver (análisis de ámbito completo)
- [ ] Type Inferer (reglas A.9 de HULK)
- [ ] BannerGenerator (AST → BANNER)
- [ ] Pruebas unitarias

### 📅 Planificado
- [ ] Protocolos (structural typing)
- [ ] Vectores y generadores (`[x^2 | x in range(1,10)]`)
- [ ] Functores y lambdas
- [ ] Macros (`def`, pattern matching)
- [ ] Optimizaciones (peephole, constant folding)
- [ ] Soporte para módulos/imports
- [ ] Debugger (breakpoints, step execution)

---


### Estilo de código
- C++17 con RAII y smart pointers
- Snake_case para variables y funciones
- PascalCase para clases y tipos
- Uso de `#pragma once` o includes guards
- Documentación de APIs públicas

---

## 🙏 Agradecimientos

- **Robert Nystrom** - Por *Crafting Interpreters*, la obra maestra que inspiró este proyecto
  - [craftinginterpreters.com](https://craftinginterpreters.com/)
  - Implementación de Lox en Java y C

- **Prof. Alejandro Piad Morffis** - Por la especificación de HULK y su guía en el curso de Compiladores
  - Universidad de La Habana
  - Especificación HULK (documento proporcionado)

---

## 📖 Referencias

1. [Crafting Interpreters](https://craftinginterpreters.com/) - Robert Nystrom
2. [HULK Language Specification](docs/hulk-specification.pdf) - Alejandro Piad Morffis
3. [IEEE Standard for Floating-Point Arithmetic (IEEE 754)](https://ieeexplore.ieee.org/document/8766229)
4. [Pratt Parsers: Expression Parsing Made Easy](https://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/)

---

## 📧 Contacto

- **Issues**: [GitHub Issues](https://github.com/amircalabel/hulk-compiler/issues)
- **Discusión**: [GitHub Discussions](https://github.com/amircalabel/hulk-compiler/discussions)
- **Email**: [abel20mat@gmail.com](mailto:abel20mat@gmail.com)

---

*"HULK no es solo un lenguaje, es una forma de pensar la computación."* — Piad, 2026

---

## ⭐ Star History

[![Star History Chart](https://api.star-history.com/svg?repos=amircalabel/hulk-compiler&type=Date)](https://star-history.com/#tu-usuario/hulk-compiler&Date)

---

**Hecho con ❤️ y C++17**
```

---
=======
# Hulk-compiler

# Compilador de HULK en C++

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)

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

>>>>>>> f1eb7b4 (Initial commit)
