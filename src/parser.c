#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

static void consume(Parser *parser);
static Token peek(Parser *parser);
// static void expect(Parser *parser, Token check, Token expected);
static void add_statement(Program *prog, Stmt *stmt);

static Stmt *parse_return(Parser *parser);
static Expr *parse_expression(Parser *parser);
static Expr *parse_prefix(Parser *parser);
// static Expr *parse_infix(Parser *parser, Expr *left);

Parser init_parser(Lexer *lexer) {
    Parser parser = {
        .lexer = lexer,
        .next_tok_index = 0,
    };
    
    // Read 2 tokens so that cur_tok and next_tok are correctly set
    consume(&parser);
    consume(&parser);
    
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
        consume(parser);
    }
    // TEMPORARY!!
    return NULL;
}

static void consume(Parser *parser) {
    parser->cur_tok = parser->next_tok;
    parser->next_tok = parser->lexer->tokens[parser->next_tok_index++].type;
}

static Token peek(Parser *parser) {
    return parser->next_tok;
}

// static void expect(Parser *parser, Token check, Token expected) {
//     if (check != expected) {
//         fprintf(stderr, "Expected %s, got %s\n", token_to_string(expected), token_to_string(check));
//         exit(1);
//     }
//     consume(parser);
// }

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
    consume(parser);

    // parse expression
    Expr *value = parse_expression(parser);

    // create return statement
    return return_stmt(value);
}

static Expr *parse_expression(Parser *parser) {
    // Expr *expr = (Expr *) malloc(sizeof(Expr));

    Expr *left = parse_prefix(parser);

    if (peek(parser) == tok_semi) {
        return left;
    }
    
    // TODO: parse infix
    // TEMPORARY
    return NULL;
}

static Expr *parse_prefix(Parser *parser) {
    switch (parser->cur_tok) {
        case tok_number:
            return int_literal(parser->lexer->tokens[parser->next_tok_index - 1].val);
        // case tok_string:
        //     return string_literal(parser->lexer->tokens[parser->next_tok_index - 1].str_val);
        // case tok_bool:
        //     return bool_literal(parser->lexer->tokens[parser->next_tok_index - 1].bool_val);
        // case tok_identifier:
        //     return NULL;
        // case tok_lparen:
        //     consume(parser);
        //     Expr *expr = parse_expression(parser);
        //     expect(parser, parser->cur_tok, tok_rparen);
        //     return expr;
        // case tok_minus:
        //     consume(parser);
        //     return unary_expr(parser->cur_tok, parse_expression(parser));
        default:
            fprintf(stderr, "Unknown token: %s\n", token_to_string(parser->cur_tok));
            exit(1);
    }
}

