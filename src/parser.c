#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

static void advance_tokens(Parser *parser);
static void add_statement(Program *prog, Stmt *stmt);

static Stmt *parse_return(Parser *parser);
static Expr *parse_expression(Parser *parser);

Parser init_parser(Lexer *lexer) {
    Parser parser = {
        .lexer = lexer,
        .next_tok_index = 0,
    };
    
    // Read 2 tokens so that cur_tok and next_tok are correctly set
    advance_tokens(&parser);
    advance_tokens(&parser);
    
    return parser;
}

Program *parse(Parser *parser) {
    // Stmt **stmts = (Stmt **) malloc(sizeof(Stmt *));
    Program *prog = (Program *) malloc(sizeof(Program));
    // prog->statements = (Stmt **) malloc(sizeof(Stmt *));
    prog->stmt_count = 0;

    while (parser->cur_tok != tok_eof) {
        Stmt *stmt = NULL;
        switch (parser->cur_tok) {
            case tok_func:
                // parse function declaration
                break;
            case tok_type:
                // parse variable declaration
                break;
            case tok_return:
                // parse return statement
                stmt = parse_return(parser);
                break;
            case tok_identifier:
                // parse expression
                break;
            default:
                fprintf(stderr, "Unknown token: %s\n", token_to_string(parser->cur_tok));
                return NULL;
        }
        add_statement(prog, stmt);
        // prog->statements[prog->stmt_count++] = stmt;
        advance_tokens(parser);
    }
}

static void advance_tokens(Parser *parser) {
    parser->cur_tok = parser->next_tok;
    parser->next_tok = parser->lexer->tokens[parser->next_tok_index++].type;
}

static void add_statement(Program *prog, Stmt *stmt) {
    if (prog->stmt_count == 0) {
        prog->statements = (Stmt **) malloc(sizeof(Stmt *));
    } else {
        // this seems inefficient
        prog->statements = (Stmt **) realloc(prog->statements, (prog->stmt_count + 1) * sizeof(Stmt *));
    }
    prog->statements[prog->stmt_count++] = stmt;
}

static Stmt *parse_return(Parser *parser) {
    // advance past return keyword
    advance_tokens(parser);

    // parse expression
    Expr *value = parse_expression(parser);

    // create return statement
    return return_stmt(value);
}

static Expr *parse_expression(Parser *parser) {
    Expr *expr = (Expr *) malloc(sizeof(Expr));
    // TODO:
}