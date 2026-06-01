// src/parser/Parser.cpp
#include "Parser.hpp"
#include <iostream>

namespace hulk {

// ============================================================
// Constructor y helpers básicos
// ============================================================

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::TOKEN_EOF;
}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    error(peek(), message);
    throw ParseError(message);
}

// ============================================================
// Manejo de errores
// ============================================================

void Parser::error(const Token& token, const std::string& message) {
    if (panicMode) return;
    panicMode = true;
    hadError = true;
    
    std::cerr << "[line " << token.line << "] Error";
    if (token.type == TokenType::TOKEN_EOF) {
        std::cerr << " at end";
    } else {
        std::cerr << " at '" << token.lexeme << "'";
    }
    std::cerr << ": " << message << std::endl;
}

void Parser::errorAtCurrent(const std::string& message) {
    error(peek(), message);
}

void Parser::synchronize() {
    panicMode = false;
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::TOKEN_SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::TOKEN_LET:
            case TokenType::TOKEN_FUNCTION:
            case TokenType::TOKEN_TYPE:
            case TokenType::TOKEN_PROTOCOL:
            case TokenType::TOKEN_DEF:
            case TokenType::TOKEN_IF:
            case TokenType::TOKEN_WHILE:
            case TokenType::TOKEN_FOR:
            case TokenType::TOKEN_RETURN:
            case TokenType::TOKEN_PRINT:
                return;
            default:
                break;
        }
        advance();
    }
}

// ============================================================
// Parsing de declarations (top-level)
// ============================================================

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    
    try {
        while (!isAtEnd()) {
            // Solo parsear print statements por ahora
            if (match(TokenType::TOKEN_PRINT)) {
                auto expr = expression();
                consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after value.");
                statements.push_back(std::make_unique<PrintStmt>(std::move(expr)));
            }
            // Parsear expresión sola (será tratada como expression statement)
            else {
                auto expr = expression();
                if (expr) {
                    statements.push_back(std::make_unique<ExpressionStmt>(std::move(expr)));
                }
                // Consumir punto y coma si existe
                if (match(TokenType::TOKEN_SEMICOLON)) {
                    // OK
                }
            }
            
            if (panicMode) {
                synchronize();
                panicMode = false;
            }
        }
    } catch (const ParseError& e) {
        // Error ya reportado
    }
    
    return statements;
}

std::vector<std::unique_ptr<Stmt>> Parser::parseRepl() {
    return parse();
}

// ============================================================
// Parsing de expresiones (Pratt parser)
// ============================================================

std::unique_ptr<Expr> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expr> Parser::assignment() {
    auto expr = logicalOr();
    
    if (match(TokenType::TOKEN_COLON_EQUAL)) {
        Token equals = previous();
        auto value = assignment();
        
        if (auto* varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
            return std::make_unique<AssignExpr>(varExpr->name, std::move(value));
        }
        
        error(equals, "Invalid assignment target.");
        return value;
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();
    
    while (match(TokenType::TOKEN_OR)) {
        Token op = previous();
        auto right = logicalAnd();
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::logicalAnd() {
    auto expr = equality();
    
    while (match(TokenType::TOKEN_AND)) {
        Token op = previous();
        auto right = equality();
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    auto expr = comparison();
    
    while (match({TokenType::TOKEN_EQUAL_EQUAL, TokenType::TOKEN_BANG_EQUAL})) {
        Token op = previous();
        auto right = comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    auto expr = term();
    
    while (match({TokenType::TOKEN_LESS, TokenType::TOKEN_LESS_EQUAL,
                  TokenType::TOKEN_GREATER, TokenType::TOKEN_GREATER_EQUAL})) {
        Token op = previous();
        auto right = term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    auto expr = factor();
    
    while (match({TokenType::TOKEN_PLUS, TokenType::TOKEN_MINUS})) {
        Token op = previous();
        auto right = factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    auto expr = concat();
    
    while (match({TokenType::TOKEN_STAR, TokenType::TOKEN_SLASH, TokenType::TOKEN_CARET})) {
        Token op = previous();
        auto right = concat();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::concat() {
    auto expr = unary();
    
    while (match({TokenType::TOKEN_AT, TokenType::TOKEN_AT_AT})) {
        Token op = previous();
        auto right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match({TokenType::TOKEN_BANG, TokenType::TOKEN_MINUS})) {
        Token op = previous();
        auto right = unary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }
    
    return call();
}

std::unique_ptr<Expr> Parser::call() {
    auto expr = primary();
    
    while (true) {
        if (match(TokenType::TOKEN_LEFT_PAREN)) {
            auto args = parseArguments();
            Token paren = previous();
            expr = std::make_unique<CallExpr>(std::move(expr), paren, std::move(args));
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::primary() {
    if (match(TokenType::TOKEN_TRUE)) {
        return std::make_unique<LiteralExpr>(true);
    }
    if (match(TokenType::TOKEN_FALSE)) {
        return std::make_unique<LiteralExpr>(false);
    }
    if (match(TokenType::TOKEN_NIL)) {
        return std::make_unique<LiteralExpr>(nullptr);
    }
    if (match(TokenType::TOKEN_NUMBER)) {
        double value = std::get<double>(previous().literal);
        return std::make_unique<LiteralExpr>(value);
    }
    if (match(TokenType::TOKEN_STRING)) {
        std::string value = std::get<std::string>(previous().literal);
        return std::make_unique<LiteralExpr>(value);
    }
    if (match(TokenType::TOKEN_IDENTIFIER)) {
        return std::make_unique<VariableExpr>(previous());
    }
    if (match(TokenType::TOKEN_LET)) {
        return letExpression();
    }
    if (match(TokenType::TOKEN_IF)) {
        return ifExpression();
    }
    if (match(TokenType::TOKEN_LEFT_PAREN)) {
        return parseParenthesizedExpression();
    }
    
    errorAtCurrent("Expect expression.");
    throw ParseError("Invalid expression");
}

// ============================================================
// Expresiones específicas de HULK
// ============================================================

std::unique_ptr<Expr> Parser::letExpression() {
    std::vector<LetExpr::Binding> bindings;
    
    do {
        Token name = consume(TokenType::TOKEN_IDENTIFIER, "Expect variable name.");
        
        Token typeAnnotation;
        typeAnnotation.type = TokenType::TOKEN_ERROR;
        if (match(TokenType::TOKEN_COLON)) {
            typeAnnotation = consume(TokenType::TOKEN_IDENTIFIER, "Expect type name.");
        }
        
        consume(TokenType::TOKEN_EQUAL, "Expect '=' after variable name.");
        auto initializer = expression();
        
        bindings.push_back({name, typeAnnotation, std::move(initializer)});
    } while (match(TokenType::TOKEN_COMMA));
    
    consume(TokenType::TOKEN_IN, "Expect 'in' after let bindings.");
    auto body = expression();
    
    return std::make_unique<LetExpr>(std::move(bindings), std::move(body));
}

std::unique_ptr<Expr> Parser::ifExpression() {
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    auto condition = expression();
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    
    auto thenBranch = expression();
    
    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> elifBranches;
    std::unique_ptr<Expr> elseBranch = nullptr;
    
    if (match(TokenType::TOKEN_ELSE)) {
        elseBranch = expression();
    }
    
    return std::make_unique<IfExpr>(std::move(condition), std::move(thenBranch),
                                     std::move(elifBranches), std::move(elseBranch));
}

std::unique_ptr<Expr> Parser::parseParenthesizedExpression() {
    auto expr = expression();
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
    return std::make_unique<GroupingExpr>(std::move(expr));
}

std::vector<std::unique_ptr<Expr>> Parser::parseArguments() {
    std::vector<std::unique_ptr<Expr>> arguments;
    
    if (!check(TokenType::TOKEN_RIGHT_PAREN)) {
        do {
            arguments.push_back(expression());
        } while (match(TokenType::TOKEN_COMMA));
    }
    
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return arguments;
}

// ============================================================
// Implementaciones mínimas para evitar errores de linker
// ============================================================

std::unique_ptr<Stmt> Parser::declaration() { return statement(); }
std::unique_ptr<Stmt> Parser::statement() { return expressionStatement(); }
std::unique_ptr<Stmt> Parser::expressionStatement() { return nullptr; }
std::unique_ptr<Stmt> Parser::printStatement() { return nullptr; }
std::unique_ptr<Stmt> Parser::returnStatement() { return nullptr; }
std::unique_ptr<Stmt> Parser::blockStatement() { return nullptr; }
std::unique_ptr<Stmt> Parser::ifStatement() { return nullptr; }
std::unique_ptr<Stmt> Parser::whileStatement() { return nullptr; }
std::unique_ptr<Stmt> Parser::forStatement() { return nullptr; }
std::unique_ptr<Stmt> Parser::varDeclaration() { return nullptr; }
std::unique_ptr<Stmt> Parser::functionDeclaration(const std::string&) { return nullptr; }
std::unique_ptr<Stmt> Parser::classDeclaration() { return nullptr; }
std::unique_ptr<Stmt> Parser::protocolDeclaration() { return nullptr; }
std::unique_ptr<Stmt> Parser::macroDeclaration() { return nullptr; }
std::unique_ptr<Expr> Parser::whileExpression() { return nullptr; }
std::unique_ptr<Expr> Parser::forExpression() { return nullptr; }
std::unique_ptr<Expr> Parser::blockExpression() { return nullptr; }

} // namespace hulk