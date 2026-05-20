// src/scanner/Token.hpp
#ifndef HULK_TOKEN_HPP
#define HULK_TOKEN_HPP

#include <string>
#include <variant>

// Tipos de token (todos los de HULK)
enum class TokenType {
    // Single-character tokens
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    TOKEN_CARET,      // ^ (potencia)
    TOKEN_AT,         // @ (concatenación)
    TOKEN_AT_AT,      // @@ (concat con espacio)

    // Añadir a enum TokenType
    TOKEN_VAR,           // var (statement)
    TOKEN_ARROW,         // =>
    TOKEN_DOLLAR,        // $ (para placeholders en macros)

    // One or two character tokens
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_COLON,      // : (anotación de tipo)
    TOKEN_COLON_EQUAL, // := (asignación destructiva)

    // Literals
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

    // Keywords
    TOKEN_LET, TOKEN_IN, TOKEN_FUNCTION, TOKEN_TYPE, TOKEN_PROTOCOL,
    TOKEN_DEF, TOKEN_IF, TOKEN_ELIF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_FOR,
    TOKEN_RETURN, TOKEN_PRINT, TOKEN_NEW, TOKEN_INHERITS,
    TOKEN_SELF, TOKEN_BASE, TOKEN_IS, TOKEN_AS,
    TOKEN_TRUE, TOKEN_FALSE, TOKEN_NIL,

    TOKEN_ERROR,
    TOKEN_EOF
};

// Para debugging: convertir TokenType a string
std::string tokenTypeToString(TokenType type);

// Clase Token (similar a Java pero con std::variant para literales)
class Token {
public:
    TokenType type;
    std::string lexeme;
    std::variant<std::monostate, double, std::string> literal;  // número o string
    int line;

    Token(TokenType type, const std::string& lexeme, const std::variant<std::monostate, double, std::string>& literal, int line);

    std::string toString() const;
};

#endif // HULK_TOKEN_HPP