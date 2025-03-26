#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer *lexer;
    Token cur_tok;
    Token next_tok;
    int next_tok_index; // technically this is the index of the next token
} Parser;

Parser init_parser(Lexer *lexer);

Program *parse(Parser *parser);

#endif
