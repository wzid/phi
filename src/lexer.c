#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

void add_token(Lexer *lexer, Token type, char *val);

int lex(Lexer *lexer) {
    switch (*lexer->cur_tok) {
        case ' ':
            // do nothing
            break;
        case '\n':
            lexer->line_number++;
            lexer->line_start = lexer->cur_tok + 1;
            break;
        case '\0':
            add_token(lexer, tok_eof, NULL);
            return 0;
        case '(':
            add_token(lexer, tok_lparen, "(");
            break;
        case ')':
            add_token(lexer, tok_rparen, ")");
            break;
        case ',':
            add_token(lexer, tok_comma, ",");
            break;
        case '+':
            add_token(lexer, tok_plus, "+");
            break;
        case '-':
            add_token(lexer, tok_minus, "-");
            break;
        case '*':
            add_token(lexer, tok_star, "*");
            break;
        case '/':
            add_token(lexer, tok_slash, "/");
            break;
        case '=':
            add_token(lexer, tok_equal, "=");
            break;
        case '<':
            add_token(lexer, tok_lessthan, "<");
            break;
        case '>':
            add_token(lexer, tok_greaterthan, ">");
            break;
        case '.':
            add_token(lexer, tok_period, ".");
            break;
        case ';':
            add_token(lexer, tok_semicolon, ";");
            break;
        case '&':
            add_token(lexer, tok_and, "&");
            break;
        case '|':
            add_token(lexer, tok_or, "|");
            break;
        case '!':
            add_token(lexer, tok_not, "!");
            break;
        case '^':
            add_token(lexer, tok_xor, "^");
            break;
        case '%':
            add_token(lexer, tok_mod, "%");
            break;
        default:
            // If the token is a number
            if (isdigit(*lexer->cur_tok)) {
                char *str_start = lexer->cur_tok;

                while (isdigit(*lexer->cur_tok)) 
                    lexer->cur_tok++;
                
                int str_len = lexer->cur_tok - str_start;
                char *number = (char *)malloc(str_len + 1);

                strncpy(number, str_start, str_len);
                number[str_len] = '\0';

                add_token(lexer, tok_number, number);
            } else if (isalpha(*lexer->cur_tok)) {
                char *str_start = lexer->cur_tok;

                // everything except the first character can be a number
                while (isalnum(*lexer->cur_tok))
                    lexer->cur_tok++;
                
                int str_len = lexer->cur_tok - str_start;
                char *identifier = (char *)malloc(str_len + 1);

                strncpy(identifier, str_start, str_len);
                identifier[str_len] = '\0';

                add_token(lexer, tok_identifier, identifier);
            } else {
                fprintf(stderr, "Unknown token: %c\n", *lexer->cur_tok);
                return 1;
            }
            break;
    }
}

void add_token(Lexer *lexer, Token type, char *val) {
    if (lexer->token_count == lexer->capacity) {
        size_t new_capacity = lexer->capacity == 0 ? 8 : lexer->capacity * 2;
        lexer->tokens = (TokenData *)realloc(lexer->tokens, lexer->capacity * sizeof(TokenData));
        if (!lexer->tokens) {
            perror("Error reallocating memory");
            exit(1);
        }

        lexer->capacity = new_capacity;
    }

    lexer->tokens[lexer->token_count].type = type;
    lexer->tokens[lexer->token_count].val = val;
    // i dont like this code
    lexer->tokens[lexer->token_count].loc.line = lexer->line_number;
    lexer->tokens[lexer->token_count].loc.col = lexer->cur_tok - lexer->line_start;
    lexer->token_count++;
}

// Free the memory allocated for the lexer
void free_lexer(Lexer *lexer) {
    for (size_t i = 0; i < lexer->token_count; i++) {
        free(lexer->tokens[i].val);
    }
    free(lexer->tokens);
    lexer->tokens = NULL;
    lexer->token_count = 0;
    lexer->line_number = 0;
    lexer->capacity = 0;
    free(lexer->start_tok);
    lexer->cur_tok = NULL;
}