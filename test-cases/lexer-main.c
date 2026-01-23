#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "memory.h"

// Helper: write a string with C-style escapes into a buffer (returns buffer pointer)
static char* escape_c_string(const char* s);

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
            char* escaped_val = escape_c_string(token.val);
            printf("%s(%s)\n", token_to_string(token.type), escaped_val);
            s_free(escaped_val);
        } else {
            printf("%s\n", token_to_string(token.type));
        }
    }

    // Free the buffer
    s_free(buffer);
    free_lexer(&lexer);
    return 0;
}

// Helper: write a string with C-style escapes into a buffer (returns buffer pointer)
static char* escape_c_string(const char* s) {
    // Allocate a buffer large enough for worst case (every char is escaped)
    size_t len = strlen(s);
    char* buf = s_malloc(len * 4 + 1); // plenty of space
    char* p = buf;
    for (; *s; ++s) {
        switch (*s) {
            case '\n': *p++ = '\\'; *p++ = 'n'; break;
            case '\t': *p++ = '\\'; *p++ = 't'; break;
            case '\r': *p++ = '\\'; *p++ = 'r'; break;
            case '\\': *p++ = '\\'; *p++ = '\\'; break;
            case '"': *p++ = '\\'; *p++ = '"'; break;
            default:
                if ((unsigned char)*s < 32 || (unsigned char)*s == 127) {
                    sprintf(p, "\\x%02x", (unsigned char)*s);
                    p += 4;
                } else {
                    *p++ = *s;
                }
        }
    }
    *p = '\0';
    return buf;
}
