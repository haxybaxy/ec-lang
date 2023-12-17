#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "scanner.h"

// Structure to track parsing information
typedef struct {
    Token current;    // Current token being parsed
    Token previous;   // Previous token
    bool hadError;    // Flag indicating whether an error has occurred
    bool panicMode;   // Flag indicating panic mode (error recovery mode)
} Parser;

// Enumeration representing the precedence levels of operators
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY,
} Precedence;

// Function pointer type for parsing prefix and infix expressions
typedef void (*ParseFn)(bool canAssign);

// Rule for parsing expressions
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

// Structure representing a local variable
typedef struct {
    Token name;
    int depth;
    bool isCaptured;
} Local;

// Structure representing an upvalue
typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

// Enumeration representing the type of a function
typedef enum {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_SCRIPT
} FunctionType;

// Compiler structure
typedef struct Compiler {
    struct Compiler* enclosing;
    ObjFunction* function;
    FunctionType type;
    Local locals[UINT8_COUNT];
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;
} Compiler;

// Global parser instance
Parser parser;

// Global current compiler instance
Compiler* current = NULL;

// Function to retrieve the current bytecode chunk
static Chunk* currentChunk() {
    return &current->function->chunk;
}

// Function to report an error at a specific token with a given message
static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

// Function to report an error at the previous token with a given message
static void error(const char* message) {
    errorAt(&parser.previous, message);
}

// Function to report an error at the current token with a given message
static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

// Function to advance the parser to the next token
static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

// Function to consume the current token if it matches the given type; otherwise, report an error
static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

// Function to check if the current token matches the given type
static bool check(TokenType type) {
    return parser.current.type == type;
}

// Function to match the current token with the given type and advance if there is a match
static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

// Function to emit a single bytecode instruction
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

// Function to emit two consecutive bytecode instructions
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

// Function to emit a loop instruction with the target offset
static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large.");

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

// Function to emit a jump instruction with an initial placeholder offset
static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

// Function to emit a return instruction based on the function type
static void emitReturn() {
    if (current->type == TYPE_INITIALIZER) {
        emitBytes(OP_GET_LOCAL, 0);
    } else {
        emitByte(OP_NIL);
    }

    emitByte(OP_RETURN);
}

// Function to create a constant value and return its index
static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

// Function to emit a constant instruction with a given constant value
static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

// Function to patch a jump instruction with the correct offset
static void patchJump(int offset) {
    int jump = currentChunk()->count - offset - 2;
    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

// Function to initialize a new compiler instance
static void initCompiler(Compiler* compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name =
                copyString(parser.previous.start, parser.previous.length);
    }

    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type != TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
}

// Function to finalize the compilation and return the resulting function
static ObjFunction* endCompiler() {
    emitReturn();
    ObjFunction* function = current->function;
    current = current->enclosing;
    return function;
}

// Function to begin a new scope
static void beginScope() {
    current->scopeDepth++;
}

// This function is responsible for ending the current scope.
static void endScope() {
    current->scopeDepth--;

    // Pop local variables until we reach the current scope depth.
    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth > current->scopeDepth) {
        // If the local variable is captured by an upvalue, emit OP_CLOSE_UPVALUE.
        // Otherwise, simply pop the local variable from the stack using OP_POP.
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
        current->localCount--;
    }
}

// Function declarations for various parsing tasks.
static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

// Converts an identifier token into a constant value and adds it to the constants array.
static uint8_t identifierConstant(Token* name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

// Checks if two identifier tokens are equal by comparing their lengths and content.
static bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

// Resolves a local variable in the current compiler scope.
static int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            // Ensure that a local variable is not read in its own initializer.
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

// Adds an upvalue to the current compiler's function.
static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    // Ensure that the number of closure variables does not exceed the limit.
    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

// Resolves an upvalue for a variable in the enclosing scope.
static int resolveUpvalue(Compiler* compiler, Token* name) {
    if (compiler->enclosing == NULL) return -1;

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        // If the variable is found in the local scope of the enclosing compiler,
        // mark it as captured and add it as an upvalue in the current function.
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        // If the variable is found as an upvalue in the enclosing scope,
        // add it as an upvalue in the current function.
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

// Adds a local variable to the current compiler's locals array.
static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}

// Declares a variable in the current scope.
static void declareVariable() {
    if (current->scopeDepth == 0) return;

    Token* name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        // Check for variable name conflicts within the current scope.
        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }
    addLocal(*name);
}

// Parses a variable and returns its constant index.
static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) return 0;

    return identifierConstant(&parser.previous);
}

// Marks the most recent local variable as initialized within its scope.
static void markInitialized() {
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

// Defines a variable by emitting the appropriate bytecode instruction.
static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

// Parses the list of arguments in a function call and returns the argument count.
static uint8_t argumentList() {
    uint8_t argCount = 0;
    // Check for an empty argument list or ')' indicating the end of the list.
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            // Check for the maximum argument count limit.
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

// Handles the 'and' logical operator.
static void and_(bool canAssign) {
    // Jump to the end if the left operand is false.
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    // Pop the left operand.
    emitByte(OP_POP);

    // Parse the right operand with higher precedence.
    parsePrecedence(PREC_AND);

    // Patch the jump instruction.
    patchJump(endJump);
}

// Handles binary operators.
static void binary(bool canAssign) {
    // Get the operator type and corresponding parsing rule.
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);

    // Parse the right operand with higher precedence.
    parsePrecedence((Precedence)(rule->precedence + 1));

    // Emit bytecode for the binary operation based on the operator type.
    switch (operatorType) {
        case TOKEN_BANG_EQUAL:
            emitBytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OP_GREATER, OP_NOT);
            break;
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE);
            break;
        default:
            return;  // Unreachable.
    }
}

// Handles function calls.
static void call(bool canAssign) {
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

// Handles literals (true, false, nil).
static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emitByte(OP_FALSE);
            break;
        case TOKEN_NIL:
            emitByte(OP_NIL);
            break;
        case TOKEN_TRUE:
            emitByte(OP_TRUE);
            break;
        default:
            return;  // Unreachable.
    }
}

// Parses grouped expressions inside parentheses.
static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

// Handles numeric literals.
static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

// Handles the 'or' logical operator.
static void or_(bool canAssign) {
    // Jump to the else branch if the left operand is false.
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    // Jump to the end, indicating true, if the left operand is true.
    int endJump = emitJump(OP_JUMP);

    // Patch the jump to the else branch.
    patchJump(elseJump);
    // Pop the left operand.
    emitByte(OP_POP);

    // Parse the right operand with higher precedence.
    parsePrecedence(PREC_OR);

    // Patch the jump to the end.
    patchJump(endJump);
}

// Handles string literals.
static void string(bool canAssign) {
    // Create a string object from the substring within quotes and emit constant.
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

// Handles variable references.
static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        // The variable is a global variable.
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    // Check if the assignment operator is present.
    if (canAssign && match(TOKEN_EQUAL)) {
        // Parse the right-hand side of the assignment.
        expression();
        // Emit the bytecode for setting the variable's value.
        emitBytes(setOp, (uint8_t)arg);
    } else {
        // Emit the bytecode for getting the variable's value.
        emitBytes(getOp, (uint8_t)arg);
    }
}

// Handles variable references in expressions.
static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

// Creates a synthetic token for error recovery.
static Token syntheticToken(const char* text) {
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}

// Handles unary operators.
static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    // Parse the operand with higher precedence.
    parsePrecedence(PREC_UNARY);

    // Emit the corresponding unary operator instruction.
    switch (operatorType) {
        case TOKEN_BANG:
            emitByte(OP_NOT);
            break;
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        default:
            return;
    }
}


ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

// Parses expressions with a given precedence level.
static void parsePrecedence(Precedence precedence) {
    advance();

    // Get the prefix rule for the current token.
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expected expression.");
        return;
    }

    // Determine if the expression can be assigned to.
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    // Parse the expression with the prefix rule.
    prefixRule(canAssign);

    // Parse infix expressions with higher precedence until a lower precedence is encountered.
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        // Get the infix rule for the current token.
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        // Parse the infix expression.
        infixRule(canAssign);
    }

    // Check for invalid assignment target after parsing the expression.
    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

// Retrieves the parsing rule for a given token type.
static ParseRule* getRule(TokenType type) { return &rules[type]; }

// Entry point for parsing expressions.
static void expression() { parsePrecedence(PREC_ASSIGNMENT); }

// Parses a block of statements enclosed in curly braces.
static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

// Parses a function definition.
static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    // Parse function parameters if any.
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;

            // Check for the maximum number of parameters allowed.
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }

            // Parse and define each parameter.
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    // Finalize the function compilation and emit the closure instruction.
    ObjFunction* function = endCompiler();
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    // Emit the upvalue information for the function.
    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

// Parses a function declaration.
static void funDeclaration() {
    // Parse the function name.
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();

    // Compile the function definition.
    function(TYPE_FUNCTION);
    defineVariable(global);
}

// Parses a variable declaration.
static void varDeclaration() {
    // Parse the variable name.
    uint8_t global = parseVariable("Expect variable name.");

    // Check for an initializer and parse it if present.
    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        // Default to nil if no initializer is provided.
        emitByte(OP_NIL);
    }

    // Consume the semicolon to complete the variable declaration.
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    // Define the variable in the current scope.
    defineVariable(global);
}

// Parses an expression statement.
static void expressionStatement() {
    // Parse the expression.
    expression();
    // Consume the semicolon to complete the statement.
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    // Pop the result of the expression from the stack.
    emitByte(OP_POP);
}

// Parses a 'for' statement.
static void forStatement() {
    beginScope();

    // Parse the initializer part of the 'for' statement.
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {
        // No initializer.
    } else if (match(TOKEN_VAR)) {
        // Variable declaration initializer.
        varDeclaration();
    } else {
        // Expression initializer.
        expressionStatement();
    }

    // Record the starting point of the loop in the bytecode.
    int loopStart = currentChunk()->count;
    int exitJump = -1;

    // Parse the condition part of the 'for' statement.
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);  // Discard the loop condition value.
    }

    // Parse the increment part of the 'for' statement.
    if (!match(TOKEN_RIGHT_PAREN)) {
        // Record the starting point of the increment code.
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;

        // Parse the increment expression.
        expression();
        emitByte(OP_POP);  // Discard the increment value.

        // Consume the closing parenthesis.
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        // Emit the loop back jump and patch the body jump.
        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    // Parse the body of the 'for' statement.
    statement();

    // Emit the loop back jump.
    emitLoop(loopStart);

    // Patch the exit jump if it was recorded.
    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP);  // Discard the loop condition value.
    }

    endScope();
}

// Parses an 'if' statement.
static void ifStatement() {
    // Parse the condition in the 'if' statement.
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    // Jump to the 'else' branch if the condition is false.
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);  // Discard the condition value.

    // Parse the 'then' branch.
    statement();

    // Jump over the 'else' branch to the end of the 'if' statement.
    int elseJump = emitJump(OP_JUMP);

    // Patch the jump to the 'else' branch.
    patchJump(thenJump);
    emitByte(OP_POP);  // Discard the condition value.

    // Parse the 'else' branch if present.
    if (match(TOKEN_ELSE)) statement();

    // Patch the jump to the end of the 'if' statement.
    patchJump(elseJump);
}

static void printStatement() {
    // Parse the expression to be printed.
    expression();
    // Consume the semicolon to complete the statement.
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    // Emit the 'OP_PRINT' instruction to print the value on the stack.
    emitByte(OP_PRINT);
}

// Parses and handles the 'return' statement.
static void returnStatement() {
    // Check if the 'return' is used in top-level code, which is not allowed.
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    // Check if 'return' is followed by a semicolon, indicating no return value.
    if (match(TOKEN_SEMICOLON)) {
        // Emit the 'OP_RETURN' instruction without a return value.
        emitReturn();
    } else {
        // Check if returning a value from an initializer, which is not allowed.
        if (current->type == TYPE_INITIALIZER) {
            error("Can't return a value from an initializer.");
        }

        // Parse the expression to be returned.
        expression();
        // Consume the semicolon to complete the statement.
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        // Emit the 'OP_RETURN' instruction with the return value.
        emitByte(OP_RETURN);
    }
}

// Parses and handles the 'while' statement.
static void whileStatement() {
    // Record the starting point of the loop in the bytecode.
    int loopStart = currentChunk()->count;
    // Parse the condition in the 'while' statement.
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    // Consume the closing parenthesis after the condition.
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    // Jump to the end of the loop if the condition is false.
    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);  // Discard the condition value.

    // Parse the body of the 'while' loop.
    statement();

    // Emit a loop back jump to the beginning of the loop.
    emitLoop(loopStart);

    // Patch the exit jump to the end of the loop.
    patchJump(exitJump);
    emitByte(OP_POP);  // Discard the loop condition value.
}

// Synchronizes the parser state in case of errors, so it can continue parsing.
static void synchronize() {
    parser.panicMode = false;

    // Continue advancing tokens until reaching the end of the file or a statement boundary.
    while (parser.current.type != TOKEN_EOF) {
        // Check for semicolon as a statement boundary.
        if (parser.previous.type == TOKEN_SEMICOLON) return;

        // Check for specific tokens that can indicate the start of a new statement.
        switch (parser.current.type) {
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default:;  // Do nothing.
        }

        // Advance to the next token.
        advance();
    }
}

// Parses a declaration, which can be a function, variable, or a statement.
static void declaration() {
    // Check the current token type to determine the declaration type.
    if (match(TOKEN_FUN)) {
        // Parse a function declaration.
        funDeclaration();
    } else if (match(TOKEN_VAR)) {
        // Parse a variable declaration.
        varDeclaration();
    } else {
        // Parse a statement if the token does not match any known declaration type.
        statement();
    }

    // Synchronize the parser state in case of errors.
    if (parser.panicMode) synchronize();
}

// Parses a statement, which can be a print, if, return, while, for, block, or an expression statement.
static void statement() {
    if (match(TOKEN_PRINT)) {
        // Parse and handle the 'print' statement.
        printStatement();
    } else if (match(TOKEN_IF)) {
        // Parse and handle the 'if' statement.
        ifStatement();
    } else if (match(TOKEN_RETURN)) {
        // Parse and handle the 'return' statement.
        returnStatement();
    } else if (match(TOKEN_WHILE)) {
        // Parse and handle the 'while' statement.
        whileStatement();
    } else if (match(TOKEN_FOR)) {
        // Parse and handle the 'for' statement.
        forStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        // Parse and handle a block of statements enclosed in curly braces.
        beginScope();
        block();
        endScope();
    } else {
        // Parse and handle an expression statement.
        expressionStatement();
    }
}

// Compiles the provided source code into an ObjFunction.
ObjFunction* compile(const char* source) {
    // Initialize the scanner and compiler.
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    // Reset error and panic mode flags.
    parser.hadError = false;
    parser.panicMode = false;

    // Start parsing from the first token.
    advance();

    // Parse declarations until reaching the end of the file.
    while (!match(TOKEN_EOF)) {
        declaration();
    }

    // End the compilation and retrieve the resulting ObjFunction.
    ObjFunction* function = endCompiler();
    // Return NULL if there were compilation errors, otherwise, return the ObjFunction.
    return parser.hadError ? NULL : function;
}

// Marks roots of the compiler in the garbage collector.
void markCompilerRoots() {
    Compiler* compiler = current;
    while (compiler != NULL) {
        // Mark the current function as a root in the garbage collector.
        markObject((Obj*)compiler->function);
        // Move to the enclosing compiler in the chain.
        compiler = compiler->enclosing;
    }
}
