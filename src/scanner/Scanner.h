// src/scanner/Scanner.h
#ifndef HULK_SCANNER_H
#define HULK_SCANNER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Token.h"

class Scanner {
public:
    Scanner(const std::string& source);
    std::vector<Token> scanTokens();

private:
    const std::string& source;
    std::vector<Token> tokens;
    int start = 0;
    int current = 0;
    int line = 1;

    // Mapa de keywords (inicializado estáticamente)
    static std::unordered_map<std::string, TokenType> keywords;

    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    void addToken(TokenType type);
    void addToken(TokenType type, const std::variant<std::monostate, double, std::string>& literal);

    void scanToken();
    void string();
    void number();
    void identifier();

    // Helpers
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
};

#endif // HULK_SCANNER_H