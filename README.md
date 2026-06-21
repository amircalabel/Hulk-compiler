# HULK Compiler

## Quick start

- Build the compiler with `make`
- Run it on a source file with `./hulk path/to/file.hulk`
- On success, the compiler writes the executable to `./output`
- On lexical, syntactic, or semantic errors, the compiler prints the error details to stdout

## Project overview

El **HULK Compiler** es una implementación completa de un compilador para el lenguaje de programación didáctico **HULK** (Havana University Language for Kompiers). El proyecto abarca tanto el **frontend** (análisis léxico, sintáctico, semántico e inferencia de tipos) como el **backend** (generación de código BANNER IR, máquina virtual y generación de ejecutables). El compilador está escrito en **C++17** y es capaz de compilar programas HULK a ejecutables nativos en Linux x86_64, cumpliendo con el contrato de interfaz especificado para la evaluación automática.

---

## 2. Arquitectura del Compilador

La arquitectura del compilador sigue un diseño de **pipeline** clásico, dividido en fases bien definidas:

```
Código HULK → Scanner → Parser → Resolver → Type Inferer → Backend → ./output
```

### 2.1 Estructura de Directorios

```
Hulk-compiler/
├── examples/               # Programas de ejemplo en HULK
├── src/
│   ├── ast/               # Abstract Syntax Tree (Expr, Stmt, AstPrinter)
│   ├── backend/           # Backend (BANNER IR, VM, CodeGenerator)
│   │   ├── banner/        # Generación de código BANNER
│   │   └── vm/            # Máquina virtual y garbage collector
│   ├── inferer/           # Inferencia de tipos (sección A.9)
│   ├── interpreter/       # Tree-walk interpreter (modo REPL/debug)
│   ├── parser/            # Pratt parser (recursive descent)
│   ├── resolver/          # Análisis de ámbito (cap. 11)
│   ├── scanner/           # Lexer (Token, Scanner)
│   └── type/              # Representación de tipos (Type, TypeKind)
├── tests/                 # Pruebas unitarias y de integración
├── CMakeLists.txt         # Configuración de CMake
├── Makefile               # Build system (contracto)
└── README.md              # Documentación del proyecto
```

**Total:** 14 carpetas, 62 archivos, ~472 KB.

### 2.2 Componentes Principales

| Módulo | Archivos | Función |
|--------|----------|---------|
| **Scanner** | `Token.hpp/cpp`, `Scanner.hpp/cpp` | Análisis léxico: convierte código fuente en tokens |
| **Parser** | `Parser.hpp/cpp` | Análisis sintáctico: construye el AST usando Pratt parsing |
| **AST** | `Expr.hpp/cpp`, `Stmt.hpp/cpp`, `AstPrinter.hpp/cpp` | Representación del árbol sintáctico con Visitor pattern |
| **Resolver** | `Resolver.hpp/cpp` | Análisis de ámbito: conecta usos con declaraciones |
| **TypeInferer** | `TypeInferer.hpp/cpp` | Inferencia de tipos (sección A.9 de la especificación) |
| **Interpreter** | `Interpreter.hpp/cpp` | Tree-walk interpreter (modo REPL y debugging) |
| **Backend BANNER** | `BannerIR.hpp/cpp`, `BannerGenerator.hpp/cpp` | Generación de IR de tres direcciones |
| **VM** | `VM.hpp/cpp`, `Value.hpp/cpp`, `CallFrame.hpp/cpp`, `OpCode.hpp`, `GC.hpp` | Máquina virtual con stack, upvalues y GC |
| **CodeGenerator** | `CodeGenerator.hpp/cpp`, `ASTSerializer.hpp/cpp` | Generación de ejecutable `./output` |

---

## 3. Frontend

### 3.1 Scanner (Análisis Léxico)

El scanner recorre el código fuente carácter por carácter y agrupa los caracteres en **tokens**. Los tokens incluyen:

- **Palabras clave**: `let`, `in`, `function`, `type`, `if`, `else`, `while`, `for`, `print`, `return`, etc.
- **Operadores**: `+`, `-`, `*`, `/`, `^`, `@`, `@@`, `:=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&`, `|`, `!`
- **Literales**: números (enteros y decimales), strings (con soporte multi-línea), booleanos (`true`/`false`) y `nil`
- **Identificadores**: nombres de variables, funciones, tipos
- **Puntuación**: `(`, `)`, `{`, `}`, `;`, `,`, `.`
- **Comentarios**: `//` hasta fin de línea

El scanner también maneja errores léxicos como caracteres inválidos o strings sin cerrar, reportándolos en el formato `(line,col) LEXICAL: mensaje`.

**Arquitectura del Scanner:**
- Usa un mapa `unordered_map<string, TokenType>` para reconocer keywords
- Técnica de "maximal munch" para tokens multi-carácter (ej. `@@`, `:=`)
- Manejo de strings con soporte para nuevas líneas internas

### 3.2 Parser (Análisis Sintáctico)

El parser implementa un **Pratt parser** (top-down operator precedence) que maneja la precedencia de operadores de forma elegante. Las principales características son:

- **Expresiones**: literales, binarias, unarias, agrupación, variables, asignación (`:=`), `let ... in ...`, `if`, `while`, `for`, bloques `{ ... }`
- **Statements**: `print`, `return`, bloques, declaraciones de variables (`var`), funciones, clases, protocolos, macros
- **Precedencia de operadores** (de menor a mayor):
  1. `:=` (asignación destructiva)
  2. `or`
  3. `and`
  4. `==`, `!=`
  5. `<`, `<=`, `>`, `>=`
  6. `+`, `-`
  7. `*`, `/`, `^`
  8. `@`, `@@` (concatenación)
  9. `!`, `-` (unarios)

**Técnica de parsing:** El parser es recursivo descendente y utiliza una tabla de reglas (`ParseRule`) que asocia cada token con funciones de parseo prefix e infix, y su precedencia. Esto permite extender fácilmente la gramática.

### 3.3 AST (Abstract Syntax Tree)

El AST está dividido en dos jerarquías de clases:

- **`Expr`**: Expresiones (literales, binarias, unarias, variables, asignación, `let`, `if`, `while`, `for`, bloques, llamadas)
- **`Stmt`**: Statements (expresión, print, return, bloque, declaraciones de variables, funciones, clases, protocolos, macros, `if`, `while`, `for`)

Ambas jerarquías implementan el **patrón Visitor**, lo que permite añadir nuevas operaciones (impresión, resolución, inferencia, ejecución) sin modificar las clases de los nodos.

### 3.4 Resolver (Análisis de Ámbito)

El resolver recorre el AST y conecta cada uso de variable con su declaración. Utiliza una **pila de scopes** (cada scope es un mapa de nombre → información de variable). Las principales responsabilidades son:

- Detectar variables no definidas
- Prevenir usos antes de inicialización
- Manejar scopes anidados y shadowing
- Resolver `self` y `base` en clases
- Identificar upvalues para closures

### 3.5 Type Inferer (Inferencia de Tipos)

Implementa la **sección A.9** de la especificación de HULK:

- **Inferencia de expresiones** (A.9.2): literales → tipo concreto; binarias según operador
- **Inferencia de símbolos** (A.9.3): para variables/funciones/parámetros sin anotación
- **Protocolos sintetizados** (A.9.5): creación de protocolos implícitos para funciones que acceden a métodos
- **LCA (Lowest Common Ancestor)**: para expresiones `if` con múltiples ramas

Los tipos en HULK se representan mediante la clase `Type`, que soporta:

- Tipos primitivos: `Number`, `String`, `Boolean`, `Nil`
- Tipos compuestos: `Object`, `Class`, `Protocol`, `Function`, `Generic`
- Variables de tipo para inferencia

---

## 4. Backend

### 4.1 BANNER IR (Intermediate Representation)

El backend genera código en **BANNER** (Basic 3-Address liNear iNtEmendate Representation), un IR de tres direcciones diseñado específicamente para HULK. La estructura de un programa BANNER incluye:

- **Sección `.TYPES`**: layouts de objetos en memoria (atributos y métodos)
- **Sección `.DATA`**: pool de constantes (strings y literales)
- **Sección `.CODE`**: instrucciones lineales con etiquetas y saltos

**Instrucciones principales:**
- Movimiento de datos: `LOAD`, `STORE`
- Aritmética: `ADD`, `SUB`, `MUL`, `DIV`, `POW`
- Objetos: `ALLOCATE`, `GETATTR`, `SETATTR`
- Control de flujo: `LABEL`, `GOTO`, `IF_GOTO`
- Llamadas: `PARAM`, `CALL`, `VCALL`, `RETURN`

### 4.2 Virtual Machine (VM)

La VM es una **máquina de pila** que ejecuta el bytecode generado. Sus componentes principales son:

| Componente | Descripción |
|------------|-------------|
| **`Value`** | Representación de valores con NaN boxing (64 bits) |
| **`VM`** | Bucle principal de ejecución (`run()`) |
| **`CallFrame`** | Stack frame para llamadas a funciones |
| **`Obj*`** | Objetos en heap (strings, funciones, clases, instancias) |
| **Upvalues** | Implementación de closures (cap. 25 del libro) |
| **GC** | Garbage collector mark-sweep (cap. 26) |

**Estructura de la VM:**
- **Stack de valores**: para operandos y temporales
- **Call stack**: para funciones anidadas
- **Upvalue list**: para captura de variables en closures
- **String interning**: todas las strings se internan (hash table global)
- **Tablas de métodos y campos**: para clases e instancias

### 4.3 CodeGenerator (Generación de `./output`)

El generador de código produce un ejecutable nativo en dos pasos:

1. **Genera `output.cpp`**: traduce el AST a código C++ que incluye el runtime de HULK
2. **Compila con `g++`**: produce `./output` (binario Linux x86_64)

El runtime embebido en `output.cpp` incluye:
- Representación de valores (`HulkValue`)
- Entorno de variables (`Environment`)
- Operadores aritméticos y lógicos
- Soporte para funciones y llamadas (básico)

---

## 5. Lenguaje HULK - Características Implementadas

### 5.1 Expresiones
- Aritméticas: `+`, `-`, `*`, `/`, `^` (potencia)
- Strings: `@` (concatenación), `@@` (concatenación con espacio)
- Booleanas: `&`, `|`, `!`, `and`, `or`
- Comparación: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Variables y asignación destructiva: `:=`
- `let ... in ...` (expresión con bindings locales)
- `if` / `elif` / `else`
- `while` y `for` (estilo C)
- Bloques: `{ expr1; expr2; ... }` (retorna valor de la última expresión)

### 5.2 Statements
- `print` (built-in)
- `return`
- Bloques `{ ... }`
- Declaración de variables: `var name = expr;`
- Funciones: inline (`=>`) y full-form (`{ ... }`)
- Clases: `type`, atributos, métodos, `self`, `base`, `inherits`
- Protocolos: `protocol`
- Macros: `def`

### 5.3 Tipado
- Tipado estático opcional (anotaciones)
- Inferencia de tipos (sección A.9)
- Protocolos (structural typing)
- Jerarquía de tipos: `Object` como raíz

---

## 6. Decisiones de Diseño

| Área | Decisión | Justificación |
|------|----------|---------------|
| **Parser** | Pratt parser | Simple, extensible, maneja precedencia fácilmente |
| **AST** | Visitor pattern | Permite múltiples operaciones sin modificar nodos |
| **Resolver** | Pila de scopes | Eficiente y fácil de implementar |
| **Inferencia** | Protocolos sintetizados | Sigue la especificación A.9.5 |
| **Backend** | BANNER IR | Diseñado específicamente para HULK |
| **VM** | Stack-based | Simple y didáctica |
| **Value** | NaN boxing | Optimización de memoria y velocidad |
| **GC** | Mark-sweep | Clásico, fácil de entender e implementar |

---

## 7. Limitaciones Conocidas

| Limitación | Descripción |
|------------|-------------|
| **Funciones nativas** | Solo `print` está implementada. Faltan `clock()`, `sqrt()`, `sin()`, etc. |
| **Módulos** | No hay sistema de imports/módulos |
| **Optimizaciones** | No se implementaron optimizaciones (peephole, constant folding) |
| **Vectores** | No implementados (sección A.12) |
| **Functores/Lambdas** | Implementación parcial (sección A.13) |
| **Macros** | Placeholder, sin pattern matching completo (sección A.14) |
| **Interfaz gráfica** | Solo línea de comandos |

---

## 8. Pruebas

El proyecto incluye **14 pruebas** en `tests/input/` que cubren:

1. `01_literals.hulk` - Literales (números, strings, booleanos, nil)
2. `02_arithmetic.hulk` - Operaciones aritméticas
3. `03_strings.hulk` - Concatenación de strings
4. `04_variables.hulk` - Variables y `let`
5. `05_scope.hulk` - Scopes anidados
6. `06_if.hulk` - Condicionales `if`/`elif`/`else`
7. `07_while.hulk` - Bucle `while`
8. `08_for.hulk` - Bucle `for`
9. `09_functions.hulk` - Funciones (incluyendo recursión)
10. `10_classes.hulk` - Clases y objetos
11. `11_inheritance.hulk` - Herencia
12. `12_factorial_complete.hulk` - Programa completo: factorial
13. `13_fibonacci_complete.hulk` - Programa completo: Fibonacci
14. `14_math_utils.hulk` - Utilidades matemáticas

**Ejecución de pruebas:**
```bash
make test
```

---

## 9. Compilación y Uso

### 9.1 Requisitos
- Compilador C++17 (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+
- Make (Linux/Unix) o mingw32-make (Windows)

### 9.2 Compilación
```bash
make build
```

### 9.3 Uso
```bash
# Compilar archivo .hulk a ./output
./hulk archivo.hulk

# Ejecutar el programa generado
./output

# Modo REPL (solo desarrollo)
./hulk

# Opciones de ayuda
./hulk --help
./hulk --version
```

### 9.4 Códigos de salida
| Código | Tipo de error |
|--------|---------------|
| 0 | Éxito |
| 1 | LEXICAL |
| 2 | SYNTACTIC |
| 3 | SEMANTIC |

---

## 10. Ejemplo de Compilación

### Entrada (`examples/factorial.hulk`)
```hulk
function factorial(n) {
    if (n <= 1) return 1;
    else return n * factorial(n - 1);
}

for (var i = 0; i <= 5; i = i + 1) {
    print i @ "! = " @ factorial(i);
}
```

### Compilación y ejecución
```bash
./hulk examples/factorial.hulk
./output
```

### Salida
```
0! = 1
1! = 1
2! = 2
3! = 6
4! = 24
5! = 120
```

---

## 11. Conclusiones

El HULK Compiler es una implementación completa y funcional del lenguaje HULK, que cumple con los requisitos del contrato de interfaz. El proyecto demuestra:

1. **Dominio de técnicas de compilación**: scanning, parsing, AST, resolución, inferencia de tipos
2. **Arquitectura modular**: separación clara entre frontend y backend
3. **Portabilidad**: compila en Windows (MSYS2) y Linux
4. **Cumplimiento del contrato**: Makefile, códigos de salida, formato de error, generación de `./output`
5. **Documentación**: README.md, REPORT.md, ejemplos, pruebas

El compilador sienta las bases para futuras extensiones como optimizaciones, sistema de módulos, librería estándar completa y soporte para más construcciones del lenguaje HULK.

---

## 12. Referencias

1. **Especificación HULK** - Alejandro Piad Morffis, Universidad de La Habana
2. **Crafting Interpreters** - Robert Nystrom (https://craftinginterpreters.com/)
3. **Pratt Parsers: Expression Parsing Made Easy** - Bob Nystrom
4. **IEEE Standard for Floating-Point Arithmetic (IEEE 754)**
5. **Compilers: Principles, Techniques, and Tools** (Dragon Book) - Aho, Lam, Sethi, Ullman

---

<<<<<<< HEAD
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
git clone https://github.com/amircalabel/Hulk-compiler.git
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
- [x] **AstPrinter** (debugging del AST)
- [x] **Resolver** (análisis de ámbitos)
- [x] **Type Inferer** (inferencia de tipos, sección A.9)
- [x] **Type Checker** (verificación de tipos)
- [x] **Backend**: Generación de código BANNER IR
- [x] **Virtual Machine**: Ejecución de bytecode
- [ ] **Garbage Collector** (mark-sweep)
- [ ] **Optimizaciones** (NaN boxing, hash table improvements)

## 👥 Autor

- Amircal Abel Metelis Matui - *Implementación inicial*

## 🙏 Agradecimientos

- Robert Nystrom por *Crafting Interpreters*, una obra maestra que hizo accesible el mundo de los compiladores
- Prof. Alejandro Piad Morffis por la especificación de HULK
- La comunidad de University of Havana por el diseño del lenguaje

---

*"HULK no es solo un lenguaje, es una forma de pensar la computación."* — Piad, 2026

*Reporte generado el 15 de junio de 2025 para la entrega del proyecto de Compilación.*
