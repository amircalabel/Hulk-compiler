// src/parser/Parser.cpp
#include "Parser.hpp"
#include <iostream>

extern void syntacticError(int line, int col, const std::string& message);

namespace hulk {

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

// ============================================================
// Helpers básicos
// ============================================================

bool Parser::isAtEnd() const {
    return peek().type == TokenType::TOKEN_EOF;
}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::peekNext() const {
    if (current + 1 >= static_cast<int>(tokens.size())) return tokens.back();
    return tokens[current + 1];
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

    int col = 0;
    std::string where = token.type == TokenType::TOKEN_EOF ? "end" : token.lexeme;
    if (token.type == TokenType::TOKEN_EOF) {
        syntacticError(token.line, col, message + " at end");
    } else {
        syntacticError(token.line, col, message + " at '" + where + "'");
    }
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
// Punto de entrada
// ============================================================

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    try {
        while (!isAtEnd()) {
            if (match(TokenType::TOKEN_SEMICOLON)) continue; // saltar ';' vacíos
            auto stmt = declaration();
            if (stmt) statements.push_back(std::move(stmt));
            if (panicMode) synchronize();
        }
    } catch (const ParseError&) {}
    return statements;
}

std::vector<std::unique_ptr<Stmt>> Parser::parseRepl() {
    // Para el modo REPL reutilizamos el parseo normal.
    return parse();
}

// ============================================================
// Declaraciones y statements
// ============================================================

std::unique_ptr<Stmt> Parser::declaration() {
    try {
        if (match(TokenType::TOKEN_FUNCTION)) {
            auto d = functionDeclaration("function");
            match(TokenType::TOKEN_SEMICOLON); // ';' opcional tras la declaración
            return d;
        }
        if (match(TokenType::TOKEN_TYPE)) {
            auto d = classDeclaration();
            match(TokenType::TOKEN_SEMICOLON);
            return d;
        }
        if (match(TokenType::TOKEN_PROTOCOL)) {
            auto d = protocolDeclaration();
            match(TokenType::TOKEN_SEMICOLON);
            return d;
        }
        if (match(TokenType::TOKEN_DEF)) {
            auto d = macroDeclaration();
            match(TokenType::TOKEN_SEMICOLON);
            return d;
        }
        if (match(TokenType::TOKEN_VAR)) {
            return varDeclaration();
        }
        if (check(TokenType::TOKEN_LET) && !letHasIn()) {
            // 'let x = e;'  (sin 'in')  -> declaración global de variable
            return letBinding();
        }
        return statement();
    } catch (const ParseError&) {
        synchronize();
        return nullptr;
    }
}

// ¿El 'let' actual es una expresión (tiene 'in') o una ligadura global?
bool Parser::letHasIn() const {
    int depth = 0;
    for (int i = current + 1; i < static_cast<int>(tokens.size()); i++) {
        TokenType t = tokens[i].type;
        if (t == TokenType::TOKEN_LEFT_PAREN || t == TokenType::TOKEN_LEFT_BRACE) {
            depth++;
        } else if (t == TokenType::TOKEN_RIGHT_PAREN || t == TokenType::TOKEN_RIGHT_BRACE) {
            if (depth == 0) break;
            depth--;
        } else if (depth == 0 && t == TokenType::TOKEN_SEMICOLON) {
            break;
        } else if (depth == 0 && t == TokenType::TOKEN_IN) {
            return true;
        } else if (t == TokenType::TOKEN_EOF) {
            break;
        }
    }
    return false;
}

// 'let name (: type)? = expr (, ...)* ;'  como declaración(es) de variable global
std::unique_ptr<Stmt> Parser::letBinding() {
    consume(TokenType::TOKEN_LET, "Expect 'let'.");
    std::vector<std::unique_ptr<Stmt>> decls;
    do {
        Token name = consume(TokenType::TOKEN_IDENTIFIER, "Expect variable name.");
        Token typeAnnotation; typeAnnotation.type = TokenType::TOKEN_ERROR;
        if (match(TokenType::TOKEN_COLON)) {
            typeAnnotation = consume(TokenType::TOKEN_IDENTIFIER, "Expect type name.");
        }
        consume(TokenType::TOKEN_EQUAL, "Expect '=' after variable name.");
        auto initializer = expression();
        decls.push_back(std::make_unique<VarDeclStmt>(name, typeAnnotation, std::move(initializer)));
    } while (match(TokenType::TOKEN_COMMA));
    consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    if (decls.size() == 1) return std::move(decls[0]);
    return std::make_unique<BlockStmt>(std::move(decls));
}

std::unique_ptr<Stmt> Parser::statement() {
    if (match(TokenType::TOKEN_PRINT)) return printStatement();
    if (match(TokenType::TOKEN_RETURN)) return returnStatement();
    if (match(TokenType::TOKEN_IF)) return ifStatement();
    if (match(TokenType::TOKEN_WHILE)) return whileStatement();
    if (match(TokenType::TOKEN_FOR)) return forStatement();
    if (match(TokenType::TOKEN_LEFT_BRACE)) return blockStatement();
    return expressionStatement();
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

std::unique_ptr<Stmt> Parser::expressionStatement() {
    auto expr = expression();
    match(TokenType::TOKEN_SEMICOLON);
    return std::make_unique<ExpressionStmt>(std::move(expr));
}

std::unique_ptr<Stmt> Parser::blockStatement() {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!check(TokenType::TOKEN_RIGHT_BRACE) && !isAtEnd()) {
        if (match(TokenType::TOKEN_SEMICOLON)) continue;
        auto stmt = declaration();
        if (stmt) statements.push_back(std::move(stmt));
    }
    consume(TokenType::TOKEN_RIGHT_BRACE, "Expect '}' after block.");
    return std::make_unique<BlockStmt>(std::move(statements));
}

std::unique_ptr<Stmt> Parser::ifStatement() {
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    auto condition = expression();
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after if condition.");
    auto thenBranch = statement();
    skipSemicolonBeforeElse();

    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Stmt>>> elifs;
    while (match(TokenType::TOKEN_ELIF)) {
        consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'elif'.");
        auto elifCond = expression();
        consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after elif condition.");
        auto elifBody = statement();
        skipSemicolonBeforeElse();
        elifs.push_back({std::move(elifCond), std::move(elifBody)});
    }

    std::unique_ptr<Stmt> elseBranch = nullptr;
    if (match(TokenType::TOKEN_ELSE)) {
        elseBranch = statement();
    }

    for (auto it = elifs.rbegin(); it != elifs.rend(); ++it) {
        elseBranch = std::make_unique<IfStmt>(std::move(it->first), std::move(it->second), std::move(elseBranch));
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

// Consume un ';' opcional cuando precede a 'else'/'elif' (if con estilo statement).
void Parser::skipSemicolonBeforeElse() {
    if (check(TokenType::TOKEN_SEMICOLON) &&
        (peekNext().type == TokenType::TOKEN_ELSE || peekNext().type == TokenType::TOKEN_ELIF)) {
        advance();
    }
}

std::unique_ptr<Stmt> Parser::whileStatement() {
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    auto condition = expression();
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after while condition.");
    auto body = statement();
    match(TokenType::TOKEN_SEMICOLON);
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::forStatement() {
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    std::unique_ptr<Stmt> initializer = nullptr;
    if (match(TokenType::TOKEN_SEMICOLON)) {
        initializer = nullptr;
    } else {
        match(TokenType::TOKEN_VAR); // 'var' opcional
        Token name = consume(TokenType::TOKEN_IDENTIFIER, "Expect loop variable name.");
        Token typeAnnotation; typeAnnotation.type = TokenType::TOKEN_ERROR;
        if (match(TokenType::TOKEN_COLON)) {
            typeAnnotation = consume(TokenType::TOKEN_IDENTIFIER, "Expect type name.");
        }
        std::unique_ptr<Expr> initExpr = nullptr;
        if (match(TokenType::TOKEN_EQUAL)) {
            initExpr = expression();
        }
        initializer = std::make_unique<VarDeclStmt>(name, typeAnnotation, std::move(initExpr));
        consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after for initializer.");
    }

    std::unique_ptr<Expr> condition = nullptr;
    if (!check(TokenType::TOKEN_SEMICOLON)) {
        condition = expression();
    }
    consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after for condition.");

    std::unique_ptr<Expr> increment = nullptr;
    if (!check(TokenType::TOKEN_RIGHT_PAREN)) {
        increment = expression();
    }
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    auto body = statement();
    match(TokenType::TOKEN_SEMICOLON);
    return std::make_unique<ForStmt>(std::move(initializer), std::move(condition),
                                     std::move(increment), std::move(body));
}

std::unique_ptr<Stmt> Parser::varDeclaration() {
    Token name = consume(TokenType::TOKEN_IDENTIFIER, "Expect variable name.");
    Token typeAnnotation; typeAnnotation.type = TokenType::TOKEN_ERROR;
    if (match(TokenType::TOKEN_COLON)) {
        if (check(TokenType::TOKEN_LEFT_PAREN)) {
            typeAnnotation = Token{TokenType::TOKEN_IDENTIFIER, "function", std::monostate{}, name.line};
        } else {
            typeAnnotation = consume(TokenType::TOKEN_IDENTIFIER, "Expect type name.");
        }
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
        body.push_back(std::make_unique<ReturnStmt>(
            Token{TokenType::TOKEN_RETURN, "return", std::monostate{}, name.line},
            std::move(expr)));
    } else {
        consume(TokenType::TOKEN_LEFT_BRACE, "Expect '{' before function body.");
        while (!check(TokenType::TOKEN_RIGHT_BRACE) && !isAtEnd()) {
            if (match(TokenType::TOKEN_SEMICOLON)) continue;
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
                if (match(TokenType::TOKEN_COLON)) {
                    consume(TokenType::TOKEN_IDENTIFIER, "Expect type name.");
                }
            } while (match(TokenType::TOKEN_COMMA));
        }
        consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after type parameters.");
    }
    Token superclass; superclass.type = TokenType::TOKEN_ERROR;
    std::vector<std::unique_ptr<Expr>> superclassArguments;
    if (match(TokenType::TOKEN_INHERITS)) {
        superclass = consume(TokenType::TOKEN_IDENTIFIER, "Expect superclass name.");
        if (match(TokenType::TOKEN_LEFT_PAREN)) {
            if (!check(TokenType::TOKEN_RIGHT_PAREN)) {
                do {
                    superclassArguments.push_back(expression());
                } while (match(TokenType::TOKEN_COMMA));
            }
            consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after superclass arguments.");
        }
    }
    consume(TokenType::TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    std::vector<std::pair<Token, Token>> attributes;
    std::vector<std::unique_ptr<Expr>> attributeInitializers;
    std::vector<std::unique_ptr<FunctionDeclStmt>> methods;
    while (!check(TokenType::TOKEN_RIGHT_BRACE) && !isAtEnd()) {
        if (match(TokenType::TOKEN_SEMICOLON)) continue;
        // Un atributo es 'IDENT (:type)? = expr ;'. Un método lleva '(' tras el nombre.
        if (check(TokenType::TOKEN_IDENTIFIER) && peekNext().type != TokenType::TOKEN_LEFT_PAREN) {
            Token attrName = consume(TokenType::TOKEN_IDENTIFIER, "Expect attribute name.");
            Token attrType; attrType.type = TokenType::TOKEN_ERROR;
            if (match(TokenType::TOKEN_COLON)) {
                attrType = consume(TokenType::TOKEN_IDENTIFIER, "Expect type name.");
            }
            consume(TokenType::TOKEN_EQUAL, "Expect '=' after attribute name.");
            auto init = expression();
            consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after attribute initializer.");
            attributes.push_back({attrName, attrType});
            attributeInitializers.push_back(std::move(init));
        } else {
            auto method = functionDeclaration("method");
            match(TokenType::TOKEN_SEMICOLON);
            methods.push_back(std::unique_ptr<FunctionDeclStmt>(
                static_cast<FunctionDeclStmt*>(method.release())));
        }
    }
    consume(TokenType::TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
    auto node = std::make_unique<ClassDeclStmt>(
        name, typeArguments, std::move(attributes), std::move(methods),
        superclass, std::move(superclassArguments));
    node->attributeInitializers = std::move(attributeInitializers);
    return node;
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
    if (match(TokenType::TOKEN_ARROW)) {
        auto body = expression();
        consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after macro body.");
        return std::make_unique<MacroDeclStmt>(name, parameters, std::move(body), false);
    } else if (match(TokenType::TOKEN_LEFT_BRACE)) {
        auto body = expression();
        consume(TokenType::TOKEN_RIGHT_BRACE, "Expect '}' after macro body.");
        return std::make_unique<MacroDeclStmt>(name, parameters, std::move(body), false);
    }
    errorAtCurrent("Expect '=>' or '{' after macro parameters.");
    throw ParseError("Invalid macro declaration");
}

// ============================================================
// Expresiones (descenso recursivo)
// ============================================================

std::unique_ptr<Expr> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expr> Parser::assignment() {
    auto expr = logicalOr();
    if (match(TokenType::TOKEN_COLON_EQUAL) || match(TokenType::TOKEN_EQUAL)) {
        Token op = previous();
        auto value = assignment();
        if (auto* varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
            return std::make_unique<AssignExpr>(varExpr->name, std::move(value));
        }
        if (auto* getExpr = dynamic_cast<GetExpr*>(expr.get())) {
            return std::make_unique<SetExpr>(std::move(getExpr->object), getExpr->name, std::move(value));
        }
        error(op, "Invalid assignment target.");
        return value;
    }
    return expr;
}

std::unique_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();
    while (match(TokenType::TOKEN_OR)) {
        Token op = previous();
        auto right = logicalAnd();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::logicalAnd() {
    auto expr = equality();
    while (match(TokenType::TOKEN_AND)) {
        Token op = previous();
        auto right = equality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
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
    auto expr = concat();
    while (match({TokenType::TOKEN_LESS, TokenType::TOKEN_LESS_EQUAL,
                  TokenType::TOKEN_GREATER, TokenType::TOKEN_GREATER_EQUAL})) {
        Token op = previous();
        auto right = concat();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::concat() {
    auto expr = term();
    while (match({TokenType::TOKEN_AT, TokenType::TOKEN_AT_AT})) {
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
    auto expr = power();
    while (match({TokenType::TOKEN_STAR, TokenType::TOKEN_SLASH, TokenType::TOKEN_PERCENT})) {
        Token op = previous();
        auto right = power();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::power() {
    auto expr = unary();
    if (match(TokenType::TOKEN_CARET)) {
        Token op = previous();
        auto right = power(); // asociatividad a la derecha
        return std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match({TokenType::TOKEN_BANG, TokenType::TOKEN_NOT, TokenType::TOKEN_MINUS})) {
        Token op = previous();
        auto right = unary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }
    return call();
}

std::unique_ptr<Expr> Parser::finishCall(std::unique_ptr<Expr> callee) {
    auto args = parseArguments();
    Token paren = previous();
    return std::make_unique<CallExpr>(std::move(callee), paren, std::move(args));
}

std::unique_ptr<Expr> Parser::call() {
    auto expr = primary();
    while (true) {
        if (match(TokenType::TOKEN_LEFT_PAREN)) {
            expr = finishCall(std::move(expr));
        } else if (match(TokenType::TOKEN_DOT)) {
            Token name = consume(TokenType::TOKEN_IDENTIFIER, "Expect property name after '.'.");
            expr = std::make_unique<GetExpr>(std::move(expr), name);
        } else if (match(TokenType::TOKEN_LEFT_BRACKET)) {
            auto index = expression();
            consume(TokenType::TOKEN_RIGHT_BRACKET, "Expect ']' after index.");
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::primary() {
    if (match(TokenType::TOKEN_TRUE))  return std::make_unique<LiteralExpr>(true);
    if (match(TokenType::TOKEN_FALSE)) return std::make_unique<LiteralExpr>(false);
    if (match(TokenType::TOKEN_NIL))   return std::make_unique<LiteralExpr>(nullptr);
    if (match(TokenType::TOKEN_NUMBER)) {
        return std::make_unique<LiteralExpr>(std::get<double>(previous().literal));
    }
    if (match(TokenType::TOKEN_STRING)) {
        return std::make_unique<LiteralExpr>(std::get<std::string>(previous().literal));
    }
    if (match(TokenType::TOKEN_SELF)) {
        return std::make_unique<SelfExpr>(previous());
    }
    if (match(TokenType::TOKEN_NEW)) {
        return newExpression();
    }
    if (match(TokenType::TOKEN_LET)) {
        return letExpression();
    }
    if (match(TokenType::TOKEN_IF)) {
        return ifExpression();
    }
    if (match(TokenType::TOKEN_WHILE)) {
        return whileExpression();
    }
    if (match(TokenType::TOKEN_FOR)) {
        return forExpression();
    }
    if (match(TokenType::TOKEN_PRINT)) {
        Token printToken = previous();
        auto value = expression();
        std::vector<std::unique_ptr<Expr>> args;
        args.push_back(std::move(value));
        return std::make_unique<CallExpr>(
            std::make_unique<VariableExpr>(Token{TokenType::TOKEN_IDENTIFIER, "print",
                                                 std::monostate{}, printToken.line}),
            printToken, std::move(args));
    }
    if (match(TokenType::TOKEN_IDENTIFIER)) {
        return std::make_unique<VariableExpr>(previous());
    }
    if (match(TokenType::TOKEN_LEFT_PAREN)) {
        return parseParenthesizedExpression();
    }
    if (check(TokenType::TOKEN_FUNCTION)) {
        return lambdaExpression();
    }
    if (match(TokenType::TOKEN_LEFT_BRACKET)) {
        return arrayLiteralExpression();
    }
    if (match(TokenType::TOKEN_LEFT_BRACE)) {
        return blockExpression();
    }
    errorAtCurrent("Expect expression.");
    throw ParseError("Invalid expression");
}

std::unique_ptr<Expr> Parser::newExpression() {
    Token className = consume(TokenType::TOKEN_IDENTIFIER, "Expect class name after 'new'.");
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after class name.");
    auto args = parseArguments();
    return std::make_unique<NewExpr>(className, std::move(args));
}

std::unique_ptr<Expr> Parser::lambdaExpression() {
    if (match(TokenType::TOKEN_FUNCTION)) {
        std::vector<LambdaExpr::Parameter> parameters;
        if (match(TokenType::TOKEN_LEFT_PAREN)) {
            if (!check(TokenType::TOKEN_RIGHT_PAREN)) {
                do {
                    Token name = consume(TokenType::TOKEN_IDENTIFIER, "Expect parameter name.");
                    Token typeAnnotation; typeAnnotation.type = TokenType::TOKEN_ERROR;
                    if (match(TokenType::TOKEN_COLON)) {
                        typeAnnotation = consume(TokenType::TOKEN_IDENTIFIER, "Expect type name.");
                    }
                    parameters.push_back({name, typeAnnotation});
                } while (match(TokenType::TOKEN_COMMA));
            }
            consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
        }
        Token returnType; returnType.type = TokenType::TOKEN_ERROR;
        if (match(TokenType::TOKEN_COLON)) {
            returnType = consume(TokenType::TOKEN_IDENTIFIER, "Expect return type name.");
        }
        consume(TokenType::TOKEN_ARROW, "Expect '->' after lambda signature.");
        auto body = expression();
        return std::make_unique<LambdaExpr>(std::move(parameters), returnType, std::move(body));
    }

    // Fallback for cases like "let f: (Number) -> Number = function (...)".
    auto body = expression();
    return std::make_unique<LambdaExpr>(std::vector<LambdaExpr::Parameter>{}, Token{TokenType::TOKEN_ERROR, "", std::monostate{}, 0}, std::move(body));
}

std::unique_ptr<Expr> Parser::arrayLiteralExpression() {
    std::vector<std::unique_ptr<Expr>> elements;
    if (!check(TokenType::TOKEN_RIGHT_BRACKET)) {
        do {
            elements.push_back(expression());
        } while (match(TokenType::TOKEN_COMMA));
    }
    consume(TokenType::TOKEN_RIGHT_BRACKET, "Expect ']' after array elements.");
    return std::make_unique<LiteralExpr>(std::string("array"));
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
        LetExpr::Binding b;
        b.name = name;
        b.typeAnnotation = typeAnnotation;
        b.initializer = std::move(initializer);
        bindings.push_back(std::move(b));
    } while (match(TokenType::TOKEN_COMMA));
    consume(TokenType::TOKEN_IN, "Expect 'in' after let bindings.");
    auto body = expression();

    // Desazucarar múltiples ligaduras a lets anidados (una ligadura por nivel),
    // de forma que cada inicializador vea las ligaduras previas.
    for (auto it = bindings.rbegin(); it != bindings.rend(); ++it) {
        std::vector<LetExpr::Binding> single;
        single.push_back(std::move(*it));
        body = std::make_unique<LetExpr>(std::move(single), std::move(body));
    }
    return body;
}

std::unique_ptr<Expr> Parser::ifExpression() {
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    auto condition = expression();
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    auto thenBranch = expression();

    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> elifBranches;
    std::unique_ptr<Expr> elseBranch = nullptr;

    skipSemicolonBeforeElse();
    while (match(TokenType::TOKEN_ELIF)) {
        consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'elif'.");
        auto elifCond = expression();
        consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after elif condition.");
        auto elifBody = expression();
        elifBranches.push_back({std::move(elifCond), std::move(elifBody)});
        skipSemicolonBeforeElse();
    }
    if (match(TokenType::TOKEN_ELSE)) {
        elseBranch = expression();
    }
    return std::make_unique<IfExpr>(std::move(condition), std::move(thenBranch),
                                    std::move(elifBranches), std::move(elseBranch));
}

std::unique_ptr<Expr> Parser::whileExpression() {
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    auto condition = expression();
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after while condition.");
    auto body = expression();
    return std::make_unique<WhileExpr>(std::move(condition), std::move(body));
}

std::unique_ptr<Expr> Parser::forExpression() {
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    std::unique_ptr<Expr> initializer = nullptr;
    if (!check(TokenType::TOKEN_SEMICOLON)) {
        match(TokenType::TOKEN_VAR);
        Token name = consume(TokenType::TOKEN_IDENTIFIER, "Expect loop variable name.");
        if (match(TokenType::TOKEN_COLON)) {
            consume(TokenType::TOKEN_IDENTIFIER, "Expect type name.");
        }
        consume(TokenType::TOKEN_EQUAL, "Expect '=' in for initializer.");
        auto v = expression();
        initializer = std::make_unique<AssignExpr>(name, std::move(v));
    }
    consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after for initializer.");
    std::unique_ptr<Expr> condition = nullptr;
    if (!check(TokenType::TOKEN_SEMICOLON)) {
        condition = expression();
    }
    consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after for condition.");
    std::unique_ptr<Expr> increment = nullptr;
    if (!check(TokenType::TOKEN_RIGHT_PAREN)) {
        increment = expression();
    }
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
    auto body = expression();
    return std::make_unique<ForExpr>(std::move(initializer), std::move(condition),
                                     std::move(increment), std::move(body));
}

std::unique_ptr<Expr> Parser::blockExpression() {
    std::vector<std::unique_ptr<Expr>> expressions;
    while (!check(TokenType::TOKEN_RIGHT_BRACE) && !isAtEnd()) {
        if (match(TokenType::TOKEN_SEMICOLON)) continue;
        expressions.push_back(expression());
        match(TokenType::TOKEN_SEMICOLON);
    }
    consume(TokenType::TOKEN_RIGHT_BRACE, "Expect '}' after block.");
    return std::make_unique<BlockExpr>(std::move(expressions));
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

} // namespace hulk
