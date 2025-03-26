#include "ast.h"
#include <stdlib.h>
#include <stdio.h>


Expr *binary_expr(Expr *left, TokenData op, Expr *right) {
    Expr *expr = (Expr *) malloc(sizeof(Expr));
    if (!expr) {
        perror("Failed to allocate memory for binary expression.");
        exit(1);
    }

    expr->type = EXPR_BINARY;
    expr->binary.left = left;
    expr->binary.op_token = op;
    expr->binary.right = right;

    return expr;
}

Expr *unary_expr(TokenData op, Expr *right) {
    Expr *expr = (Expr *) malloc(sizeof(Expr));
    if (!expr) {
        perror("Failed to allocate memory for unary expression.");
        exit(1);
    }

    expr->type = EXPR_UNARY;
    expr->unary.op_token = op;
    expr->unary.right = right;

    return expr;
}

Expr *int_literal(int value) {
    Expr *expr = (Expr *) malloc(sizeof(Expr));
    if (!expr) {
        perror("Failed to allocate memory for int literal.");
        exit(1);
    }

    expr->type = EXPR_LITERAL_INT;
    expr->int_literal.value = value;

    return expr;
}

Expr *string_literal(char *value) {
    Expr *expr = (Expr *) malloc(sizeof(Expr));
    if (!expr) {
        perror("Failed to allocate memory for string literal.");
        exit(1);
    }

    expr->type = EXPR_LITERAL_STRING;
    expr->str_literal.value = value;

    return expr;
}

Expr *bool_literal(int value) {
    Expr *expr = (Expr *) malloc(sizeof(Expr));
    if (!expr) {
        perror("Failed to allocate memory for bool literal.");
        exit(1);
    }

    expr->type = EXPR_LITERAL_BOOL;
    expr->bool_literal.value = value;

    return expr;
}

Stmt *return_stmt(Expr *value) {
    Stmt *stmt = (Stmt *) malloc(sizeof(Stmt));
    if (!stmt) {
        perror("Failed to allocate memory for return statement.");
        exit(1);
    }

    stmt->type = STMT_RETURN;
    stmt->return_stmt.value = value;

    return stmt;
}

Stmt *block_stmt(Stmt **statements, int stmt_count) {
    Stmt *stmt = (Stmt *) malloc(sizeof(Stmt));
    if (!stmt) {
        perror("Failed to allocate memory for block statement.");
        exit(1);
    }

    stmt->type = STMT_BLOCK;
    stmt->block_stmt.statements = statements;
    stmt->block_stmt.stmt_count = stmt_count;

    return stmt;
}