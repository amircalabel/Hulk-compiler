// src/scanner/Scanner.cpp
#include "Scanner.hpp"
#include <cctype>
#include <iostream>

// Forward declaration de la función global (definida en main.cpp)
extern void lexicalError(int line, int col, const std::string& message);

// Inicialización estática de keywords (HULK específico)
std::unordered_map<std::string, TokenType> Scanner::keywords = {
    // Palabras clave de HULK según secciones A.2, A.3, A.4, A.5, A.6, A.7, A.10, A.14
    {"let", TokenType::TOKEN_LET},
    {"in", TokenType::TOKEN_IN},
    {"function", TokenType::TOKEN_FUNCTION},
    {"type", TokenType::TOKEN_TYPE},
    {"protocol", TokenType::TOKEN_PROTOCOL},
    {"def", TokenType::TOKEN_DEF},
    {"if", TokenType::TOKEN_IF},
    {"elif", TokenType::TOKEN_ELIF},
    {"else", TokenType::TOKEN_ELSE},
    {"while", TokenType::TOKEN_WHILE},
    {"for", TokenType::TOKEN_FOR},
    {"return", TokenType::TOKEN_RETURN},
    {"print", TokenType::TOKEN_PRINT},
    {"new", TokenType::TOKEN_NEW},
    {"inherits", TokenType::TOKEN_INHERITS},
    {"self", TokenType::TOKEN_SELF},
    {"base", TokenType::TOKEN_BASE},
    {"is", TokenType::TOKEN_IS},
    {"as", TokenType::TOKEN_AS},
    {"and", TokenType::TOKEN_AND},
    {"or", TokenType::TOKEN_OR},
    {"not", TokenType::TOKEN_NOT},
    {"true", TokenType::TOKEN_TRUE},
    {"false", TokenType::TOKEN_FALSE},
    {"nil", TokenType::TOKEN_NIL},
    {"var", TokenType::TOKEN_VAR} // <-- added var keyword
};

Scanner::Scanner(const std::string& source) : source(source) {}

// Obtener columna
int Scanner::getColumn() const {
    // Encontrar el inicio de la línea actual
    int col = 1;
    for (int i = current - 1; i >= 0; i--) {
        if (source[i] == '\n') break;
        col++;
    }
    return col;
}

// Reportar error lexico
void Scanner::lexicalError(const std::string& message) {
    ::lexicalError(line, getColumn(), message);
}

std::vector<Token> Scanner::scanTokens() {
    while (!isAtEnd()) {
        start = current;
        scanToken();
    }

    tokens.emplace_back(TokenType::TOKEN_EOF, "", std::monostate{}, line);
    return tokens;
}

bool Scanner::isAtEnd() const {
    return current >= static_cast<int>(source.length());
}

char Scanner::advance() {
    return source[current++];
}

char Scanner::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Scanner::peekNext() const {
    if (current + 1 >= static_cast<int>(source.length())) return '\0';
    return source[current + 1];
}

bool Scanner::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    current++;
    return true;
}

void Scanner::addToken(TokenType type) {
    addToken(type, std::monostate{});
}

void Scanner::addToken(TokenType type, const std::variant<std::monostate, double, std::string>& literal) {
    std::string text = source.substr(start, current - start);
    tokens.emplace_back(type, text, literal, line);
}

void Scanner::scanToken() {
    char c = advance();

    // Single-character tokens
    switch (c) {
        case '(' : addToken(TokenType::TOKEN_LEFT_PAREN); break;
        case ')' : addToken(TokenType::TOKEN_RIGHT_PAREN); break;
        case '{' : addToken(TokenType::TOKEN_LEFT_BRACE); break;
        case '}' : addToken(TokenType::TOKEN_RIGHT_BRACE); break;
        case '[' : addToken(TokenType::TOKEN_LEFT_BRACKET); break;
        case ']' : addToken(TokenType::TOKEN_RIGHT_BRACKET); break;
        case ',' : addToken(TokenType::TOKEN_COMMA); break;
        case '.' : addToken(TokenType::TOKEN_DOT); break;
        case '-' : {
            if (peek() == '>') { advance(); addToken(TokenType::TOKEN_ARROW); }
            else addToken(TokenType::TOKEN_MINUS);
            break;
        }
        case '+' : addToken(TokenType::TOKEN_PLUS); break;
        case ';' : addToken(TokenType::TOKEN_SEMICOLON); break;
        case '*' : addToken(TokenType::TOKEN_STAR); break;
        case '%' : addToken(TokenType::TOKEN_PERCENT); break;
        case '^' : addToken(TokenType::TOKEN_CARET); break;
        case '@' : {
            // @@ es el operador de concatenación con espacio
            if (peek() == '@') {
                advance();
                addToken(TokenType::TOKEN_AT_AT);
            } else {
                addToken(TokenType::TOKEN_AT);
            }
            break;
        }
        case ':' : {
            if (match('=')) {
                addToken(TokenType::TOKEN_COLON_EQUAL);  // :=
            } else {
                addToken(TokenType::TOKEN_COLON);
            }
            break;
        }
        case '/' : {
            if (match('/')) {
                // Comentario hasta fin de línea
                while (peek() != '\n' && !isAtEnd()) advance();
            } else {
                addToken(TokenType::TOKEN_SLASH);
            }
            break;
        }
        case '!' :
            addToken(match('=') ? TokenType::TOKEN_BANG_EQUAL : TokenType::TOKEN_BANG);
            break;
        case '=' :
            if (match('>')) {
                addToken(TokenType::TOKEN_ARROW);
            } else if (match('=')) {
                addToken(TokenType::TOKEN_EQUAL_EQUAL);
            } else {
                addToken(TokenType::TOKEN_EQUAL);
            }
            break;
        case '<' :
            addToken(match('=') ? TokenType::TOKEN_LESS_EQUAL : TokenType::TOKEN_LESS);
            break;
        case '>' :
            addToken(match('=') ? TokenType::TOKEN_GREATER_EQUAL : TokenType::TOKEN_GREATER);
            break;
        case ' ':
        case '\r':
        case '\t':
            // Ignorar whitespace
            break;
        case '\n':
            line++;
            break;
        case '"':
            string();
            break;
        default:
            if (isDigit(c)) {
                number();
            } else if (isAlpha(c)) {
                identifier();
            } else {
                // Caracter inesperado
                lexicalError("unexpected character '" + std::string(1, c) + "'");
                addToken(TokenType::TOKEN_ERROR);
            }
            break;
    }
}

void Scanner::string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') line++;
        advance();
    }

    if (isAtEnd()) {
        // String sin cerrar - REPORTAR ERROR LÉXICO
        lexicalError("unterminated string");
        addToken(TokenType::TOKEN_ERROR);
        return;
    }

    // Consumir el " de cierre
    advance();

    // Extraer el contenido de la string (sin las comillas)
    std::string value = source.substr(start + 1, current - start - 2);
    addToken(TokenType::TOKEN_STRING, value);
}

void Scanner::number() {
    while (isDigit(peek())) advance();

    // Parte fraccionaria
    if (peek() == '.' && isDigit(peekNext())) {
        advance(); // consumir el .
        while (isDigit(peek())) advance();
    }

    double value = std::stod(source.substr(start, current - start));
    addToken(TokenType::TOKEN_NUMBER, value);
}

void Scanner::identifier() {
    while (isAlphaNumeric(peek()) || peek() == '$') advance();

    std::string text = source.substr(start, current - start);

    auto it = keywords.find(text);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::TOKEN_IDENTIFIER;
    addToken(type);
}

bool Scanner::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Scanner::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_' ||
           c == '$';
}

bool Scanner::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}
