#include "scanner.h"
#include <string.h>
#include "common.h"

// Structure representing the current state of the scanner
typedef struct {
    const char* start; // Pointer to the start of the current lexeme
    const char* current; // Pointer to the current character being processed
    int line; // Current line number in the source code
} Scanner;

// Global instance of the Scanner structure
Scanner scanner;

// Initialize the scanner with the given source code
void initScanner(const char* source) {
    scanner = (Scanner){ .start = source, .current = source, .line = 1 };
}

// Helper function to check if a character is an alphabetic character or underscore
static bool isAlpha(char c) {
    return strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_", c) != NULL;
}

// Helper function to check if a character is a digit
static bool isDigit(char c) { return strchr("0123456789", c) != NULL; }

// Helper function to check if the scanner has reached the end of the source code
static bool isAtEnd() { return *scanner.current == '\0'; }

// Advance the current character pointer and return the character
static char advance() {
    return *(scanner.current)++;
}

// Peek at the current character without advancing the pointer
static char peek() { return *scanner.current; }

// Peek at the next character without advancing the pointer
static char peekNext() {
    return isAtEnd() ? '\0' : scanner.current[1];
}

// Match the current character with the expected character and advance if matched
static bool match(char expected) {
    if (isAtEnd() || *scanner.current != expected) return false;
    scanner.current++;
    return true;
}

// Create a new Token with the given type, text, and length
static Token createToken(TokenType type, const char* text, int length) {
    return (Token){ .type = type, .start = text, .length = length, .line = scanner.line };
}

// Create a Token for a specific TokenType without explicit lexeme information
static Token makeToken(TokenType type) {
    return createToken(type, scanner.start, scanner.current - scanner.start);
}

// Create an error Token with the given error message
static Token errorToken(const char* message) {
    return createToken(TOKEN_ERROR, message, strlen(message));
}

// Skip over whitespace characters and comments
static void skipWhitespace() {
    while (true) {
        char c = peek();
        switch (c) {
            case ' ': case '\r': case '\t': case '\n':
                if (c == '\n') scanner.line++;
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    // Comment found, skip until the end of the line
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

// Check if the lexeme starting at 'start' matches a keyword, return the corresponding TokenType
static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

// Determine the TokenType for an identifier based on keywords
static TokenType identifierType() {
    // Structure to hold keyword information
    struct Keyword {
        const char* name;
        int length;
        TokenType type;
    };

    // Array of keywords with their corresponding TokenType
    static const struct Keyword keywords[] = {
            {"and", 3, TOKEN_AND}, {"class", 5, TOKEN_CLASS}, {"else", 4, TOKEN_ELSE},
            {"false", 5, TOKEN_FALSE}, {"for", 3, TOKEN_FOR}, {"fun", 3, TOKEN_FUN},
            {"if", 2, TOKEN_IF}, {"nil", 3, TOKEN_NIL}, {"or", 2, TOKEN_OR},
            {"print", 5, TOKEN_PRINT}, {"return", 6, TOKEN_RETURN},
            {"true", 4, TOKEN_TRUE}, {"var", 3, TOKEN_VAR}, {"while", 5, TOKEN_WHILE},
            {"is", 2, TOKEN_EQUAL},
            {"matches", 7, TOKEN_EQUAL_EQUAL},
            {"say", 3, TOKEN_PRINT},
            {"give", 4, TOKEN_RETURN},
            {"action", 6, TOKEN_FUN},
            {"store", 5, TOKEN_VAR},
            {NULL, 0, TOKEN_IDENTIFIER}
    };

    int length = scanner.current - scanner.start;
    for (const struct Keyword* kw = keywords; kw->length != 0; ++kw) {
        if (kw->length == length && memcmp(scanner.start, kw->name, length) == 0) {
            return kw->type;
        }
    }
    return TOKEN_IDENTIFIER;
}

// Handle identifier lexemes
static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

// Handle numeric literal lexemes
static Token number() {
    while (isDigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) {
        advance();  // Consume the ".".

        while (isDigit(peek())) advance();
    }

    return makeToken(TOKEN_NUMBER);
}

// Handle string literal lexemes
static Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }

    if (isAtEnd()) return errorToken("Unterminated string.");

    advance();  // Consume the closing quote.
    return makeToken(TOKEN_STRING);
}

// Main function to scan the next token in the source code
Token scanToken() {
    skipWhitespace();       // Skip over whitespace and comments

    scanner.start = scanner.current;  // Mark the start of the current lexeme

    if (isAtEnd()) return makeToken(TOKEN_EOF);  // Check for the end of the source code

    char c = advance();  // Get the next character from the source code

    if (isAlpha(c)) return identifier();  // Handle identifiers (keywords or user-defined)
    if (isDigit(c)) return number();      // Handle numeric literals

    // Handle single-character tokens
    switch (c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);  // Handle '!' and '!='
        case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);  // Handle '=' and '=='
        case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);    // Handle '<' and '<='
        case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);  // Handle '>' and '>='
        case '"': return string();  // Handle string literals
    }

    return errorToken("Unexpected character.");  // Report an error for unexpected characters
}
