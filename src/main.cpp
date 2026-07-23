// src/main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>

// Frontend
#include "scanner/Scanner.hpp"
#include "scanner/Token.hpp"
#include "parser/Parser.hpp"
#include "ast/Expr.hpp"
#include "ast/Stmt.hpp"
#include "ast/AstPrinter.hpp"
#include "interpreter/Interpreter.hpp"
#include "resolver/Resolver.hpp"
#include <cstdlib>      
#include <sys/stat.h> 

#include "inferer/TypeInferer.hpp"
#include "backend/banner/BannerGenerator.hpp"
#include "backend/vm/VM.hpp"
#include "backend/CodeGenerator.hpp"

using namespace hulk;

// ============================================================
// Variables globales de error
// ============================================================
bool hadError = false;
bool hadLexicalError = false;
bool hadSyntacticError = false;
bool hadSemanticError = false;

int lastErrorLine = 0;
int lastErrorCol = 0;
std::string lastErrorMessage = "";
std::string lastErrorType = "";

// ============================================================
// Reporte de errores con formato (line,col) TYPE: message
// ============================================================

void reportError(int line, int col, const std::string& type, const std::string& message) {
    std::cout << "(" << line << "," << col << ") " << type << ": " << message << std::endl;
    hadError = true;
    lastErrorLine = line;
    lastErrorCol = col;
    lastErrorMessage = message;
    lastErrorType = type;
    
    if (type == "LEXICAL") hadLexicalError = true;
    if (type == "SYNTACTIC") hadSyntacticError = true;
    if (type == "SEMANTIC") hadSemanticError = true;
}

void lexicalError(int line, int col, const std::string& message) {
    reportError(line, col, "LEXICAL", message);
}

void syntacticError(int line, int col, const std::string& message) {
    reportError(line, col, "SYNTACTIC", message);
}

void semanticError(int line, int col, const std::string& message) {
    reportError(line, col, "SEMANTIC", message);
}

// ============================================================
// Funciones legacy (para compatibilidad con código existente)
// ============================================================

void report(int line, const std::string& where, const std::string& message) {
    int col = 0;
    std::cout << "(" << line << "," << col << ") SYNTACTIC: " << message << std::endl;
    hadError = true;
    hadSyntacticError = true;
}

void error(int line, const std::string& message) {
    report(line, "", message);
}

void error(const Token& token, const std::string& message) {
    int line = token.line;
    int col = 0;  // Por ahora, columna no está disponible en Token
    if (token.type == TokenType::TOKEN_EOF) {
        syntacticError(line, col, message + " at end");
    } else {
        syntacticError(line, col, message + " at '" + token.lexeme + "'");
    }
}

// ============================================================
// Determinar código de salida según errores
// ============================================================

int getExitCode() {
    if (hadLexicalError) return 1;
    if (hadSyntacticError) return 2;
    if (hadSemanticError) return 3;
    return 0;
}


// ============================================================
// Generar archivo ./output
// ============================================================

void generateOutput(const std::vector<std::unique_ptr<Stmt>>& statements) {
    hulk::backend::CodeGenerator generator;

    // Análisis semántico. Si falla, los errores ya han sido reportados y el
    // código de salida se determina con getExitCode() (3 = semántico).
    if (!generator.checkSemantics(statements)) {
        return;
    }

    // Backend: transpilar el AST a C++ y compilar a ./output.
    if (!generator.generate(statements, "./output")) {
        std::cerr << "Failed to generate output" << std::endl;
        exit(70);
    }
}
// ============================================================
// Función principal de ejecución
// ============================================================

void run(const std::string& source) {
    Scanner scanner(source);
    auto tokens = scanner.scanTokens();
    
    // Verificar errores léxicos (ya deberían estar reportados por Scanner)
    if (hadError) {
        return;
    }
    
    Parser parser(tokens);
    auto statements = parser.parseRepl();
    
    if (hadError) {
        return;
    }
    
    // Resolver + inferencia de tipos + generar programa BANNER + ejecutar en la VM
    Interpreter interpreter;
    Resolver resolver(interpreter);
    resolver.resolve(statements);

    // TypeInferer se instancia inmediatamente después de Resolver
    TypeInferer inferer(resolver);
    inferer.infer(statements);

    hulk::backend::BannerGenerator bannerGenerator(resolver, inferer);
    hulk::backend::BannerProgram bannerProgram = bannerGenerator.generate(statements);

    hulk::backend::VM vm;
    auto result = vm.interpret(bannerProgram);
    if (result != hulk::backend::InterpretResult::INTERPRET_OK) {
        semanticError(0, 0, "Backend VM execution failed");
    }
    
    hadError = false;
}

// ============================================================
// Ejecución de archivo
// ============================================================

void runFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << path << "'" << std::endl;
        exit(74);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    // --- Fase léxica ---
    Scanner scanner(source);
    auto tokens = scanner.scanTokens();
    if (hadError) {
        exit(getExitCode());   // 1 = léxico
    }

    // --- Fase sintáctica ---
    Parser parser(tokens);
    auto statements = parser.parse();
    if (hadError) {
        exit(getExitCode());   // 2 = sintáctico
    }

    // --- Fase semántica + backend (genera ./output) ---
    generateOutput(statements);
    if (hadError) {
        exit(getExitCode());   // 3 = semántico
    }

    // Ejecutar el binario generado para producir la salida esperada por el
    // cliente (por ejemplo, el harness de tests usa la salida de `hulk`).
    // Ignoramos el código de retorno de la ejecución del programa generado
    // y devolvemos 0 salvo errores previos.
    std::system("./output");
    exit(0);
}

// ============================================================
// REPL (no se evalúa en la entrega, solo para desarrollo)
// ============================================================

void runRepl() {
    std::cout << "HULK Compiler v0.1.0 (REPL mode)" << std::endl;
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
            std::cout << "Commands:" << std::endl;
            std::cout << "  exit    - Exit REPL" << std::endl;
            std::cout << "  help    - Show this help" << std::endl;
            continue;
        }
        
        run(line);
        hadError = false;
        hadLexicalError = false;
        hadSyntacticError = false;
        hadSemanticError = false;
    }
}

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[]) {
    // Opciones de ayuda y versión
    if (argc == 2 && std::string(argv[1]) == "--version") {
        std::cout << "HULK Compiler v0.1.0" << std::endl;
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "Usage: hulk <file.hulk>" << std::endl;
        std::cout << "       hulk --help" << std::endl;
        std::cout << "       hulk --version" << std::endl;
        return 0;
    }
    
    // Modo REPL (solo para desarrollo)
    if (argc == 1) {
        runRepl();
        return 0;
    }
    
    // Modo archivo (compilar a ./output)
    if (argc == 2) {
        runFile(argv[1]);
    } else {
        std::cerr << "Usage: hulk <file.hulk>" << std::endl;
        return 64;
    }
    
    return 0;
}
