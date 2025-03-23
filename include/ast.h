#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef struct expr Expr;
typedef struct stmt Stmt;

// Expressions

typedef enum {
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_LITERAL_INT,
    EXPR_LITERAL_STRING,
    EXPR_LITERAL_BOOL,
    EXPR_CALL,
} ExprKind;

typedef struct {
    Expr *left;
    TokenData op_token;
    Expr *right;
} BinaryExpr;

typedef struct {
    TokenData op_token;
    Expr *right;
} UnaryExpr;

typedef struct {
    int value;
} IntLiteral;

typedef struct {
    char *value;
} StringLiteral;

typedef struct {
    int value;
} BoolLiteral;

struct expr {
    ExprKind type;
    union {
        BinaryExpr binary;
        UnaryExpr unary;
        IntLiteral int_literal;
        StringLiteral str_literal;
        BoolLiteral bool_literal;
    };
};


//  Statements

typedef enum {
    STMT_VAR_DECL,
    STMT_FUNC_DECL,
    STMT_FUNC_DECL_EXPLICIT,
    STMT_RETURN,
    STMT_EXPR,
    STMT_BLOCK,
} StmtKind;

typedef struct {
    TokenData tok_identifier;
    
} VarDeclStmt;

struct stmt {
    StmtKind type;
    union {
        
    };
};


#endif