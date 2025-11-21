#include "ast.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Expr *binary_expr(Expr *left, TokenData op, Expr *right) {
    Expr *expr = (Expr *)s_malloc(sizeof(Expr));

    expr->type = EXPR_BINARY;
    expr->binary.left = left;
    expr->binary.op_token = op;
    expr->binary.right = right;

    return expr;
}

Expr *unary_expr(TokenData op, Expr *right) {
    Expr *expr = (Expr *)s_malloc(sizeof(Expr));

    expr->type = EXPR_UNARY;
    expr->unary.op_token = op;
    expr->unary.right = right;

    return expr;
}

Expr *identifier_expr(TokenData identifier) {
    Expr *expr = (Expr *)s_malloc(sizeof(Expr));

    expr->type = EXPR_IDENTIFIER;
    expr->identifier.tok = identifier;

    return expr;
}

Expr *int_literal(char *value) {
    Expr *expr = (Expr *)s_malloc(sizeof(Expr));

    expr->type = EXPR_LITERAL_INT;
    expr->int_literal.value = value;

    return expr;
}

Expr *string_literal(char *value) {
    Expr *expr = (Expr *)s_malloc(sizeof(Expr));

    expr->type = EXPR_LITERAL_STRING;
    expr->str_literal.value = value;

    return expr;
}

Expr *bool_literal(int value) {
    Expr *expr = (Expr *)s_malloc(sizeof(Expr));

    expr->type = EXPR_LITERAL_BOOL;
    expr->bool_literal.value = value;

    return expr;
}

Stmt *var_decl_stmt(TokenData identifier, Expr *value) {
    Stmt *stmt = (Stmt *)s_malloc(sizeof(Stmt));

    stmt->type = STMT_VAR_DECL;
    stmt->var_decl.tok_identifier = identifier;
    stmt->var_decl.value = value;

    return stmt;
}

Stmt *return_stmt(Expr *value) {
    Stmt *stmt = (Stmt *)s_malloc(sizeof(Stmt));

    stmt->type = STMT_RETURN;
    stmt->return_stmt.value = value;

    return stmt;
}

Stmt *block_stmt(Stmt **statements, int stmt_count) {
    Stmt *stmt = (Stmt *)s_malloc(sizeof(Stmt));

    stmt->type = STMT_BLOCK;
    stmt->block_stmt.statements = statements;
    stmt->block_stmt.stmt_count = stmt_count;

    return stmt;
}

Program *create_program(Stmt **statements, int stmt_count) {
    Program *program = (Program *)s_malloc(sizeof(Program));

    program->statements = statements;
    program->stmt_count = stmt_count;

    return program;
}

// Helper function to recursively format expressions
char *expr_to_string(Expr *expr) {
    if (!expr) return strdup("null");

    char *buffer = s_malloc(512);

    switch (expr->type) {
        case EXPR_BINARY: {
            char *left_str = expr_to_string(expr->binary.left);
            char *right_str = expr_to_string(expr->binary.right);
            snprintf(buffer, 512, "BinaryExpr(%s %s %s)", left_str, expr->binary.op_token.val, right_str);
            s_free(left_str);
            s_free(right_str);
            return buffer;
        }
        case EXPR_UNARY: {
            char *right_str = expr_to_string(expr->unary.right);
            snprintf(buffer, 512, "UnaryExpr(%s %s)", expr->unary.op_token.val, right_str);
            s_free(right_str);
            return buffer;
        }
        case EXPR_IDENTIFIER:
            snprintf(buffer, 512, "IdentifierExpr(%s)", expr->identifier.tok.val);
            return buffer;
        case EXPR_LITERAL_INT:
            snprintf(buffer, 512, "IntLiteral(%s)", expr->int_literal.value);
            return buffer;
        case EXPR_LITERAL_STRING:
            snprintf(buffer, 512, "StringLiteral(\"%s\")", expr->str_literal.value);
            return buffer;
        case EXPR_LITERAL_BOOL:
            snprintf(buffer, 512, "BoolLiteral(%s)", expr->bool_literal.value ? "true" : "false");
            return buffer;
        case EXPR_CALL:
            s_free(buffer);
            return strdup("CallExpr()");
        default:
            s_free(buffer);
            return strdup("Unknown Expression");
    }
}

char *stmt_to_string(Stmt *stmt) {
    char *buffer = s_malloc(1024);

    switch (stmt->type) {
        case STMT_VAR_DECL: {
            char *expr_str = expr_to_string(stmt->var_decl.value);
            snprintf(buffer, 1024, "VarDeclStmt(%s = %s)", stmt->var_decl.tok_identifier.val, expr_str);
            s_free(expr_str);
            return buffer;
        }
        case STMT_RETURN: {
            if (stmt->return_stmt.value) {
                char *expr_str = expr_to_string(stmt->return_stmt.value);
                snprintf(buffer, 1024, "ReturnStmt(%s)", expr_str);
                s_free(expr_str);
            } else {
                snprintf(buffer, 1024, "ReturnStmt()");
            }
            return buffer;
        }
        case STMT_EXPR: {
            Expr *expr = stmt->expression_stmt.value;
            char *expr_str = expr_to_string(expr);
            snprintf(buffer, 1024, "ExprStmt(%s)", expr_str);
            s_free(expr_str);
            return buffer;
        }
        case STMT_BLOCK:
            snprintf(buffer, 1024, "BlockStmt()");
            return buffer;
        default:
            snprintf(buffer, 1024, "Unknown Statement Type");
            return buffer;
    }
}

void free_program(Program *prog) {
    if (!prog) return;

    for (int i = 0; i < prog->stmt_count; i++) {
        Stmt *stmt = prog->statements[i];
        if (stmt->type == STMT_EXPR && stmt->expression_stmt.value) {
            s_free(stmt->expression_stmt.value);
        } else if (stmt->type == STMT_RETURN && stmt->return_stmt.value) {
            s_free(stmt->return_stmt.value);
        }
        s_free(stmt);
    }
    s_free(prog->statements);
    s_free(prog);
}