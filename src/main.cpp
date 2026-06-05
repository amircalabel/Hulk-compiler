// src/main.cpp - Versión simplificada para pruebas del parser
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>

// Frontend (solo lo necesario para pruebas)
#include "scanner/Scanner.hpp"
#include "scanner/Token.hpp"
#include "parser/Parser.hpp"
#include "ast/Expr.hpp"
#include "ast/Stmt.hpp"
#include "ast/AstPrinter.hpp"
#include "interpreter/Interpreter.hpp"

using namespace hulk;

// ============================================================
// Variables globales de error
// ============================================================
bool hadError = false;

// ============================================================
// Reporte de errores
// ============================================================

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

// ============================================================
// Función principal de ejecución (solo parser)
// ============================================================

void run(const std::string& source) {
    /*
    //========================================================
    // Seccion de prueba para el AStPrinter y parser 
    // FASE 1: SCANNER
    Scanner scanner(source);
    std::vector<Token> tokens = scanner.scanTokens();

    std::cout << "=== TOKENS ===" << std::endl;
    for (const auto& token : tokens) {
        std::cout << "  " << token.toString() << std::endl;
    }
    std::cout << "==============" << std::endl;
    
    if (hadError) {
        hadError = false;
        return;
    }
    
    // FASE 2: PARSER
    Parser parser(tokens);
    auto statements = parser.parseRepl();
    
    if (hadError) {
        hadError = false;
        return;
    }
    
    // FASE 3: IMPRIMIR AST
    AstPrinter printer;
    std::cout << printer.print(statements) << std::endl;

    //==================================================== */
     Scanner scanner(source);
    auto tokens = scanner.scanTokens();
    if (hadError) { hadError = false; return; }
    
    Parser parser(tokens);
    auto statements = parser.parseRepl();
    if (hadError) { hadError = false; return; }
    
    // Interpreter
    Interpreter interpreter;
    try {
        interpreter.interpret(statements);
    } catch (const std::exception& e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
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
    run(buffer.str());
    
    if (hadError) exit(65);
}

// ============================================================
// REPL
// ============================================================

void runRepl() {
    std::cout << "HULK Parser v0.1.0" << std::endl;
    std::cout << "Type 'exit' to quit" << std::endl;
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
        
        run(line);
        hadError = false;
    }
}

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[]) {
    // En main(), antes de procesar argumentos, agrega:
    if (argc == 2 && std::string(argv[1]) == "--version") {
        std::cout << "HULK Compiler v0.1.0" << std::endl;
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "Usage: hulk [script]" << std::endl;
        return 0;
    }
    if (argc == 1) {
        runRepl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        std::cerr << "Usage: hulk [script]" << std::endl;
        exit(64);
    }
    return 0;
}