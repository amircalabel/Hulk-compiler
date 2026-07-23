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

namespace hulk {

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& message)
        : std::runtime_error(message) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    
    std::vector<std::unique_ptr<Stmt>> parse();
    std::vector<std::unique_ptr<Stmt>> parseRepl();

private:
    const std::vector<Token>& tokens;
    int current = 0;
    bool hadError = false;
    bool panicMode = false;

    // Helpers básicos
    bool isAtEnd() const;
    Token peek() const;
    Token peekNext() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(const std::vector<TokenType>& types);
    Token consume(TokenType type, const std::string& message);
    void synchronize();
    
    void error(const Token& token, const std::string& message);
    void errorAtCurrent(const std::string& message);
    
    // Parsing de declarations
    std::unique_ptr<Stmt> declaration();
    std::unique_ptr<Stmt> statement();
    
    // Statements
    std::unique_ptr<Stmt> expressionStatement();
    std::unique_ptr<Stmt> printStatement();
    std::unique_ptr<Stmt> returnStatement();
    std::unique_ptr<Stmt> blockStatement();
    std::unique_ptr<Stmt> ifStatement();
    void skipSemicolonBeforeElse();
    std::unique_ptr<Stmt> whileStatement();
    std::unique_ptr<Stmt> forStatement();

    // Declaraciones
    bool letHasIn() const;
    std::unique_ptr<Stmt> letBinding();
    std::unique_ptr<Stmt> varDeclaration();
    std::unique_ptr<Stmt> functionDeclaration(const std::string& kind);
    std::unique_ptr<Stmt> classDeclaration();
    std::unique_ptr<Stmt> protocolDeclaration();
    std::unique_ptr<Stmt> macroDeclaration();
    
    // Expresiones (Pratt parser)
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> assignment();
    std::unique_ptr<Expr> logicalOr();
    std::unique_ptr<Expr> logicalAnd();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> power();
    std::unique_ptr<Expr> concat();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> call();
    std::unique_ptr<Expr> finishCall(std::unique_ptr<Expr> callee);
    std::unique_ptr<Expr> primary();
    std::unique_ptr<Expr> newExpression();
    std::unique_ptr<Expr> lambdaExpression();
    std::unique_ptr<Expr> arrayLiteralExpression();
    
    // Expresiones específicas de HULK
    std::unique_ptr<Expr> letExpression();
    std::unique_ptr<Expr> ifExpression();
    std::unique_ptr<Expr> whileExpression();
    std::unique_ptr<Expr> forExpression();
    std::unique_ptr<Expr> blockExpression();
    
    // Helpers
    std::vector<std::unique_ptr<Expr>> parseArguments();
    std::unique_ptr<Expr> parseParenthesizedExpression();
};

} // namespace hulk

#endif // HULK_PARSER_HPP
