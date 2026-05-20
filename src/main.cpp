// src/main.cpp
// src/main.cpp (actualizado)
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "scanner/Scanner.hpp"
#include "parser/Parser.hpp"
#include "ast/AstPrinter.hpp"
#include "resolver/Resolver.hpp"
#include "inferer/TypeInferer.hpp"

// Variables globales para errores
bool hadError = false;
bool hadResolverError = false;
bool hadTypeError = false;
bool hadRuntimeError = false;

// Reporte de errores (igual que antes)
void report(int line, const std::string& where, const std::string& message) {
    std::cerr << "[line " << line << "] Error" << where << ": " << message << std::endl;
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

// Función principal de ejecución con Resolver e Inferer
void run(const std::string& source) {
    // 1. Escanear
    Scanner scanner(source);
    std::vector<Token> tokens = scanner.scanTokens();
    
    // 2. Parsear
    Parser parser(tokens);
    auto statements = parser.parse();
    
    if (hadError) return;
    
    // 3. Resolver (análisis de ámbito)
    // NOTA: Interpreter se implementará después en el backend
    // Por ahora, creamos un placeholder
    // Interpreter interpreter;
    // Resolver resolver(interpreter);
    // resolver.resolve(statements);
    
    // if (hadResolverError) return;
    
    // 4. Type Inferer (inferencia de tipos)
    // TypeInferer inferer(resolver);
    // inferer.infer(statements);
    
    // if (hadTypeError) return;
    
    // 5. Imprimir el AST (debug)
    AstPrinter printer;
    std::cout << "\n=== AST ===\n" << std::endl;
    std::cout << printer.print(statements) << std::endl;
    
    // 6. Backend (pendiente)
    // ...
}

// runFile y runPrompt igual que antes...