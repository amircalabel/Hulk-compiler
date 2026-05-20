cmake_minimum_required(VERSION 3.10)
project(HulkCompiler VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Opciones de compilación
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Headers
set(HEADERS
    src/scanner/Token.h
    src/scanner/Scanner.h
    src/ast/Expr.h
    src/ast/Stmt.h
    src/ast/AstPrinter.h
    src/parser/Parser.h
)

# Sources
set(SOURCES
    src/main.cpp
    src/scanner/Token.cpp
    src/scanner/Scanner.cpp
    src/ast/Expr.cpp
    src/ast/Stmt.cpp
    src/ast/AstPrinter.cpp
    src/parser/Parser.cpp
)

# Ejecutable
add_executable(hulk ${SOURCES} ${HEADERS})

# Habilitar debugging en modo debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(hulk PRIVATE DEBUG)
endif()