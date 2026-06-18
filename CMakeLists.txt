# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(HulkCompiler VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Opciones de compilación
option(ENABLE_DEBUG "Enable debug mode" OFF)
option(ENABLE_GC_STRESS "Enable GC stress testing" OFF)

if(ENABLE_DEBUG)
    add_compile_definitions(DEBUG)
    add_compile_definitions(DEBUG_PRINT_TOKENS)
    add_compile_definitions(DEBUG_PRINT_AST)
    add_compile_definitions(DEBUG_PRINT_BANNER)
    add_compile_definitions(DEBUG_TRACE_EXECUTION)
    message(STATUS "Debug mode enabled")
endif()

if(ENABLE_GC_STRESS)
    add_compile_definitions(DEBUG_STRESS_GC)
    message(STATUS "GC stress mode enabled")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# ============================================================
# Directorios de inclusión - ¡ESTA ES LA PARTE CLAVE!
# ============================================================
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# ============================================================
# Frontend
# ============================================================

set(FRONTEND_SOURCES
    src/main.cpp

    # Scanner
    src/scanner/Token.cpp
    src/scanner/Scanner.cpp

    # AST
    src/ast/Expr.cpp
    src/ast/Stmt.cpp
    src/ast/AstPrinter.cpp

    # Parser
    src/parser/Parser.cpp

    # Resolver
    src/resolver/Resolver.cpp

    # Inferer
    src/inferer/TypeInferer.cpp

    # Interpreter
    src/interpreter/Interpreter.cpp 
)

# ============================================================
# Backend
# ============================================================

set(BACKEND_SOURCES
    # Backend utilities
    src/backend/ASTSerializer.cpp
    src/backend/CodeGenerator.cpp

    # BANNER IR
    src/backend/banner/BannerIR.cpp
    src/backend/banner/BannerGenerator.cpp

    # VM
    src/backend/vm/Value.cpp
    src/backend/vm/CallFrame.cpp
    src/backend/vm/VM.cpp
)

# ============================================================
# Headers
# ============================================================

set(HEADERS
    src/scanner/Token.hpp
    src/scanner/Scanner.hpp
    src/ast/Expr.hpp
    src/ast/Stmt.hpp
    src/ast/AstPrinter.hpp
    src/parser/Parser.hpp
    src/backend/ASTSerializer.hpp
    src/backend/CodeGenerator.hpp
    src/backend/banner/BannerIR.hpp
    src/backend/banner/BannerGenerator.hpp
    src/backend/vm/Value.hpp
    src/backend/vm/CallFrame.hpp
    src/backend/vm/OpCode.hpp
    src/backend/vm/VM.hpp
    src/backend/vm/GC.hpp
)

# ============================================================
# Ejecutable
# ============================================================

add_executable(hulk ${FRONTEND_SOURCES} ${BACKEND_SOURCES} ${HEADERS})

# ============================================================
# Testing
# ============================================================

option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()