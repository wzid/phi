#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

static void add_token(Lexer *lexer, Token type, char *val);
static void handle_identifier(Lexer *lexer);
static char next_char(Lexer *lexer);

int lex(Lexer *lexer) {
    while (*lexer->cur_tok != '\0') {
        switch (*lexer->cur_tok) {
            case ' ':
                // do nothing
                break;
            case '\n':
                lexer->line_number++;
                lexer->line_start = lexer->cur_tok + 1;
                break;
            case '(':
                add_token(lexer, tok_lparen, "(");
                break;
            case ')':
                add_token(lexer, tok_rparen, ")");
                break;
            case '{':
                add_token(lexer, tok_lbrace, "{");
                break;
            case '}':
                add_token(lexer, tok_rbrace, "}");
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
                if (next_char(lexer) == '/') {
                    // skip the comment (and don't go out of bounds)
                    while (*lexer->cur_tok != '\n' && *lexer->cur_tok != '\0') lexer->cur_tok++;
                    continue;
                } else {
                    add_token(lexer, tok_slash, "/");
                }
                break;
            case '=':
                if (next_char(lexer) == '=') {
                    lexer->cur_tok++; // skip the next '='
                    add_token(lexer, tok_equality, "==");
                } else {
                    add_token(lexer, tok_equal, "=");
                }
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
                add_token(lexer, tok_semi, ";");
                break;
            case '&':
                add_token(lexer, tok_and, "&");
                break;
            case ':':
                add_token(lexer, tok_colon, ":");
                break;
            case '|':
                add_token(lexer, tok_or, "|");
                break;
            case '!':
                if (next_char(lexer) == '=') {
                    lexer->cur_tok++; // skip the next '='
                    add_token(lexer, tok_inequality, "!=");
                } else {
                    // If it's just '!', we treat it as a logical NOT
                    add_token(lexer, tok_not, "!");
                }
                break;
            case '^':
                add_token(lexer, tok_xor, "^");
                break;
            case '%':
                add_token(lexer, tok_mod, "%");
                break;
            default:
                if (*lexer->cur_tok == '"') {
                    lexer->cur_tok++;
                    char *str_start = lexer->cur_tok;

                    while (*lexer->cur_tok != '"' && *lexer->cur_tok != '\0') lexer->cur_tok++;
                    
                    // Inform user of missing string closing
                    if (*lexer->cur_tok == '\0') {
                        fprintf(stderr, "%zu: Missing string closing\n", lexer->line_number);
                        return 1;
                    }
                    

                    int str_len = lexer->cur_tok - str_start;
                    char *string = (char *)s_malloc(str_len + 1);

                    strncpy(string, str_start, str_len);
                    string[str_len] = '\0';

                    add_token(lexer, tok_string, string);
                    // we don't continue here so we can skip the closing quote
                } else if (isdigit(*lexer->cur_tok)) {  // If the token is a number
                    char *str_start = lexer->cur_tok;

                    while (isdigit(*lexer->cur_tok)) lexer->cur_tok++;

                    int str_len = lexer->cur_tok - str_start;
                    char *number = (char *)s_malloc(str_len + 1);

                    strncpy(number, str_start, str_len);
                    number[str_len] = '\0';

                    add_token(lexer, tok_number, number);

                    // we continue here because we are already at the next token 
                    // which is the result of the while loop
                    continue;
                } else if (isalpha(*lexer->cur_tok)) {
                    handle_identifier(lexer);
                    // we continue here because we are already at the next token 
                    // which is the result of the while loop
                    continue;
                } else {
                    fprintf(stderr, "Unknown token: %c\n", *lexer->cur_tok);
                    return 1;
                }
                break;
        }

        // advance token
        lexer->cur_tok++;
    }

    add_token(lexer, tok_eof, NULL);
    return 0;
}

static void add_token(Lexer *lexer, Token type, char *val) {
    if (lexer->token_count == (int) lexer->capacity) {
        size_t new_capacity = lexer->capacity == 0 ? 8 : lexer->capacity * 2;
        lexer->tokens = (TokenData *)s_realloc(lexer->tokens, new_capacity * sizeof(TokenData));
        if (!lexer->tokens) {
            perror("Error reallocating memory");
            exit(1);
        }

        lexer->capacity = new_capacity;
    }

    lexer->tokens[lexer->token_count].type = type;
    lexer->tokens[lexer->token_count].val = val;

    // setting the location
    lexer->tokens[lexer->token_count].loc.line = lexer->line_number;
    lexer->tokens[lexer->token_count].loc.col = lexer->cur_tok - lexer->line_start;

    lexer->token_count++;
}

static void handle_identifier(Lexer *lexer) {
    char *str_start = lexer->cur_tok;

    // everything except the first character can be a number
    while (isalnum(*lexer->cur_tok) || *lexer->cur_tok == '_') lexer->cur_tok++;

    int str_len = lexer->cur_tok - str_start;
    char *identifier = (char *)s_malloc(str_len + 1);

    strncpy(identifier, str_start, str_len);
    identifier[str_len] = '\0';

    if (!strcmp(identifier, "func")) {
        add_token(lexer, tok_func, identifier);
    } else if (!strcmp(identifier, "int") || !strcmp(identifier, "string") || !strcmp(identifier, "bool")) {
        add_token(lexer, tok_type, identifier);
    } else if (!strcmp(identifier, "return")) {
        add_token(lexer, tok_return, identifier);
    } else {
        add_token(lexer, tok_identifier, identifier);
    }
}

static char next_char(Lexer *lexer) {
    if (*(lexer->cur_tok + 1) == '\0') {
        return '\0'; // end of string
    }
    return *(lexer->cur_tok + 1);
}

// Free the memory allocated for the lexer
void free_lexer(Lexer *lexer) {
    for (int i = 0; i < lexer->token_count; i++) {
        if (lexer->tokens[i].type == tok_identifier || lexer->tokens[i].type == tok_string ||
            lexer->tokens[i].type == tok_number || lexer->tokens[i].type == tok_type || 
            lexer->tokens[i].type == tok_return || lexer->tokens[i].type == tok_func) {
            s_free(lexer->tokens[i].val);
        }
    }
    s_free(lexer->tokens);
}

char *token_to_string(Token token) {
    switch (token) {
        case tok_eof: return "TOK_EOF";
        case tok_lparen: return "TOK_LPAREN";
        case tok_rparen: return "TOK_RPAREN";
        case tok_lbrace: return "TOK_LBRACE";
        case tok_rbrace: return "TOK_RBRACE";
        case tok_comma: return "TOK_COMMA";
        case tok_plus: return "TOK_PLUS";
        case tok_minus: return "TOK_MINUS";
        case tok_star: return "TOK_STAR";
        case tok_slash: return "TOK_SLASH";
        case tok_equal: return "TOK_EQUAL";
        case tok_equality: return "TOK_EQUALITY";
        case tok_inequality: return "TOK_INEQUALITY";
        case tok_lessthan: return "TOK_LESSTHAN";
        case tok_greaterthan: return "TOK_GREATERTHAN";
        case tok_period: return "TOK_PERIOD";
        case tok_semi: return "TOK_SEMI";
        case tok_and: return "TOK_AND";
        case tok_or: return "TOK_OR";
        case tok_not: return "TOK_NOT";
        case tok_xor: return "TOK_XOR";
        case tok_mod: return "TOK_MOD";
        case tok_func: return "TOK_FUNC";
        case tok_return: return "TOK_RETURN";
        case tok_type: return "TOK_TYPE";
        case tok_string: return "TOK_STRING";
        case tok_identifier: return "TOK_IDENTIFIER";
        case tok_number: return "TOK_NUMBER";
        case tok_colon: return "TOK_COLON";
        default: return "TOK_UNKNOWN";
    }
}