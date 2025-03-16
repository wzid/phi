#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

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
    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        perror("Error allocating memory");
        fclose(file);
        return 1;
    }
    
    // read the file into the buffer
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0'; // Null-terminate the string
    fclose(file);

    Lexer lexer = {
        .start_tok = buffer, 
        .cur_tok = buffer, 
        .line_start = buffer,
    };

    // Call the lex function (to be implemented)
    int result = lex(&lexer);

    if (result == 0) {
        printf("Lexing successful\n");
    } else {
        printf("Lexing failed\n");
    }

    // print lexer tokens to file
    FILE *out_file = fopen("tokens.txt", "w");
    if (!out_file) {
        perror("Error opening file");
        return 1;
    }

    for (size_t i = 0; i < lexer.token_count; i++) {
        TokenData token = lexer.tokens[i];
        fprintf(out_file, "Token: %d, Value: %s, Line: %zu, Column: %zu\n", token.type, token.val, token.loc.line, token.loc.col);
    }

    fclose(out_file);
    
    // Free the buffer
    free(buffer);
    free_lexer(&lexer);
    return 0;
}