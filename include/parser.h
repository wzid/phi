#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer *lexer;
    Token cur_tok;
    Token next_tok;
    int next_tok_index;
} Parser;

typedef enum {
    LOWEST,
    EQUALITY,    // == or !=
    LESS_GREATER, // < or >
    ADD_SUBRACT, // + or -
    TIMES_DIVIDE_MOD, // * or /
    PREFIX,      // -X or !X
    CALL,        // myFunction(X)
} Precedence;

Parser init_parser(Lexer *lexer);

Program *parse(Parser *this);

#endif
