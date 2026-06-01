// src/parser/Parser.cpp
#include "Parser.hpp"
#include <iostream>

namespace hulk {

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
// Parsing
// ============================================================

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    try {
        while (!isAtEnd()) {
            auto stmt = declaration();
            if (stmt) statements.push_back(std::move(stmt));
            if (panicMode) synchronize();
        }
    } catch (const ParseError&) {}
    return statements;
}

std::vector<std::unique_ptr<Stmt>> Parser::parseRepl() {
    return parse();
}

std::unique_ptr<Stmt> Parser::declaration() {
    if (match(TokenType::TOKEN_FUNCTION)) return functionDeclaration("function");
    if (match(TokenType::TOKEN_TYPE)) return classDeclaration();
    if (match(TokenType::TOKEN_VAR)) return varDeclaration();
    return statement();
}

std::unique_ptr<Stmt> Parser::statement() {
    if (match(TokenType::TOKEN_PRINT)) return printStatement();
    if (match(TokenType::TOKEN_RETURN)) return returnStatement();
    if (match(TokenType::TOKEN_LEFT_BRACE)) return blockStatement();
    return expressionStatement();
}

std::unique_ptr<Stmt> Parser::expressionStatement() {
    auto expr = expression();
    consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after expression.");
    return std::make_unique<ExpressionStmt>(std::move(expr));
}

std::unique_ptr<Stmt> Parser::printStatement() {
    auto expr = expression();
    consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after value.");
    return std::make_unique<PrintStmt>(std::move(expr));
}

std::unique_ptr<Stmt> Parser::returnStatement() {
    Token keyword = previous();
    std::unique_ptr<Expr> value = nullptr;
    if (!check(TokenType::TOKEN_SEMICOLON)) {
        value = expression();
    }
    consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after return value.");
    return std::make_unique<ReturnStmt>(keyword, std::move(value));
}

std::unique_ptr<Stmt> Parser::blockStatement() {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!check(TokenType::TOKEN_RIGHT_BRACE) && !isAtEnd()) {
        auto stmt = declaration();
        if (stmt) statements.push_back(std::move(stmt));
        if (panicMode) synchronize();
    }
    consume(TokenType::TOKEN_RIGHT_BRACE, "Expect '}' after block.");
    return std::make_unique<BlockStmt>(std::move(statements));
}

std::unique_ptr<Stmt> Parser::varDeclaration() {
    Token name = consume(TokenType::TOKEN_IDENTIFIER, "Expect variable name.");
    Token typeAnnotation; typeAnnotation.type = TokenType::TOKEN_ERROR;
    if (match(TokenType::TOKEN_COLON)) {
        typeAnnotation = consume(TokenType::TOKEN_IDENTIFIER, "Expect type name.");
    }
    std::unique_ptr<Expr> initializer = nullptr;
    if (match(TokenType::TOKEN_EQUAL)) {
        initializer = expression();
    }
    consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_unique<VarDeclStmt>(name, typeAnnotation, std::move(initializer));
}

std::unique_ptr<Stmt> Parser::functionDeclaration(const std::string& kind) {
    Token name = consume(TokenType::TOKEN_IDENTIFIER, "Expect " + kind + " name.");
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after " + kind + " name.");
    std::vector<FunctionDeclStmt::Parameter> parameters;
    if (!check(TokenType::TOKEN_RIGHT_PAREN)) {
        do {
            Token paramName = consume(TokenType::TOKEN_IDENTIFIER, "Expect parameter name.");
            Token paramType; paramType.type = TokenType::TOKEN_ERROR;
            if (match(TokenType::TOKEN_COLON)) {
                paramType = consume(TokenType::TOKEN_IDENTIFIER, "Expect type name.");
            }
            parameters.push_back({paramName, paramType});
        } while (match(TokenType::TOKEN_COMMA));
    }
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    Token returnType; returnType.type = TokenType::TOKEN_ERROR;
    if (match(TokenType::TOKEN_COLON)) {
        returnType = consume(TokenType::TOKEN_IDENTIFIER, "Expect return type name.");
    }
    std::vector<std::unique_ptr<Stmt>> body;
    if (match(TokenType::TOKEN_ARROW)) {
        auto expr = expression();
        consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after inline function body.");
        std::vector<std::unique_ptr<Stmt>> returnStmts;
        returnStmts.push_back(std::make_unique<ReturnStmt>(
            Token{TokenType::TOKEN_RETURN, "return", std::monostate{}, name.line},
            std::move(expr)));
        body = std::move(returnStmts);
    } else {
        consume(TokenType::TOKEN_LEFT_BRACE, "Expect '{' before function body.");
        while (!check(TokenType::TOKEN_RIGHT_BRACE) && !isAtEnd()) {
            auto stmt = declaration();
            if (stmt) body.push_back(std::move(stmt));
        }
        consume(TokenType::TOKEN_RIGHT_BRACE, "Expect '}' after function body.");
    }
    return std::make_unique<FunctionDeclStmt>(name, parameters, returnType, std::move(body));
}

std::unique_ptr<Stmt> Parser::classDeclaration() {
    Token name = consume(TokenType::TOKEN_IDENTIFIER, "Expect class name.");
    std::vector<Token> typeArguments;
    if (match(TokenType::TOKEN_LEFT_PAREN)) {
        if (!check(TokenType::TOKEN_RIGHT_PAREN)) {
            do {
                typeArguments.push_back(consume(TokenType::TOKEN_IDENTIFIER, "Expect parameter name."));
            } while (match(TokenType::TOKEN_COMMA));
        }
        consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after type parameters.");
    }
    Token superclass; superclass.type = TokenType::TOKEN_ERROR;
    std::vector<std::unique_ptr<Expr>> superclassArguments;
    if (match(TokenType::TOKEN_INHERITS)) {
        superclass = consume(TokenType::TOKEN_IDENTIFIER, "Expect superclass name.");
        if (match(TokenType::TOKEN_LEFT_PAREN)) {
            while (!check(TokenType::TOKEN_RIGHT_PAREN) && !isAtEnd()) {
                auto expr = expression();
                if (expr) superclassArguments.push_back(std::move(expr));
            }
            consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after superclass arguments.");
        }
    }
    consume(TokenType::TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    std::vector<std::pair<Token, Token>> attributes;
    std::vector<std::unique_ptr<FunctionDeclStmt>> methods;
    while (!check(TokenType::TOKEN_RIGHT_BRACE) && !isAtEnd()) {
        Token attrName = consume(TokenType::TOKEN_IDENTIFIER, "Expect attribute or method name.");
        if (match(TokenType::TOKEN_EQUAL)) {
            Token attrType; attrType.type = TokenType::TOKEN_ERROR;
            if (match(TokenType::TOKEN_COLON)) {
                attrType = consume(TokenType::TOKEN_IDENTIFIER, "Expect type name.");
            }
            expression();
            consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after attribute initializer.");
            attributes.push_back({attrName, attrType});
        } else {
            auto method = functionDeclaration("method");
            if (dynamic_cast<FunctionDeclStmt*>(method.get())) {
                methods.push_back(std::unique_ptr<FunctionDeclStmt>(
                    static_cast<FunctionDeclStmt*>(method.release())));
            }
        }
    }
    consume(TokenType::TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
    return std::unique_ptr<ClassDeclStmt>(new ClassDeclStmt(
        name, typeArguments, attributes, std::move(methods), superclass, std::move(superclassArguments)));
}

std::unique_ptr<Stmt> Parser::protocolDeclaration() {
    Token name = consume(TokenType::TOKEN_IDENTIFIER, "Expect protocol name.");
    Token extends; extends.type = TokenType::TOKEN_ERROR;
    if (match(TokenType::TOKEN_INHERITS)) {
        extends = consume(TokenType::TOKEN_IDENTIFIER, "Expect protocol name to extend.");
    }
    consume(TokenType::TOKEN_LEFT_BRACE, "Expect '{' before protocol body.");
    std::vector<ProtocolDeclStmt::MethodSignature> methods;
    while (!check(TokenType::TOKEN_RIGHT_BRACE) && !isAtEnd()) {
        Token methodName = consume(TokenType::TOKEN_IDENTIFIER, "Expect method name.");
        consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after method name.");
        std::vector<std::pair<Token, Token>> parameters;
        if (!check(TokenType::TOKEN_RIGHT_PAREN)) {
            do {
                Token paramName = consume(TokenType::TOKEN_IDENTIFIER, "Expect parameter name.");
                Token paramType; paramType.type = TokenType::TOKEN_ERROR;
                if (match(TokenType::TOKEN_COLON)) {
                    paramType = consume(TokenType::TOKEN_IDENTIFIER, "Expect type name.");
                }
                parameters.push_back({paramName, paramType});
            } while (match(TokenType::TOKEN_COMMA));
        }
        consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
        Token returnType; returnType.type = TokenType::TOKEN_ERROR;
        if (match(TokenType::TOKEN_COLON)) {
            returnType = consume(TokenType::TOKEN_IDENTIFIER, "Expect return type name.");
        }
        consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after method signature.");
        methods.push_back({methodName, parameters, returnType});
    }
    consume(TokenType::TOKEN_RIGHT_BRACE, "Expect '}' after protocol body.");
    return std::make_unique<ProtocolDeclStmt>(name, methods, extends);
}

std::unique_ptr<Stmt> Parser::macroDeclaration() {
    Token name = consume(TokenType::TOKEN_IDENTIFIER, "Expect macro name.");
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after macro name.");
    std::vector<MacroDeclStmt::Parameter> parameters;
    if (!check(TokenType::TOKEN_RIGHT_PAREN)) {
        do {
            Token paramName;
            bool isSymbolic = false;
            bool isPlaceholder = false;
            if (check(TokenType::TOKEN_AT)) {
                advance();
                paramName = consume(TokenType::TOKEN_IDENTIFIER, "Expect parameter name after '@'.");
                isSymbolic = true;
            } else if (check(TokenType::TOKEN_DOLLAR)) {
                advance();
                paramName = consume(TokenType::TOKEN_IDENTIFIER, "Expect parameter name after '$'.");
                isPlaceholder = true;
            } else {
                paramName = consume(TokenType::TOKEN_IDENTIFIER, "Expect parameter name.");
            }
            parameters.push_back({paramName, isSymbolic, isPlaceholder});
        } while (match(TokenType::TOKEN_COMMA));
    }
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    bool hasPatternMatching = false;
    if (match(TokenType::TOKEN_ARROW)) {
        auto body = expression();
        consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after macro body.");
        return std::make_unique<MacroDeclStmt>(name, parameters, std::move(body), hasPatternMatching);
    } else if (match(TokenType::TOKEN_LEFT_BRACE)) {
        auto body = expression();
        consume(TokenType::TOKEN_RIGHT_BRACE, "Expect '}' after macro body.");
        return std::make_unique<MacroDeclStmt>(name, parameters, std::move(body), hasPatternMatching);
    } else {
        errorAtCurrent("Expect '=>' or '{' after macro parameters.");
        throw ParseError("Invalid macro declaration");
    }
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
        } else if (match(TokenType::TOKEN_DOT)) {
            consume(TokenType::TOKEN_IDENTIFIER, "Expect property name after '.'.");
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::primary() {
    if (match(TokenType::TOKEN_TRUE)) return std::make_unique<LiteralExpr>(true);
    if (match(TokenType::TOKEN_FALSE)) return std::make_unique<LiteralExpr>(false);
    if (match(TokenType::TOKEN_NIL)) return std::make_unique<LiteralExpr>(nullptr);
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
        Token typeAnnotation; typeAnnotation.type = TokenType::TOKEN_ERROR;
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
    while (match(TokenType::TOKEN_ELIF)) {
        consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'elif'.");
        auto elifCond = expression();
        consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after elif condition.");
        auto elifBody = expression();
        elifBranches.push_back({std::move(elifCond), std::move(elifBody)});
    }
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

// Implementaciones vacías
std::unique_ptr<Expr> Parser::whileExpression() { return nullptr; }
std::unique_ptr<Expr> Parser::forExpression() { return nullptr; }
std::unique_ptr<Expr> Parser::blockExpression() { return nullptr; }

} // namespace hulk