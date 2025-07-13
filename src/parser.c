#include "parser.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>

static void consume(Parser *this);
static Token peek(Parser *this);
static TokenData curr_token_data(Parser *this);
static TokenData next_token_data(Parser *this);

static void expect_next(Parser *this, Token expected);
static void add_statement(Program *prog, Stmt *stmt);

static Stmt *parse_return(Parser *this);
static Stmt *parse_expression_stmt(Parser *this);
static Expr *parse_expression(Parser *this, Precedence precedence);
static Expr *parse_prefix(Parser *this);
static Expr *parse_infix(Parser *this, Expr *left);
static Expr *parse_binary_expr(Parser *this, Expr *left, TokenData op_token);

static Precedence get_precedence(Token token);

Parser init_parser(Lexer *lexer) {
    Parser parser = {
        .lexer = lexer,
        .cur_tok = lexer->tokens[0].type,
        .next_tok = lexer->tokens[1].type,
        .next_tok_index = 1,
    };

    return parser;
}

Program *parse(Parser *this) {
    Program *prog = (Program *)s_malloc(sizeof(Program));
    prog->stmt_count = 0;
    prog->capacity = 8; // Start with reasonable initial capacity
    prog->statements = (Stmt **)s_malloc(prog->capacity * sizeof(Stmt *));

    while (this->cur_tok != tok_eof) {
        Stmt *stmt = NULL;
        switch (this->cur_tok) {
            case tok_func:
                // parse function declaration
                break;
            case tok_type:
                // parse variable declaration
                break;
            case tok_return:
                // parse return statement
                stmt = parse_return(this);
                break;
            default:
                stmt = parse_expression_stmt(this);
                break;
        }
        add_statement(prog, stmt);
    }

    return prog;
}

static void consume(Parser *this) {
    if (this->next_tok_index + 1 > this->lexer->token_count) {
        fprintf(stderr, "Unexpected end of input\n");
        exit(1);
    }
    this->cur_tok = this->next_tok;
    this->next_tok = this->lexer->tokens[++this->next_tok_index].type;
}

static Token peek(Parser *this) { return this->next_tok; }

static TokenData curr_token_data(Parser *this) { return this->lexer->tokens[this->next_tok_index - 1]; }

static TokenData next_token_data(Parser *this) { return this->lexer->tokens[this->next_tok_index]; }

static void expect_next(Parser *this, Token expected) {
    if (peek(this) != expected) {
        Location loc = next_token_data(this).loc;
        fprintf(stderr, "(%zu:%zu) Expected %s, got %s\n", loc.line, loc.col, token_to_string(expected),
                token_to_string(peek(this)));
        exit(1);
    }
}

static void add_statement(Program *prog, Stmt *stmt) {
    // Check if we need to resize the array
    if (prog->stmt_count >= prog->capacity) {
        // Double the capacity
        int new_capacity = prog->capacity * 2;
        prog->statements = (Stmt **)s_realloc(prog->statements, new_capacity * sizeof(Stmt *));
        prog->capacity = new_capacity;
    }
    
    prog->statements[prog->stmt_count++] = stmt;
}

static Stmt *parse_expression_stmt(Parser *this) {
    // parse expression
    Expr *expr = parse_expression(this, LOWEST);

    expect_next(this, tok_semi);  // expect a semicolon after the expression
    // consume the semicolon
    consume(this);
    consume(this);

    // create expression statement
    Stmt *stmt = (Stmt *)s_malloc(sizeof(Stmt));
    stmt->type = STMT_EXPR;
    stmt->expression_stmt.value = expr;

    return stmt;
}

static Stmt *parse_return(Parser *this) {
    // advance past return keyword
    consume(this);

    Expr *value = parse_expression(this, LOWEST);

    expect_next(this, tok_semi);  // expect a semicolon after the return value
    // consume the semicolon
    consume(this);
    consume(this);

    return return_stmt(value);
}

static Expr *parse_expression(Parser *this, Precedence precedence) {
    // TODO: handle errors
    Expr *left = parse_prefix(this);

    while (peek(this) != tok_semi && precedence < get_precedence(this->next_tok)) {
        left = parse_infix(this, left);
    }

    return left;
}

static Expr *parse_prefix(Parser *this) {
    switch (this->cur_tok) {
        case tok_number:
            return int_literal(curr_token_data(this).val);
        case tok_minus: {
            TokenData op = curr_token_data(this);
            consume(this);  // consume the '-' token
            return unary_expr(op, parse_expression(this, PREFIX));
        }
        case tok_not: {
            TokenData op = curr_token_data(this);
            consume(this);  // consume the '!' token
            return unary_expr(op, parse_expression(this, PREFIX));
        }
        case tok_lparen:
            consume(this);
            Expr *expr = parse_expression(this, LOWEST);
            expect_next(this, tok_rparen);  // error out if we don't have a closing parenthesis
            consume(this);                  // consume the closing parenthesis
            return expr;
        default:
            fprintf(stderr, "Unknown prefix token: %s\n", token_to_string(this->cur_tok));
            exit(1);
    }
}

static Expr *parse_infix(Parser *this, Expr *left) {
    // We need to make sure that the next token is an operator
    // which is why we switch off of this->next_tok
    switch (this->next_tok) {
        case tok_plus:
        case tok_minus:
        case tok_star:
        case tok_slash:
        case tok_mod:
        case tok_lessthan:
        case tok_greaterthan:
        case tok_equality:
        case tok_inequality: {
            TokenData op_token = next_token_data(this);
            // move past operator token completely
            consume(this);
            consume(this);
            return parse_binary_expr(this, left, op_token);
        }
        default:
            return NULL; // TODO: handle other infix expressions like function calls
    }
}

static Expr *parse_binary_expr(Parser *this, Expr *left, TokenData op_token) {
    Precedence curr_precedence = get_precedence(op_token.type);
    
    Expr *right = parse_expression(this, curr_precedence);
    return binary_expr(left, op_token, right);
}

Precedence get_precedence(Token token) {
    switch (token) {
        case tok_equality:
        case tok_inequality:
            return EQUALITY;
        case tok_lessthan:
        case tok_greaterthan:
            return LESS_GREATER;
        case tok_plus:
        case tok_minus:
            return ADD_SUBRACT;
        case tok_star:
        case tok_slash:
        case tok_mod:
            return TIMES_DIVIDE_MOD;
        case tok_not:  // Maybe remove this?
            return PREFIX;
        case tok_lparen:  // Function call
            return CALL;
        default:
            return LOWEST;
    }
}