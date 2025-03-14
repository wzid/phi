#pragma once

typedef struct lexer {
    char *start_tok;
    char *cur_tok;
    TokenData *tokens;
    size_t line_number;
    char *line_start;
    size_t token_count;
    size_t capacity;
} Lexer;

typedef enum token {
    tok_eof = '\0',
    tok_lparen = '(',
    tok_rparen = ')',
    tok_comma = ',',
    tok_plus = '+',
    tok_minus = '-',
    tok_star = '*',
    tok_slash = '/',
    tok_equal = '=',
    tok_lessthan = '<',
    tok_greaterthan = '>',
    tok_period = '.',
    tok_semicolon = ';',
    tok_and = '&',
    tok_or = '|',
    tok_not = '!',
    tok_xor = '^',
    tok_mod = '%',
    tok_identifier,
    tok_number
} Token;

typedef struct location {
    size_t line;
    size_t col;
} Location;

typedef struct token_data {
    Token type; // The type of the token
    char *val; // The value of the token
    Location loc; // The location of the token
} TokenData;

int lex(Lexer *lexer);

void free_lexer(Lexer *lexer);