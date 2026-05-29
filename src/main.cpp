// src/main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <memory>

// Frontend
#include "scanner/Scanner.hpp"
#include "scanner/Token.hpp"
#include "parser/Parser.hpp"
#include "ast/Expr.hpp"
#include "ast/Stmt.hpp"
#include "ast/AstPrinter.hpp"
#include "resolver/Resolver.hpp"
#include "inferer/TypeInferer.hpp"
#include "interpreter/Interpreter.hpp"

// Backend
#include "backend/banner/BannerGenerator.hpp"
#include "backend/banner/BannerIR.hpp"
#include "backend/vm/VM.hpp"
#include "backend/vm/Value.hpp"
#include "backend/vm/CallFrame.hpp"

// ============================================================
// Flags de depuración (compilar con -DDEBUG para habilitar)
// ============================================================
// #define DEBUG_PRINT_TOKENS
// #define DEBUG_PRINT_AST
// #define DEBUG_PRINT_BANNER
// #define DEBUG_TRACE_EXECUTION

using namespace hulk;
using namespace hulk::backend;

// ============================================================
// Variables globales de error
// ============================================================
bool hadError = false;
bool hadResolverError = false;
bool hadTypeError = false;
bool hadRuntimeError = false;

// ============================================================
// Reporte de errores (frontend)
// ============================================================

void report(int line, const std::string& where, const std::string& message) {
    std::cerr << "\033[31m[line " << line << "] Error" << where << ": " << message << "\033[0m" << std::endl;
    hadError = true;
}

void error(int line, const std::string& message) {
    report(line, "", message);
}

void error(const Token& token, const std::string& message) {
    if (token.type == TokenType::TOKEN_EOF) {
        report(token.line, " at end", message);
    } else {
        report(token.line, " at '" + token.lexeme + "'", message);
    }
    hadError = true;
}

// ============================================================
// Reporte de errores (resolver)
// ============================================================

void resolverError(const Token& token, const std::string& message) {
    std::cerr << "\033[33m[line " << token.line << "] Resolution Error";
    if (token.type != TokenType::TOKEN_EOF) {
        std::cerr << " at '" << token.lexeme << "'";
    }
    std::cerr << ": " << message << "\033[0m" << std::endl;
    hadResolverError = true;
    hadError = true;
}

// ============================================================
// Reporte de errores (type inferer)
// ============================================================

void typeError(const Token& token, const std::string& expected, const std::string& found) {
    std::cerr << "\033[35m[line " << token.line << "] Type Error";
    if (token.type != TokenType::TOKEN_EOF) {
        std::cerr << " at '" << token.lexeme << "'";
    }
    std::cerr << ": expected " << expected << ", found " << found << "\033[0m" << std::endl;
    hadTypeError = true;
    hadError = true;
}

// ============================================================
// Versión del compilador
// ============================================================

void printVersion() {
    std::cout << "HULK Compiler v0.1.0" << std::endl;
    std::cout << "Copyright (c) 2024" << std::endl;
    std::cout << "Language: HULK (Havana University Language for Kompiers)" << std::endl;
    std::cout << "Backend: BANNER IR + Stack-based VM" << std::endl;
}

void printHelp() {
    std::cout << "Usage: hulk [options] [script]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --help, -h     Show this help message" << std::endl;
    std::cout << "  --version, -v  Show version information" << std::endl;
    std::cout << "  --dump-tokens  Print tokens before compilation" << std::endl;
    std::cout << "  --dump-ast     Print AST before code generation" << std::endl;
    std::cout << "  --dump-banner  Print BANNER IR before execution" << std::endl;
    std::cout << "  --no-exec      Compile but do not execute" << std::endl;
    std::cout << std::endl;
    std::cout << "If no script is provided, starts REPL mode." << std::endl;
}

// ============================================================
// Compilación y ejecución
// ============================================================

struct CompileOptions {
    bool dumpTokens = false;
    bool dumpAST = false;
    bool dumpBanner = false;
    bool noExec = false;
    bool timing = false;
};

struct CompileResult {
    bool success = false;
    std::vector<std::unique_ptr<Stmt>> statements;
    std::unique_ptr<hulk::backend::BannerProgram> bannerProgram;
    std::unique_ptr<hulk::backend::ObjFunction> compiledFunction;
    double frontendTime = 0;
    double backendTime = 0;
    double executionTime = 0;
};

// ============================================================
// Declaraciones adelantadas de funciones
// ============================================================
void run(const std::string& source);
void runFile(const std::string& path, const CompileOptions& options);
void runRepl(const CompileOptions& options);

// ============================================================
// Función principal de ejecución (compila y ejecuta código HULK)
// ============================================================
void run(const std::string& source) {
    // ============================================================
    // FASE 1: SCANNER (Lexer)
    // ============================================================
    Scanner scanner(source);
    std::vector<Token> tokens = scanner.scanTokens();
    
    if (hadError) {
        hadError = false;
        return;
    }
    
    #ifdef DEBUG_PRINT_TOKENS
    std::cout << "\n=== TOKENS ===" << std::endl;
    for (const auto& token : tokens) {
        std::cout << "  " << token.toString() << std::endl;
    }
    std::cout << std::endl;
    #endif
    
    // ============================================================
    // FASE 2: PARSER
    // ============================================================
    Parser parser(tokens);
    auto statements = parser.parse();
    
    if (hadError) {
        hadError = false;
        return;
    }
    
    // ============================================================
    // FASE 3: RESOLVER (Análisis de ámbito)
    // ============================================================
    Interpreter interpreter;
    Resolver resolver(interpreter);
    resolver.resolve(statements);
    
    if (hadResolverError) {
        hadResolverError = false;
        return;
    }
    
    // ============================================================
    // FASE 4: TYPE INFERER (Inferencia de tipos)
    // ============================================================
    TypeInferer inferer(resolver);
    inferer.infer(statements);
    
    if (hadTypeError) {
        hadTypeError = false;
        return;
    }
    
    // ============================================================
    // FASE 5: IMPRIMIR AST (para debugging)
    // ============================================================
    #ifdef DEBUG_PRINT_AST
    std::cout << "\n=== AST ===" << std::endl;
    AstPrinter printer;
    std::cout << printer.print(statements) << std::endl;
    #endif
    
    // ============================================================
    // FASE 6: INTERPRETAR (tree-walk interpreter)
    // ============================================================
    try {
        interpreter.interpret(statements);
    } catch (const std::exception& e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        hadRuntimeError = true;
    }
    
    // Limpiar errores para la siguiente ejecución (REPL)
    hadError = false;
    hadResolverError = false;
    hadTypeError = false;
}

// ============================================================
// Compilación completa (con backend BANNER)
// ============================================================
CompileResult compile(const std::string& source, const CompileOptions& options) {
    CompileResult result;
    auto startTotal = std::chrono::high_resolution_clock::now();
    
    // ============================================================
    // FASE 1: SCANNER (Lexer)
    // ============================================================
    auto startFrontend = std::chrono::high_resolution_clock::now();
    
    Scanner scanner(source);
    std::vector<Token> tokens = scanner.scanTokens();
    
    if (hadError) {
        return result;
    }
    
    #ifdef DEBUG_PRINT_TOKENS
    if (options.dumpTokens) {
        std::cout << "\n=== TOKENS ===" << std::endl;
        for (const auto& token : tokens) {
            std::cout << "  " << token.toString() << std::endl;
        }
        std::cout << std::endl;
    }
    #endif
    
    // ============================================================
    // FASE 2: PARSER
    // ============================================================
    Parser parser(tokens);
    auto statements = parser.parse();
    
    if (hadError) {
        return result;
    }
    
    result.statements = std::move(statements);
    
    // ============================================================
    // FASE 3: RESOLVER (Análisis de ámbito)
    // ============================================================
    Interpreter interpreter;
    Resolver resolver(interpreter);
    resolver.resolve(result.statements);
    
    if (hadResolverError) return result;
    
    // ============================================================
    // FASE 4: TYPE INFERER (Inferencia de tipos)
    // ============================================================
    TypeInferer inferer(resolver);
    inferer.infer(result.statements);
    
    if (hadTypeError) return result;
    
    auto endFrontend = std::chrono::high_resolution_clock::now();
    result.frontendTime = std::chrono::duration<double>(endFrontend - startFrontend).count();
    
    // ============================================================
    // FASE 5: DUMP AST (opcional)
    // ============================================================
    if (options.dumpAST) {
        AstPrinter printer;
        std::cout << "\n=== AST ===" << std::endl;
        std::cout << printer.print(result.statements) << std::endl;
        std::cout << std::endl;
    }
    
    // ============================================================
    // FASE 6: BACKEND - Generación de BANNER IR
    // ============================================================
    if (!options.noExec) {
        auto startBackend = std::chrono::high_resolution_clock::now();
        
        BannerGenerator generator(resolver, inferer);
        auto bannerProgram = generator.generate(result.statements);
        result.bannerProgram = std::make_unique<hulk::backend::BannerProgram>(std::move(bannerProgram));
        
        if (options.dumpBanner && result.bannerProgram) {
            std::cout << "\n=== BANNER IR ===" << std::endl;
            std::cout << result.bannerProgram->toString() << std::endl;
        }
        
        auto endBackend = std::chrono::high_resolution_clock::now();
        result.backendTime = std::chrono::duration<double>(endBackend - startBackend).count();
        
        // ============================================================
        // FASE 7: VM - Ejecución
        // ============================================================
        auto startExecution = std::chrono::high_resolution_clock::now();
        
        hulk::backend::VM vm;
        auto execResult = vm.interpret(*result.bannerProgram);
        
        if (execResult != hulk::backend::InterpretResult::INTERPRET_OK) {
            hadRuntimeError = true;
            return result;
        }
        
        auto endExecution = std::chrono::high_resolution_clock::now();
        result.executionTime = std::chrono::duration<double>(endExecution - startExecution).count();
    }
    
    result.success = true;
    return result;
}

// ============================================================
// Ejecución de archivo
// ============================================================

void runFile(const std::string& path, const CompileOptions& options) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << path << "'" << std::endl;
        exit(74);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    if (options.timing) {
        std::cout << "\n=== Compiling: " << path << " ===" << std::endl;
    }
    
    auto result = compile(source, options);
    
    if (options.timing && result.success) {
        std::cout << "\n=== Timing ===" << std::endl;
        std::cout << "  Frontend (scan+parse+resolve+infer): " << result.frontendTime << "s" << std::endl;
        if (!options.noExec) {
            std::cout << "  Backend (BANNER generation): " << result.backendTime << "s" << std::endl;
            std::cout << "  Execution (VM): " << result.executionTime << "s" << std::endl;
            std::cout << "  Total: " << (result.frontendTime + result.backendTime + result.executionTime) << "s" << std::endl;
        } else {
            std::cout << "  Total: " << result.frontendTime << "s" << std::endl;
        }
    }
    
    if (hadError) exit(65);
    if (hadRuntimeError) exit(70);
}

// ============================================================
// Función para el REPL (usa el tree-walk interpreter)
// ============================================================

void runReplLine(const std::string& line) {
    // Para el REPL, usamos run() que es más simple
    run(line);
}

// ============================================================
// REPL (Read-Eval-Print Loop)
// ============================================================

void runRepl(const CompileOptions& options) {
    std::cout << "HULK Interpreter v0.1.0" << std::endl;
    std::cout << "Type 'exit' to quit, 'help' for help" << std::endl;
    std::cout << std::endl;
    
    std::string line;
    
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        if (line.empty()) continue;
        
        if (line == "exit") {
            std::cout << "Goodbye!" << std::endl;
            break;
        }
        
        if (line == "help") {
            std::cout << "HULK REPL Commands:\n";
            std::cout << "  exit    - Exit the REPL\n";
            std::cout << "  help    - Show this help\n";
            std::cout << "\nExamples:\n";
            std::cout << "  > print \"Hello\";\n";
            std::cout << "  > 1 + 2 * 3;\n";
            std::cout << "  > let x = 42 in print x;\n";
            std::cout << "  > function fib(n) => if (n <= 1) n else fib(n-1) + fib(n-2);\n";
            continue;
        }
        
        // Ejecutar la línea
        runReplLine(line);
        
        // Resetear errores para la siguiente línea
        hadError = false;
        hadResolverError = false;
        hadTypeError = false;
        hadRuntimeError = false;
    }
}

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[]) {
    CompileOptions options;
    std::string scriptPath;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printHelp();
            return 0;
        }
        else if (arg == "--version" || arg == "-v") {
            printVersion();
            return 0;
        }
        else if (arg == "--dump-tokens") {
            options.dumpTokens = true;
        }
        else if (arg == "--dump-ast") {
            options.dumpAST = true;
        }
        else if (arg == "--dump-banner") {
            options.dumpBanner = true;
        }
        else if (arg == "--no-exec") {
            options.noExec = true;
        }
        else if (arg == "--timing") {
            options.timing = true;
        }
        else if (arg[0] != '-') {
            scriptPath = arg;
        }
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printHelp();
            return 64;
        }
    }
    
    // Run
    if (!scriptPath.empty()) {
        runFile(scriptPath, options);
    } else {
        runRepl(options);
    }
    
    return 0;
}