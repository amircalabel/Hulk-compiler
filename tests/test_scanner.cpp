// tests/test_scanner.cp (ejemplo de prueba unitaria)
#include <iostream>
#include <cassert>
#include "../src/scanner/Scanner.hpp"
#include "../src/scanner/Token.hpp"

void testNumberLiteral() {
    Scanner scanner("42");
    auto tokens = scanner.scanTokens();
    
    assert(tokens.size() == 2);  // NUMBER + EOF
    assert(tokens[0].type == TokenType::TOKEN_NUMBER);
    assert(std::get<double>(tokens[0].literal) == 42.0);
    
    std::cout << "✓ testNumberLiteral passed" << std::endl;
}

void testStringLiteral() {
    Scanner scanner("\"hello\"");
    auto tokens = scanner.scanTokens();
    
    assert(tokens.size() == 2);
    assert(tokens[0].type == TokenType::TOKEN_STRING);
    assert(std::get<std::string>(tokens[0].literal) == "hello");
    
    std::cout << "✓ testStringLiteral passed" << std::endl;
}

void testLetKeyword() {
    Scanner scanner("let x = 42 in x");
    auto tokens = scanner.scanTokens();
    
    assert(tokens[0].type == TokenType::TOKEN_LET);
    assert(tokens[1].type == TokenType::TOKEN_IDENTIFIER);
    assert(tokens[1].lexeme == "x");
    assert(tokens[2].type == TokenType::TOKEN_EQUAL);
    assert(tokens[3].type == TokenType::TOKEN_NUMBER);
    assert(tokens[4].type == TokenType::TOKEN_IN);
    assert(tokens[5].type == TokenType::TOKEN_IDENTIFIER);
    assert(tokens[5].lexeme == "x");
    
    std::cout << "✓ testLetKeyword passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "Running Scanner Tests..." << std::endl;
    std::cout << std::endl;
    
    testNumberLiteral();
    testStringLiteral();
    testLetKeyword();
    
    std::cout << std::endl;
    std::cout << "All tests passed!" << std::endl;
    
    return 0;
}
