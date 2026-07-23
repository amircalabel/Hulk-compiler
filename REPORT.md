# HULK Compiler - Reporte Técnico

## 1. Resumen del Proyecto

El **HULK Compiler** es una implementación parcial del compilador para el lenguaje didáctico **HULK** (Havana University Language for Kompiers), desarrollado como parte de un curso de Compiladores. El frontend funciona para análisis léxico, sintáctico, semántico e inferencia de tipos, pero el backend está todavía en desarrollo. La compilación de archivos a `./output` existe, aunque solo cubre un subconjunto limitado de programas.

El proyecto está escrito en **C++17** y sigue una arquitectura de pipeline clásica, con influencias notables de "Crafting Interpreters" de Robert Nystrom. El repositorio contiene aproximadamente 5,700 líneas de código distribuidas en 8 módulos principales, lo que demuestra una inversión significativa en el diseño y la estructura del compilador.

---

## 2. Arquitectura del Compilador

El compilador sigue un pipeline clásico de compilación, dividido en fases bien definidas que transforman el código fuente desde su forma textual hasta un ejecutable nativo:

```
Código HULK → Scanner → Parser → Resolver → TypeInferer → Backend → ./output
```

### 2.1 Estructura de Directorios

La organización del código refleja la separación de responsabilidades entre las diferentes fases del compilador:

```
Hulk-compiler/
├── examples/               # Programas de ejemplo compatibles con el subconjunto actual
│   ├── class.hulk
│   ├── factorial.hulk
│   ├── fibonacci.hulk
│   └── inheritance.hulk
├── src/
│   ├── ast/               # Abstract Syntax Tree (Expr, Stmt, AstPrinter)
│   │   ├── Expr.hpp/cpp   # Nodos para expresiones (binarias, unarias, literales, etc.)
│   │   ├── Stmt.hpp/cpp   # Nodos para statements (print, return, declaraciones)
│   │   └── AstPrinter.hpp/cpp # Pretty printer para debugging del AST
│   ├── backend/           # Backend en desarrollo
│   │   ├── banner/        # BANNER IR (tres direcciones)
│   │   │   ├── BannerIR.hpp/cpp    # Definición de instrucciones BANNER
│   │   │   └── BannerGenerator.hpp/cpp # Generador de código BANNER
│   │   ├── vm/            # Máquina virtual
│   │   │   ├── VM.hpp/cpp         # Ejecutor de bytecode
│   │   │   ├── Value.hpp/cpp      # Representación de valores (NaN boxing)
│   │   │   ├── CallFrame.hpp/cpp  # Stack frames para funciones
│   │   │   ├── OpCode.hpp         # Definición de opcodes
│   │   │   └── GC.hpp             # Garbage collector (mark-sweep)
│   │   ├── CodeGenerator.hpp/cpp  # Generador de ejecutable nativo
│   │   └── ASTSerializer.hpp/cpp  # Serializador de AST a C++
│   ├── inferer/           # Inferencia de tipos (sección A.9 de la especificación)
│   │   ├── TypeInferer.hpp
│   │   └── TypeInferer.cpp
│   ├── interpreter/       # Tree-walk interpreter y REPL
│   │   ├── Interpreter.hpp
│   │   └── Interpreter.cpp
│   ├── parser/            # Parser Pratt (descenso recursivo con precedencia)
│   │   ├── Parser.hpp
│   │   └── Parser.cpp
│   ├── resolver/          # Resolución de ámbitos y variables
│   │   ├── Resolver.hpp
│   │   └── Resolver.cpp
│   ├── scanner/           # Lexer (análisis léxico)
│   │   ├── Scanner.hpp
│   │   ├── Scanner.cpp
│   │   ├── Token.hpp
│   │   └── Token.cpp
│   ├── type/              # Representación de tipos del sistema de tipos HULK
│   │   ├── Type.hpp
│   │   └── Type.cpp
│   └── main.cpp           # Punto de entrada (REPL, ejecución de archivos, opciones CLI)
├── tests/                 # Pruebas de entrada
├── CMakeLists.txt        # Configuración de build
├── Makefile              # Comandos de build y test
└── README.md             # Documentación del proyecto
```

### 2.2 Flujo de Compilación

El flujo de compilación actual se puede resumir en los siguientes pasos:

1. **Scanner**: Lee el código fuente y produce una secuencia de tokens.
2. **Parser**: Construye un Abstract Syntax Tree (AST) a partir de los tokens.
3. **Resolver**: Recorre el AST y resuelve ámbitos, conectando usos con declaraciones.
4. **TypeInferer**: Infiere tipos para expresiones y variables no anotadas.
5. **Backend**: Genera un ejecutable nativo (`./output`) a partir del AST.

---

## 3. Componentes del Frontend

### 3.1 Scanner (Análisis Léxico)

El scanner es un analizador léxico manual que recorre el código fuente carácter por carácter y lo agrupa en tokens. Los tokens incluyen:

- **Palabras clave**: `let`, `in`, `function`, `type`, `protocol`, `def`, `if`, `elif`, `else`, `while`, `for`, `return`, `print`, `new`, `inherits`, `self`, `base`, `is`, `as`, `true`, `false`, `nil`, `and`, `or`, `not`
- **Operadores**: `+`, `-`, `*`, `/`, `^`, `%`, `@`, `@@`, `:=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&`, `|`, `!`, `=>`
- **Literales**: números (enteros y decimales), strings (con soporte multi-línea), booleanos y `nil`
- **Identificadores**: nombres de variables, funciones y tipos
- **Puntuación**: `(`, `)`, `{`, `}`, `;`, `,`, `.`
- **Comentarios**: `//` hasta fin de línea

El scanner maneja errores léxicos como caracteres inválidos o strings sin cerrar, reportándolos en el formato requerido `(line,col) LEXICAL: mensaje`. Esta funcionalidad cumple con los requisitos del contrato de evaluación.

**Características adicionales del scanner:**

- Soporte para el carácter `$` en identificadores (utilizado para placeholders de macros)
- Técnica de "maximal munch" para tokens multi-carácter (ej. `@@`, `:=`, `=>`)
- Manejo de strings con soporte para nuevas líneas internas
- Comentarios de línea con `//`

### 3.2 Parser (Análisis Sintáctico)

El parser implementa un **Pratt parser** (descenso recursivo con manejo de precedencia de operadores). Las principales características son:

**Expresiones soportadas:**
- Literales (números, strings, booleanos, nil)
- Binarias (`+`, `-`, `*`, `/`, `^`, `%`, `@`, `@@`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&`, `|`)
- Unarias (`!`, `-` prefijos)
- Agrupación con paréntesis
- Variables y asignación destructiva (`:=`)
- `let ... in ...` (con bindings múltiples)
- `if` / `elif` / `else`
- `while` y `for`
- Bloques `{ ... }` como expresiones

**Statements soportados:**
- `print`
- `return`
- Bloques `{ ... }`
- Declaraciones de variables (`var`)
- Funciones (inline y full-form)
- Clases (`type`)
- Protocolos (`protocol`)
- Macros (`def`)

**Jerarquía de precedencia (de menor a mayor):**

| Nivel | Operadores | Asociatividad |
|-------|------------|---------------|
| 1 | `:=` | Derecha |
| 2 | `or`, `|` | Izquierda |
| 3 | `and`, `&` | Izquierda |
| 4 | `==`, `!=` | Izquierda |
| 5 | `<`, `<=`, `>`, `>=` | Izquierda |
| 6 | `+`, `-` | Izquierda |
| 7 | `*`, `/`, `%`, `^` | Izquierda |
| 8 | `@`, `@@` | Izquierda |
| 9 | `!`, `-` (unarios) | Derecha |

### 3.3 Resolver (Análisis de Ámbito)

El resolver recorre el AST y conecta cada uso de variable con su declaración correspondiente. Utiliza una **pila de scopes** donde cada scope es un mapa de nombres a información de variable. Las principales responsabilidades del resolver son:

- Detectar variables no definidas
- Prevenir usos antes de inicialización
- Manejar scopes anidados y shadowing
- Resolver referencias a `self` y `base` en clases
- Identificar upvalues para closures

El resolver implementa un análisis de ámbito estático siguiendo el patrón descrito en el capítulo 11 de "Crafting Interpreters", lo que permite detectar errores semánticos temprano en el proceso de compilación.

### 3.4 TypeInferer (Inferencia de Tipos)

El TypeInferer implementa la **sección A.9** de la especificación de HULK, que describe el sistema de inferencia de tipos del lenguaje. Sus principales funcionalidades incluyen:

- **Inferencia de expresiones (A.9.2)**: literales → tipo concreto; binarias según operador
- **Inferencia de símbolos (A.9.3)**: para variables/funciones/parámetros sin anotación de tipo
- **Protocolos sintetizados (A.9.5)**: creación de protocolos implícitos para funciones que acceden a métodos
- **LCA (Lowest Common Ancestor)**: para expresiones `if` con múltiples ramas

El sistema de tipos de HULK se representa mediante la clase `Type`, que soporta:

- Tipos primitivos: `Number`, `String`, `Boolean`, `Nil`
- Tipos compuestos: `Object`, `Class`, `Protocol`, `Function`, `Generic`
- Variables de tipo para inferencia (unificación)

El TypeInferer es instanciado en el pipeline después del Resolver y puede inferir tipos básicos en el AST, aunque actualmente su integración con el backend está pendiente.

---

## 4. Backend

### 4.1 BANNER IR (Intermediate Representation)

El backend incluye una representación de **BANNER** (Basic 3-Address liNear iNtEmendate Representation), un IR de tres direcciones diseñado específicamente para HULK. La estructura de un programa BANNER incluye:

- **Sección `.TYPES`**: layouts de objetos en memoria (atributos y métodos)
- **Sección `.DATA`**: pool de constantes (strings y literales)
- **Sección `.CODE`**: instrucciones lineales con etiquetas y saltos

**Instrucciones principales:**
- Movimiento de datos: `LOAD`, `STORE`, `COPY`
- Aritmética: `ADD`, `SUB`, `MUL`, `DIV`, `POW`
- Objetos: `ALLOCATE`, `GETATTR`, `SETATTR`
- Control de flujo: `LABEL`, `GOTO`, `IF_GOTO`
- Llamadas: `PARAM`, `CALL`, `VCALL`, `RETURN`

La infraestructura BANNER está implementada y compila correctamente, aunque no está completamente integrada en la ruta principal de compilación de archivos.

### 4.2 Virtual Machine (VM)

El proyecto incluye una **máquina virtual de pila** que ejecuta bytecode BANNER. Sus componentes principales son:

| Componente | Descripción |
|------------|-------------|
| **`Value`** | Representación de valores con NaN boxing (64 bits) |
| **`VM`** | Bucle principal de ejecución (`run()`) con switch sobre opcodes |
| **`CallFrame`** | Stack frame para llamadas a funciones |
| **`Obj*`** | Objetos en heap (strings, funciones, clases, instancias) |
| **Upvalues** | Implementación de closures (cap. 25 de "Crafting Interpreters") |
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
- Representación de valores (`HulkValue` como `std::variant`)
- Entorno de variables (`Environment` con `define`/`get`/`assign`)
- Operadores aritméticos y lógicos básicos
- Funciones de conversión y utilidad (`stringify`, `isTruthy`, `valuesEqual`)

---

## 5. Estado Actual del Lenguaje

### 5.1 Gramática y Parsing

El parser admite todas las construcciones sintácticas del lenguaje HULK, incluyendo:

- Expresiones binarias, unarias, literales y agrupadas
- `print` y `return`
- `if` / `elif` / `else`
- Bucles `while` y `for`
- Declaraciones `var`
- Funciones en forma inline (`=>`) y en bloque (`{ ... }`)
- Declaraciones de `type` y `protocol`
- Macros `def`

### 5.2 Limitaciones de Generación

La generación nativa en `./output` actualmente solo cubre un subconjunto de la gramática:

**Soportado:**
- `PrintStmt` con literales y operaciones aritméticas básicas
- `ExpressionStmt` con literales, variables y operaciones `+`, `-`, `*`, `/`
- Literales numéricos y strings

**No soportado (en el ejecutable nativo):**
- Declaraciones `var` y asignaciones `:=`
- `let ... in ...`
- Funciones definidas y llamadas internas
- Clases, métodos, herencia o `new`
- Macros y protocolos
- Control de flujo compleja (`if`, `while`, `for` como expresiones)
- Strings con concatenación (`@`, `@@`)

### 5.3 Soporte en REPL

El modo REPL (`./hulk` sin argumentos) utiliza el Interpreter tree-walk, que soporta un conjunto más amplio de construcciones:

- Todas las expresiones y statements del parser
- Variables y `let ... in ...`
- `if`, `while`, `for` como expresiones
- Funciones definidas y llamadas
- Clases básicas (sin herencia completa)

---

## 6. Limitaciones Conocidas

### 6.1 Backend Incompleto

El backend nativo está parcialmente implementado. La VM está implementada en código pero no se usa como motor de ejecución de archivos. La ruta de compilación de archivos actual utiliza `CodeGenerator`, que solo cubre un subconjunto reducido de la gramática.

### 6.2 BANNER IR Parcial

La generación de BANNER existe como infraestructura en `BannerGenerator.cpp` (875 LOC), pero no está integrada en la ruta principal de compilación de `./hulk`. La función `VM::interpret(const BannerProgram&)` es un placeholder que no ejecuta el programa BANNER generado.

### 6.3 Generación Nativa Limitada

`CodeGenerator` solo maneja 5 tipos de nodos AST, principalmente `PrintStmt`, `ExpressionStmt`, `LiteralExpr`, `BinaryExpr`, y `VariableExpr`. El resto de los nodos se traducen a comentarios `// Unhandled statement type`.

### 6.4 Ejemplos Ajustados

Los ejemplos en `examples/` fueron reemplazados para coincidir con el subconjunto actual. Un ejemplo válido es:

```hulk
print "Hello from HULK!";
print 1 + 2;
```

### 6.5 Soporte de Objetos

La gramática de clases y protocolos está presente en el AST y el parser la reconoce, pero la ejecución/compilación completa de objetos no está garantizada en el ejecutable nativo.

### 6.6 Soporte de Tipos

La inferencia de tipos (TypeInferer) está implementada como módulo, pero su integración con el backend está pendiente. Los tipos se verifican pero no afectan la generación de código.

---

## 7. Pruebas y Ejemplos

### 7.1 Suite de Pruebas

El repositorio contiene una suite de ejemplos de entrada en `tests/input/`. La mayoría son casos de parseo y análisis que cubren:

| Categoría | Archivos | Descripción |
|-----------|----------|-------------|
| Literales | `01_literals.hulk` | Números, strings, booleanos, nil |
| Aritmética | `02_arithmetic.hulk` | `+`, `-`, `*`, `/`, `^` |
| Strings | `03_strings.hulk` | Concatenación con `@` y `@@` |
| Variables | `04_variables.hulk`, `05_scope.hulk` | `let` y scopes anidados |
| Control | `06_if.hulk`, `07_while.hulk`, `08_for.hulk` | Condicionales y bucles |
| Funciones | `09_functions.hulk` | Definición y llamada |
| Clases | `10_classes.hulk`, `11_inheritance.hulk` | Objetos y herencia |
| Programas | `12_factorial_complete.hulk`, `13_fibonacci_complete.hulk`, `14_math_utils.hulk` | Ejemplos completos |

### 7.2 Ejemplo Válido Actual

El siguiente programa es un ejemplo válido para el compilador actual:

```hulk
print "Hello from HULK!";
print 1 + 2;
print 42;
```

Al compilar y ejecutar:

```bash
./hulk examples/hello.hulk
./output
```

La salida esperada es:

```
Hello from HULK!
3
42
```

---

## 8. Guía de Compilación y Uso

### 8.1 Requisitos

| Requisito | Versión |
|-----------|---------|
| **Compilador C++** | C++17 (GCC 7+, Clang 5+, MSVC 2017+) |
| **CMake** | 3.10+ |
| **Make** | GNU Make 3.81+ |
| **Sistema** | Linux x86_64 (para ejecutable generado) |

### 8.2 Compilación

```bash
# Clonar el repositorio
git clone https://github.com/amircalabel/Hulk-compiler.git
cd Hulk-compiler

# Compilar el compilador
make build

# El ejecutable './hulk' se genera en la raíz
```

**En Windows (MSYS2):**
```bash
mingw32-make build
```

### 8.3 Uso

```bash
# Compilar un archivo HULK a ejecutable
./hulk archivo.hulk

# Ejecutar el programa generado
./output

# Modo REPL (interactivo)
./hulk

# Opciones de ayuda
./hulk --help
./hulk --version
```

### 8.4 Códigos de Salida

| Código | Tipo de error | Descripción |
|--------|---------------|-------------|
| 0 | Éxito | Compilación correcta, se generó `./output` |
| 1 | LEXICAL | Error léxico (carácter inválido, string sin cerrar) |
| 2 | SYNTACTIC | Error sintáctico (falta `;`, paréntesis, etc.) |
| 3 | SEMANTIC | Error semántico (tipo incorrecto, variable no definida) |
| 65 | Compilación | Error general de compilación |

### 8.5 Formato de Errores

Todos los errores se reportan en **stderr** con el formato:

```
(line,col) TYPE: mensaje
```

**Ejemplos:**
```
(1,9) LEXICAL: unexpected character '$'
(2,3) SYNTACTIC: Expect expression at 'print'
(3,5) SEMANTIC: type mismatch — expected Number, got String
```

---

## 9. Conclusiones y Trabajo Futuro

El HULK Compiler es una base sólida para continuar el desarrollo. El frontend está implementado y funciona para análisis completo del lenguaje HULK, incluyendo scanner, parser, resolver y TypeInferer. El backend, sin embargo, aún requiere trabajo para soportar la generación y ejecución de programas más allá de un subconjunto simple.

### 9.1 Logros Actuales

- ✅ Scanner completo y funcional
- ✅ Parser con cobertura total de la gramática HULK
- ✅ Resolver para análisis de ámbitos
- ✅ TypeInferer implementado
- ✅ Interpreter funcional en modo REPL
- ✅ Generación de `./output` para subconjunto básico
- ✅ Códigos de salida y formato de errores según contrato

### 9.2 Trabajo Pendiente

| Prioridad | Tarea | Descripción |
|-----------|-------|-------------|
| 🔴 Alta | VM Integration | Completar `VM::interpret(const BannerProgram&)` y conectar al pipeline |
| 🔴 Alta | BANNER Integration | Integrar BANNER IR con la ruta de compilación de archivos |
| 🟡 Media | CodeGenerator | Ampliar para soportar `var`, `let`, `if`, `while`, `for`, funciones y objetos |
| 🟡 Media | Ejemplos | Actualizar ejemplos y pruebas para reflejar capacidades reales |
| 🟢 Baja | Optimizaciones | Implementar optimizaciones (peephole, constant folding) |
| 🟢 Baja | Vectores | Implementar soporte para vectores (sección A.12) |
| 🟢 Baja | Functores | Implementar soporte para functores y lambdas (sección A.13) |

### 9.3 Próximos Pasos Lógicos

1. **Completar `VM::interpret(const BannerProgram&)`** - Implementar la ejecución de bytecode BANNER
2. **Terminar la integración de BANNER IR** - Conectar el generador BANNER a la ruta de compilación de archivos
3. **Ampliar `CodeGenerator`** - Añadir soporte para `var`, `let`, `if`, `while`, `for`, funciones y objetos
4. **Actualizar ejemplos y pruebas** - Reflejar las capacidades reales del compilador
5. **Optimizar y refinar** - Mejorar el rendimiento y la cobertura del lenguaje

---

## 10. Referencias

1. **Especificación HULK** - Alejandro Piad Morffis, Universidad de La Habana
2. **Crafting Interpreters** - Robert Nystrom (https://craftinginterpreters.com/)
3. **Pratt Parsers: Expression Parsing Made Easy** - Bob Nystrom
4. **IEEE Standard for Floating-Point Arithmetic (IEEE 754)**
5. **Compilers: Principles, Techniques, and Tools** (Dragon Book) - Aho, Lam, Sethi, Ullman
6. **HULK Language Specification** - Documento proporcionado en el curso

---

*Reporte generado el 7 de julio de 2025 para la entrega del proyecto de Compilaciòn.*
