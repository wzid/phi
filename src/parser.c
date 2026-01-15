#include "parser.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>

#define true 1
#define false 0

static void consume(Parser *this);
static Token peek(Parser *this);
static TokenData curr_token_data(Parser *this);
static TokenData next_token_data(Parser *this);

static void expect_next(Parser *this, Token expected);
static void expect_next_and_consume_current(Parser *this, Token expected);
static void add_statement(Program *prog, Stmt *stmt);

static Stmt **parse_block_statements(Parser *this, int *out_stmt_count);
static Stmt *parse_func_decl(Parser *this);
static Stmt *parse_global_var_decl(Parser *this);
static Stmt *parse_var_decl(Parser *this);
static Stmt *parse_var_assign(Parser *this);
static Stmt *parse_return(Parser *this);
static Stmt *parse_expression_stmt(Parser *this);
static Stmt *parse_if_stmt(Parser *this);
static Stmt *parse_while_stmt(Parser *this);

static void parse_function_parameters(Parser *this, TokenData** out_parameter_names, TokenData** out_parameter_types, int *out_param_count);

static Expr *parse_increment_expr(Parser *this, int is_prefix);
static Expr **parse_function_arguments(Parser *this, int *out_arg_count);
static Expr *parse_function_call(Parser *this);
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

// This function will parse the entire program
// The only top level things this function will parse are function declarations and variable declarations
Program *parse(Parser *this) {
    Program *prog = (Program *)s_malloc(sizeof(Program));
    prog->stmt_count = 0;
    prog->capacity = 8; // Start with reasonable initial capacity
    prog->statements = (Stmt **)s_malloc(prog->capacity * sizeof(Stmt *));

    while (this->cur_tok != tok_eof) {
        Stmt *stmt = NULL;
        switch (this->cur_tok) {
            case tok_func:
                stmt = parse_func_decl(this);
                break;
            case tok_type:
                stmt = parse_global_var_decl(this);
                break;
            default:
                // throw an error
                fprintf(stderr, "Unexpected token at top level: %s\n", token_to_string(this->cur_tok));
                exit(1);
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

static void expect_next_and_consume_current(Parser *this, Token expected) {
    expect_next(this, expected);
    consume(this);
}

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

    // expect a semicolon after the expression
    expect_next_and_consume_current(this, tok_semi);
    consume(this);

    return expression_stmt(expr);
}

static Stmt* parse_if_stmt(Parser *this) {
    consume(this); // consume current token, if
    consume(this); // consume current token, (
    Expr* condition = parse_expression(this, LOWEST);
    expect_next_and_consume_current(this, tok_rparen);
    expect_next_and_consume_current(this, tok_lbrace);
    consume(this); // consume current token, {
    
    int then_stmt_count = 0;
    Stmt** then_stmts = parse_block_statements(this, &then_stmt_count);
    
    Stmt *then_block = block_stmt(then_stmts, then_stmt_count);
    
    // is this needed? got from the parse func decl
    if (this->cur_tok != tok_rbrace) {
        Location loc = curr_token_data(this).loc;
        fprintf(stderr, "(%zu:%zu) Expected closing '}' for if statement, got %s\n", loc.line, loc.col,
                token_to_string(this->cur_tok));
        exit(1);
    }

    consume(this); // consume current token, }

    // early exit if we don't have an else branch
    if (this->cur_tok != tok_else) {
        return if_stmt(condition, then_block, 0, NULL, NULL, NULL);
    }

    // Store the else-if condition and branch
    int max_else_if = 8;
    Expr** else_if_conditions = s_malloc(max_else_if * sizeof(Expr*));
    Stmt** else_if_branches = s_malloc(max_else_if * sizeof(Stmt*));
    int else_if_count = 0;

    // else if branches
    while (peek(this) == tok_if) {
        consume(this); // consume current token, else
        expect_next_and_consume_current(this, tok_lparen);
        Expr* else_if_condition = parse_expression(this, LOWEST);
        expect_next_and_consume_current(this, tok_lbrace);
        consume(this); // consume current token, {

        int else_if_stmt_count = 0;
        Stmt** else_if_stmts = parse_block_statements(this, &else_if_stmt_count);
        Stmt *else_if_block = block_stmt(else_if_stmts, else_if_stmt_count);

        if (this->cur_tok != tok_rbrace) {
            Location loc = curr_token_data(this).loc;
            fprintf(stderr, "(%zu:%zu) Expected closing '}' for else-if statement, got %s\n", loc.line, loc.col,
                    token_to_string(this->cur_tok));
            exit(1);
        }

        consume(this); // consume current token, }

        // Resize arrays if needed
        if (else_if_count >= max_else_if) {
            max_else_if *= 2;
            else_if_conditions = s_realloc(else_if_conditions, max_else_if * sizeof(Expr*));
            else_if_branches = s_realloc(else_if_branches, max_else_if * sizeof(Stmt*));
        }

        else_if_conditions[else_if_count] = else_if_condition;
        else_if_branches[else_if_count] = else_if_block;
        else_if_count++;
    }

    if (else_if_count == 0) {
        s_free(else_if_conditions);
        s_free(else_if_branches);
    }

    if (this->cur_tok != tok_else && else_if_count > 0) {
        return if_stmt(condition, then_block, else_if_count, else_if_conditions, else_if_branches, NULL);

        // it is impossible to get this case below because we checked above that an else branch exists at least
        // even though we are checking to see if the current token is else and the else if count is > 0
        // We could simplify it to just check if the cur token is not else because that would mean that there exists else ifs
        // return if_stmt(condition, then_block, 0, NULL, NULL, NULL);
    }

    // cur token = tok_else
    expect_next_and_consume_current(this, tok_lbrace);
    consume(this); // consume current token, {

    int else_stmt_count = 0;
    Stmt** else_stmts = parse_block_statements(this, &else_stmt_count);
    Stmt *else_block = block_stmt(else_stmts, else_stmt_count);

    // is this needed? got from the parse func decl
    if (this->cur_tok != tok_rbrace) {
        Location loc = curr_token_data(this).loc;
        fprintf(stderr, "(%zu:%zu) Expected closing '}' for if-else statement, got %s\n", loc.line, loc.col,
                token_to_string(this->cur_tok));
        exit(1);
    }

    consume(this); // consume current token, }

    if (else_if_count > 0) {
        return if_stmt(condition, then_block, else_if_count, else_if_conditions, else_if_branches, else_block);
    } else {
        return if_stmt(condition, then_block, 0, NULL, NULL, else_block);
    }
}

static Stmt *parse_while_stmt(Parser *this) {
    expect_next_and_consume_current(this, tok_lparen);
    consume(this); // consume current token, (
    Expr* condition = parse_expression(this, LOWEST);
    expect_next_and_consume_current(this, tok_rparen);
    expect_next_and_consume_current(this, tok_lbrace);
    consume(this); // consume current token, {

    int body_stmt_count = 0;
    Stmt** body_stmts = parse_block_statements(this, &body_stmt_count);
    Stmt *body_block = block_stmt(body_stmts, body_stmt_count);

    // is this needed? got from the parse func decl
    if (this->cur_tok != tok_rbrace) {
        Location loc = curr_token_data(this).loc;
        fprintf(stderr, "(%zu:%zu) Expected closing '}' for while statement, got %s\n", loc.line, loc.col,
                token_to_string(this->cur_tok));
        exit(1);
    }
    consume(this); // consume current token, }

    return while_stmt(condition, body_block);
}

/**
 * @param this The parser instance.
 * @param out_parameter_names Pointer to array of TokenData to hold parameter names.
 * @param out_parameter_types Pointer to array of TokenData to hold parameter types.
 * @param out_param_count Pointer to int to hold number of parameters parsed.
 */
static void parse_function_parameters(Parser *this, TokenData** out_parameter_names, TokenData** out_parameter_types, int *out_param_count) {
    int capacity = 4; // initial capacity
    *out_parameter_names = (TokenData *)s_malloc(capacity * sizeof(TokenData));
    *out_parameter_types = (TokenData *)s_malloc(capacity * sizeof(TokenData));
    int param_count = 0;

    // parameters are formed as: name:type, name:type, ...
    while (this->cur_tok != tok_rparen && this->cur_tok != tok_eof) {
        TokenData param_name = curr_token_data(this);
        expect_next_and_consume_current(this, tok_colon);
        expect_next_and_consume_current(this, tok_type);
        TokenData param_type = curr_token_data(this);

        // Add parameter to the array
        if (param_count >= capacity) {
            capacity *= 2;
            *out_parameter_names = (TokenData *)s_realloc(*out_parameter_names, capacity * sizeof(TokenData));
            *out_parameter_types = (TokenData *)s_realloc(*out_parameter_types, capacity * sizeof(TokenData));
        }
        (*out_parameter_names)[param_count] = param_name;
        (*out_parameter_types)[param_count] = param_type;
        param_count++;
        if (peek(this) == tok_comma) {
            consume(this); // consume comma
            consume(this); // move to next parameter
        } else {
            break; // no more parameters
        }
    }

    *out_param_count = param_count;
}

static Stmt *parse_func_decl(Parser *this) {
    // functions are formed as: func name(param1:type1, param2:type2, ...):return_type { body }
    // advance past 'func' keyword
    expect_next_and_consume_current(this, tok_identifier);
    TokenData func_name = curr_token_data(this);
    expect_next_and_consume_current(this, tok_lparen);

    // Need to gather the parameter names and types!
    TokenData *parameter_names = NULL;
    TokenData *parameter_types = NULL;
    int parameter_count = 0;
    
    // no parameters
    if (peek(this) == tok_rparen) {
        expect_next_and_consume_current(this, tok_rparen);
    } else {
        consume(this); // move to first parameter
        // modifies the parameter arrays and count
        parse_function_parameters(this, &parameter_names, &parameter_types, &parameter_count);
        expect_next_and_consume_current(this, tok_rparen);
    }

    TokenData return_type = {0};
    if (peek(this) == tok_colon) {
        consume(this); // consume return type token
        expect_next_and_consume_current(this, tok_type);
        return_type = curr_token_data(this);
    }

    // implicit return function
    // the syntax is func name(params):return_type => expression;
    if (peek(this) == tok_arrow) {
        expect_next_and_consume_current(this, tok_arrow);
        consume(this); // consume '=>' token
        Expr *return_expr = parse_expression(this, LOWEST);
        expect_next_and_consume_current(this, tok_semi);
        consume(this); // consume ';' token

        // create function body with return statement
        Stmt **statements = (Stmt **)s_malloc(sizeof(Stmt *));
        statements[0] = return_stmt(return_expr);
        int stmt_count = 1;
        Stmt *body = block_stmt(statements, stmt_count);
        
        return func_decl_stmt(func_name, body, return_type, parameter_names, parameter_types, parameter_count);
    }

    expect_next_and_consume_current(this, tok_lbrace);
    consume(this); // move past '{' token

    int stmt_count = 0;
    // this is the body of the function
    Stmt **statements = parse_block_statements(this, &stmt_count);
    // Create block statement for function body
    Stmt *body = block_stmt(statements, stmt_count);

    if (this->cur_tok != tok_rbrace) {
        Location loc = curr_token_data(this).loc;
        fprintf(stderr, "(%zu:%zu) Expected closing '}' for function body, got %s\n", loc.line, loc.col,
                token_to_string(this->cur_tok));
        exit(1);
    }

    consume(this); // move past '}' token


    return func_decl_stmt(func_name, body, return_type, parameter_names, parameter_types, parameter_count);
}

static Stmt **parse_block_statements(Parser *this, int *out_stmt_count) {
    int capacity = 8; // initial capacity
    Stmt **statements = (Stmt **)s_malloc(capacity * sizeof(Stmt *));
    int stmt_count = 0;

    while (this->cur_tok != tok_rbrace && this->cur_tok != tok_eof) {
        Stmt *stmt = NULL;
        switch (this->cur_tok) {
            case tok_type:
                stmt = parse_var_decl(this);
                break;
            case tok_return:
                stmt = parse_return(this);
                break;
            case tok_identifier:
                if (peek(this) == tok_lparen) {
                    stmt = parse_expression_stmt(this);
                } else if (peek(this) == tok_increment || peek(this) == tok_decrement) {
                    // postfix operator
                    // x++ or x--
                    stmt = expression_stmt(parse_increment_expr(this, false));
                } else {
                    stmt = parse_var_assign(this);
                }
                break;
            case tok_if:
                stmt = parse_if_stmt(this);
                break;
            case tok_while:
                stmt = parse_while_stmt(this);
                break;
            // prefix increment/decrement operators
            case tok_increment:
            case tok_decrement:
            // --x or ++x
                stmt = expression_stmt(parse_increment_expr(this, true));
                break;
            default:
                stmt = parse_expression_stmt(this);
                break;
        }

        // Add statement to the array
        if (stmt_count >= capacity) {
            capacity *= 2;
            statements = (Stmt **)s_realloc(statements, capacity * sizeof(Stmt *));
        }
        statements[stmt_count++] = stmt;
    }

    *out_stmt_count = stmt_count;
    return statements;
}

static Stmt *parse_var_decl(Parser *this) {
    TokenData type = curr_token_data(this);
    // variable declarations are formed as: type identifier = expression;
    // advance past 'int' keyword
    consume(this);

    TokenData identifier = curr_token_data(this);
    expect_next_and_consume_current(this, tok_equal);
    consume(this);  // move past '=' token

    Expr *value = parse_expression(this, LOWEST);

    expect_next_and_consume_current(this, tok_semi);  // expect a semicolon after the expression
    
    // consume the semicolon
    consume(this);

    return var_decl_stmt(type, identifier, value);
}

static Stmt *parse_global_var_decl(Parser *this) {
    Stmt *var_decl = parse_var_decl(this);
    // change the statement type to global variable declaration
    // this also should work because they have the same struct layout as regular var decls
    var_decl->type = STMT_GLOBAL_VAR_DECL;
    return var_decl;
}

static Stmt *parse_var_assign(Parser *this) {
    TokenData identifier = curr_token_data(this);
    consume(this);

    // if it is not an assignment operator, throw an error
    if (this->cur_tok != tok_equal && this->cur_tok != tok_plus_equal && this->cur_tok != tok_minus_equal &&
        this->cur_tok != tok_star_equal && this->cur_tok != tok_slash_equal) {
        Location loc = next_token_data(this).loc;
        fprintf(stderr, "(%zu:%zu) Expected assignment operator after identifier, got %s\n", loc.line, loc.col,
                token_to_string(this->next_tok));
        exit(1);
    }

    TokenData modifying_tok = curr_token_data(this);
    consume(this); // consume the assignment operator

    Expr *new_value = parse_expression(this, LOWEST);

    expect_next_and_consume_current(this, tok_semi);  // expect a semicolon after the expression
    consume(this); // consume the semicolon

    return var_assign_stmt(identifier, modifying_tok, new_value);
}

static Stmt *parse_return(Parser *this) {
    // return statements are formed as: return expression;
    // advance past return keyword
    consume(this);

    Expr *value = parse_expression(this, LOWEST);

    expect_next_and_consume_current(this, tok_semi);  // expect a semicolon after the return value
    consume(this); // move past ';' token

    return return_stmt(value);
}

static Expr **parse_function_arguments(Parser *this, int *out_arg_count) {
    int capacity = 4; // initial capacity
    Expr **args = (Expr **)s_malloc(capacity * sizeof(Expr *));
    int arg_count = 0;

    while (this->cur_tok != tok_rparen && this->cur_tok != tok_eof) {
        Expr *arg = parse_expression(this, LOWEST);
        
        if (arg_count >= capacity) {
            capacity *= 2;
            args = (Expr **)s_realloc(args, capacity * sizeof(Expr *));
        }
        
        // Add argument to the array
        args[arg_count++] = arg;

        if (peek(this) == tok_comma) {
            consume(this); // consume current
            consume(this); // consume comma and move to next argument
        } else {
            break; // no more arguments
        }
    }

    *out_arg_count = arg_count;
    return args;
}

static Expr *parse_function_call(Parser *this) {
    TokenData func_name = curr_token_data(this);
    expect_next_and_consume_current(this, tok_lparen);
    
    // Check for no arguments
    if (peek(this) == tok_rparen) {
        // no arguments
        expect_next_and_consume_current(this, tok_rparen);
        return func_call(func_name, NULL, 0);
    } else {
        consume(this); // move to first argument
        int arg_count = 0;
        Expr **args = parse_function_arguments(this, &arg_count);
        expect_next_and_consume_current(this, tok_rparen);
        return func_call(func_name, args, arg_count);
    }
}

static Expr *parse_increment_expr(Parser *this, int is_prefix) {
    TokenData op_token = is_prefix ? curr_token_data(this) : next_token_data(this);
    TokenData identifier = is_prefix ? next_token_data(this) : curr_token_data(this);
    consume(this); // consume current token so that we move past the operator/identifier and the next_tok is another expression
    return increment_expr(op_token, identifier, is_prefix);
}

static Expr *parse_expression(Parser *this, Precedence precedence) {
    Expr *left = parse_prefix(this);

    while (peek(this) != tok_semi && precedence < get_precedence(this->next_tok)) {
        left = parse_infix(this, left);
    }

    return left;
}

static Expr *parse_prefix(Parser *this) {
    switch (this->cur_tok) {
        case tok_identifier:
            return identifier_expr(curr_token_data(this));
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
        case tok_increment:
        case tok_decrement:
            return parse_increment_expr(this, true);
        case tok_string:
            return string_literal(curr_token_data(this).val);
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
        case tok_lessthan_equal:
        case tok_greaterthan_equal:
        case tok_equality:
        case tok_inequality: {
            TokenData op_token = next_token_data(this);
            // move past operator token completely
            consume(this);
            consume(this);
            return parse_binary_expr(this, left, op_token);
        }
        // the left expression is just an identifier expr which 
        // we don't need because we only need to get the function name
        // and we can get that using the curr_token_data function
        case tok_lparen:
            return parse_function_call(this);
        case tok_increment:
        case tok_decrement:
            if (this->cur_tok != tok_identifier) {
                fprintf(stderr, "Expected identifier before increment/decrement operator\n");
                exit(1);
            }
            return parse_increment_expr(this, 0);
        default:
            return NULL; 
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
        case tok_lessthan_equal:
        case tok_greaterthan_equal:
            return LESS_GREATER;
        case tok_plus:
        case tok_minus:
            return ADD_SUBRACT;
        case tok_star:
        case tok_slash:
        case tok_mod:
            return TIMES_DIVIDE_MOD;
        case tok_not:  // Maybe remove this?
        case tok_increment:
        case tok_decrement:
            return PREFIX;
        case tok_lparen:  // Function call
            return CALL;
        default:
            return LOWEST;
    }
}