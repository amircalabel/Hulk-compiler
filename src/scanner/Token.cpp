// src/scanner/Token.cpp
#include "Token.hpp"

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::TOKEN_LEFT_PAREN: return "LEFT_PAREN";
        case TokenType::TOKEN_RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenType::TOKEN_LEFT_BRACE: return "LEFT_BRACE";
        case TokenType::TOKEN_RIGHT_BRACE: return "RIGHT_BRACE";
        case TokenType::TOKEN_COMMA: return "COMMA";
        case TokenType::TOKEN_DOT: return "DOT";
        case TokenType::TOKEN_MINUS: return "MINUS";
        case TokenType::TOKEN_PLUS: return "PLUS";
        case TokenType::TOKEN_SEMICOLON: return "SEMICOLON";
        case TokenType::TOKEN_SLASH: return "SLASH";
        case TokenType::TOKEN_STAR: return "STAR";
        case TokenType::TOKEN_CARET: return "CARET";
        case TokenType::TOKEN_AT: return "AT";
        case TokenType::TOKEN_AT_AT: return "AT_AT";
        case TokenType::TOKEN_BANG: return "BANG";
        case TokenType::TOKEN_BANG_EQUAL: return "BANG_EQUAL";
        case TokenType::TOKEN_EQUAL: return "EQUAL";
        case TokenType::TOKEN_EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TokenType::TOKEN_GREATER: return "GREATER";
        case TokenType::TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::TOKEN_LESS: return "LESS";
        case TokenType::TOKEN_LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::TOKEN_COLON: return "COLON";
        case TokenType::TOKEN_COLON_EQUAL: return "COLON_EQUAL";
        case TokenType::TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TokenType::TOKEN_STRING: return "STRING";
        case TokenType::TOKEN_NUMBER: return "NUMBER";
        case TokenType::TOKEN_LET: return "LET";
        case TokenType::TOKEN_IN: return "IN";
        case TokenType::TOKEN_FUNCTION: return "FUNCTION";
        case TokenType::TOKEN_TYPE: return "TYPE";
        case TokenType::TOKEN_PROTOCOL: return "PROTOCOL";
        case TokenType::TOKEN_DEF: return "DEF";
        case TokenType::TOKEN_IF: return "IF";
        case TokenType::TOKEN_ELIF: return "ELIF";
        case TokenType::TOKEN_ELSE: return "ELSE";
        case TokenType::TOKEN_WHILE: return "WHILE";
        case TokenType::TOKEN_FOR: return "FOR";
        case TokenType::TOKEN_RETURN: return "RETURN";
        case TokenType::TOKEN_PRINT: return "PRINT";
        case TokenType::TOKEN_NEW: return "NEW";
        case TokenType::TOKEN_INHERITS: return "INHERITS";
        case TokenType::TOKEN_SELF: return "SELF";
        case TokenType::TOKEN_BASE: return "BASE";
        case TokenType::TOKEN_IS: return "IS";
        case TokenType::TOKEN_AS: return "AS";
        case TokenType::TOKEN_TRUE: return "TRUE";
        case TokenType::TOKEN_FALSE: return "FALSE";
        case TokenType::TOKEN_NIL: return "NIL";
        case TokenType::TOKEN_ERROR: return "ERROR";
        case TokenType::TOKEN_EOF: return "EOF";
        default: return "UNKNOWN";
    }
}

Token::Token(TokenType type, const std::string& lexeme,
             const std::variant<std::monostate, double, std::string>& literal,
             int line)
    : type(type), lexeme(lexeme), literal(literal), line(line) {}

std::string Token::toString() const {
    return tokenTypeToString(type) + " " + lexeme + " ";
}