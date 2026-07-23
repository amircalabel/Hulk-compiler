# HULK Compiler — Backend Completion Notes

This document summarizes the work done to complete the compiler so it meets the
course requirements: `make build` → `./hulk`, and `./hulk <file.hulk>` →
`./output` (a native Linux x86_64 binary).

## Pipeline

```
Scanner → Parser → (semantic analysis) → Backend (CodeGenerator) → ./output
```

`./hulk file.hulk` transpiles the AST to a self-contained C++ program that
implements HULK semantics (dynamic values, lexical environments, functions,
classes with inheritance and dynamic dispatch) and compiles it to the native
binary `./output`.

## What was fixed / implemented

### 1. Parser (`src/parser/Parser.cpp`)
The original parser failed to parse almost every non-trivial construct. Rewritten
to correctly handle:
- `let … in …` (single and multiple bindings, desugared to nested scopes),
  and top-level `let x = e;` global bindings (no `in`).
- `if / elif / else` in both expression and statement positions, including the
  `… ; else …` style used in the test suite.
- `while` and `for` as both expressions and statements; `for (var i = 0; i < n; i = i + 1)`.
- Functions: `f(x) => e;` and block bodies with `return`; recursion.
- Classes: `type C(args) { attr = e; method() … }`, `inherits`, `self`,
  `new C(...)`, property access `obj.attr`, method calls `obj.m(...)`,
  destructive `obj.attr := v`.
- Operators: `^` (right-assoc power), `%`, `@`/`@@` concatenation, comparisons,
  `and`/`or`/`not`, `:=` and `=` assignment.
- Stray/optional semicolons after declarations.

### 2. New AST nodes (`src/ast/Expr.{hpp,cpp}`)
Added `GetExpr`, `SetExpr`, `SelfExpr`, `NewExpr` for OOP, with default
(non-pure) visitor methods so existing visitors keep compiling. `ClassDeclStmt`
gained `attributeInitializers`.

### 3. Backend (`src/backend/CodeGenerator.{hpp,cpp}`)
Rewritten to handle **all** expression and statement nodes (literals, arithmetic,
strings, comparisons, logic, `let`, `if`, `while`, `for`, blocks, calls,
functions, classes, inheritance, `self`, fields, method dispatch). Includes a
semantic pass that reports undefined functions / undefined types.

### 4. Error reporting & exit codes (`src/main.cpp`)
Errors are reported as `(line,col) TYPE: message` where TYPE is
`LEXICAL` / `SYNTACTIC` / `SEMANTIC`, with exit codes **1 / 2 / 3** respectively.

### 5. BANNER VM (`src/backend/vm/VM.cpp`)
`VM::interpret(const BannerProgram&)` was a no-op; it now actually executes the
BANNER IR instruction stream (LOAD/STORE, arithmetic, LABEL/GOTO/IF_GOTO,
PRINT, …). It backs the interactive REPL path.

## Verified

All 14 programs in `tests/input/` compile and run with correct output:
literals, arithmetic, strings, variables, scope, if, while, for, functions,
classes, inheritance, and the complete factorial/fibonacci/math-utils programs.
Error cases produce the correct format and exit codes. Builds cleanly with both
`g++` and `clang++` on Ubuntu 24.04.

## Build & run

```sh
make build            # produces ./hulk
./hulk program.hulk   # produces ./output
./output              # runs the program
make test             # compiles every tests/input/*.hulk
```
