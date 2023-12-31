#include <stdio.h>
#include <stdlib.h>
#include "vm.h"

#define BUFFER_SIZE 1024

// Interactive REPL (Read-Eval-Print Loop)
static void repl() {
    char line[BUFFER_SIZE];
    while (printf("> "), fgets(line, sizeof(line), stdin)) {
        interpret(line);
    }
    printf("\n");
}

// Reads the content of a file and returns it as a string.
static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = malloc(fileSize + 1);
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        free(buffer);
        fclose(file);
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}

// Runs the script from a file and handles interpreter errors.
static void runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

// Entry point of the program.
int main(int argc, const char* argv[]) {
    initVM(); // Initialize the Virtual Machine.

    if (argc == 1) {
        repl(); // Start the REPL if no command-line arguments are provided.
    } else if (argc == 2) {
        runFile(argv[1]); // Run the script if a file path is provided as an argument.
    } else {
        fprintf(stderr, "Usage: eclang [path]\n");
        exit(64); // Exit with an error code for incorrect command-line usage.
    }

    freeVM(); // Free resources used by the Virtual Machine.
    return 0; // Explicit return statement for clarity.
}
