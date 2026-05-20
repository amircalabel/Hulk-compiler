// src/main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Forward declarations de los módulos (los definiremos después)
#include "scanner/Scanner.h"
#include "parser/Parser.h"

// Variables globales para errores (simplificado, luego mejoraremos)
bool hadError = false;
bool hadRuntimeError = false;

// Reporte de errores (igual que en el libro)
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
}

// Función principal de ejecución (scanner + parser)
void run(const std::string& source) {
    Scanner scanner(source);
    std::vector<Token> tokens = scanner.scanTokens();

    Parser parser(tokens);
    std::unique_ptr<Expr> expression = parser.parse();

    // Si hubo error, no continuamos
    if (hadError) return;

    // Por ahora solo imprimimos el AST (pretty printer)
    // En el backend, aquí iría la generación de código
    std::cout << "Parsing successful!" << std::endl;
}

// Ejecutar un archivo
void runFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << path << std::endl;
        exit(74);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    run(buffer.str());

    if (hadError) exit(65);
    if (hadRuntimeError) exit(70);
}

// REPL (Read-Eval-Print Loop)
void runPrompt() {
    std::string line;
    for (;;) {
        std::cout << "> ";
        std::getline(std::cin, line);
        if (line.empty()) break;
        run(line);
        hadError = false;  // Resetear error para la siguiente línea
    }
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        runPrompt();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        std::cerr << "Usage: hulk [script]" << std::endl;
        exit(64);
    }
    return 0;
}