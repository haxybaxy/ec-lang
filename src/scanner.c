#include "scanner.h"
#include <stdio.h>
#include <string.h>
#include "common.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
    scanner = (Scanner){ .start = source, .current = source, .line = 1 };
}

static bool isAlpha(char c) {
    return strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_", c) != NULL;
}

static bool isDigit(char c) { return strchr("0123456789", c) != NULL; }

static bool isAtEnd() { return *scanner.current == '\0'; }

static char advance() {
    return *(scanner.current)++;
}

static char peek() { return *scanner.current; }

static char peekNext() {
    return isAtEnd() ? '\0' : scanner.current[1];
}

static bool match(char expected) {
    if (isAtEnd() || *scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static Token createToken(TokenType type, const char* text, int length) {
    return (Token){ .type = type, .start = text, .length = length, .line = scanner.line };
}

static Token makeToken(TokenType type) {
    return createToken(type, scanner.start, scanner.current - scanner.start);
}

static Token errorToken(const char* message) {
    return createToken(TOKEN_ERROR, message, strlen(message));
}

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

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
    struct Keyword {
        const char* name;
        int length;
        TokenType type;
    };

    static const struct Keyword keywords[] = {
            {"and", 3, TOKEN_AND}, {"class", 5, TOKEN_CLASS}, {"else", 4, TOKEN_ELSE},
            {"false", 5, TOKEN_FALSE}, {"for", 3, TOKEN_FOR}, {"fun", 3, TOKEN_FUN},
            {"if", 2, TOKEN_IF}, {"nil", 3, TOKEN_NIL}, {"or", 2, TOKEN_OR},
            {"print", 5, TOKEN_PRINT}, {"return", 6, TOKEN_RETURN},
            {"true", 4, TOKEN_TRUE}, {"var", 3, TOKEN_VAR}, {"do", 3, TOKEN_WHILE},
            {"divide", 6, TOKEN_SLASH},
            {"is", 2, TOKEN_EQUAL},
            {"issameas", 8, TOKEN_EQUAL_EQUAL},
            {"give", 4, TOKEN_RETURN},
            {"howbig", 6, TOKEN_SIZEOF},  // Replace "sizeof" with "howbig"
            {NULL, 0, TOKEN_IDENTIFIER} // Sentinel to mark end of array
    };

    int length = scanner.current - scanner.start;
    for (const struct Keyword* kw = keywords; kw->length != 0; ++kw) {
        if (kw->length == length && memcmp(scanner.start, kw->name, length) == 0) {
            return kw->type;
        }
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}


static Token number() {
    while (isDigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) {
        advance();  // Consume the ".".

        while (isDigit(peek())) advance();
    }

    return makeToken(TOKEN_NUMBER);
}

static Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }

    if (isAtEnd()) return errorToken("Unterminated string.");

    advance();  // The closing quote.
    return makeToken(TOKEN_STRING);
}

Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    char c = advance();
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    switch (c) {
        case '(':
            return makeToken(TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(TOKEN_RIGHT_BRACE);
        case ';':
            return makeToken(TOKEN_SEMICOLON);
        case ',':
            return makeToken(TOKEN_COMMA);
        case '.':
            return makeToken(TOKEN_DOT);
        case '-':
            return makeToken(TOKEN_MINUS);
        case '+':
            return makeToken(TOKEN_PLUS);
        case '/':
            return makeToken(TOKEN_SLASH);
        case '*':
            return makeToken(TOKEN_STAR);
        case '!':
            return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"':
            return string();
    }

    return errorToken("Unexpected character.");
}