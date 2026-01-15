#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "memory.h"

// Print a string with C-style escapes (e.g., "\n" for newline)
static void print_escaped_string(const char *s);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // Open the file
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate buffer
    char *buffer = (char *)s_malloc(file_size + 1);
    if (!buffer) {
        perror("Error allocating memory");
        fclose(file);
        return 1;
    }

    // read the file into the buffer
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';  // Null-terminate the string
    fclose(file);

    Lexer lexer = {
        .start_tok = buffer,
        .cur_tok = buffer,
        .line_start = buffer,
    };

    // Call the lex function (to be implemented)
    int result = lex(&lexer);

    if (result == 1) {
        s_free(buffer);
        free_lexer(&lexer);
        return 1;
    }

    for (int i = 0; i < lexer.token_count; i++) {
        TokenData token = lexer.tokens[i];

        if (token.type == tok_identifier || token.type == tok_string || token.type == tok_number || token.type == tok_type) {
            printf("%s(", token_to_string(token.type));
            print_escaped_string(token.val);
            printf(")\n");
        } else {
            printf("%s\n", token_to_string(token.type));
        }
    }

    // Free the buffer
    s_free(buffer);
    free_lexer(&lexer);
    return 0;
}

// Print a string with C-style escapes (e.g., "\n" for newline)
static void print_escaped_string(const char *s) {
    for (; *s; ++s) {
        switch (*s) {
            case '\n': printf("\\n"); break;
            case '\t': printf("\\t"); break;
            case '\r': printf("\\r"); break;
            case '\\': printf("\\\\"); break;
            case '"': printf("\\\""); break;
            default:
                if ((unsigned char)*s < 32 || (unsigned char)*s == 127) {
                    printf("\\x%02x", (unsigned char)*s);
                } else {
                    putchar(*s);
                }
        }
    }
}
