// src/parser/Parser.hpp
#ifndef HULK_PARSER_HPP
#define HULK_PARSER_HPP

#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>
#include "scanner/Token.hpp"
#include "ast/Expr.hpp"
#include "ast/Stmt.hpp"

// Clase de error del parser
class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& message)
        : std::runtime_error(message) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    
    // Punto de entrada principal
    std::vector<std::unique_ptr<Stmt>> parse();

private:
    const std::vector<Token>& tokens;
    int current = 0;
    
    // Variables globales de error (simulando las del main)
    bool hadError = false;
    bool panicMode = false;

    // ============================================================
    // Helpers básicos
    // ============================================================
    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(const std::vector<TokenType>& types);
    Token consume(TokenType type, const std::string& message);
    void synchronize();
    
    // Reporte de errores
    void error(const Token& token, const std::string& message);
    void errorAtCurrent(const std::string& message);
    
    // ============================================================
    // Parsing de declarations (top-level)
    // ============================================================
    std::unique_ptr<Stmt> declaration();
    std::unique_ptr<Stmt> statement();
    
    // Statements específicos
    std::unique_ptr<Stmt> expressionStatement();
    std::unique_ptr<Stmt> printStatement();
    std::unique_ptr<Stmt> returnStatement();
    std::unique_ptr<Stmt> blockStatement();
    
    // Declaraciones específicas
    std::unique_ptr<Stmt> varDeclaration();
    std::unique_ptr<Stmt> functionDeclaration(const std::string& kind);
    std::unique_ptr<Stmt> classDeclaration();
    std::unique_ptr<Stmt> protocolDeclaration();
    std::unique_ptr<Stmt> macroDeclaration();
    
    // ============================================================
    // Parsing de expresiones (Pratt parser)
    // ============================================================
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> assignment();
    std::unique_ptr<Expr> logicalOr();
    std::unique_ptr<Expr> logicalAnd();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> concat();      // @ y @@ (HULK específico)
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> call();
    std::unique_ptr<Expr> primary();
    
    // ============================================================
    // Expresiones específicas de HULK
    // ============================================================
    std::unique_ptr<Expr> letExpression();
    std::unique_ptr<Expr> ifExpression();
    std::unique_ptr<Expr> whileExpression();
    std::unique_ptr<Expr> forExpression();
    std::unique_ptr<Expr> blockExpression();
    
    // ============================================================
    // Helpers para parsing
    // ============================================================
    TokenType annotationType();  // para : Number
    std::unique_ptr<Expr> parseLetBinding();
    std::vector<std::unique_ptr<Expr>> parseArguments();
    std::vector<Token> parseParameters();
    std::unique_ptr<Expr> parseParenthesizedExpression();
};

#endif // HULK_PARSER_HPP