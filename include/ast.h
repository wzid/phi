#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef struct expr Expr;
typedef struct stmt Stmt;

// Expressions

typedef enum {
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_IDENTIFIER,
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
    TokenData tok;
} IdentifierExpr;

typedef struct {
    char *value;
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
        IdentifierExpr identifier;
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
    Expr *value;
} ExprStmt;

typedef struct {
    Expr *value;
} ReturnStmt;

typedef struct {
    Stmt **statements; // pointer to an array of statement pointers
    int stmt_count;
} BlockStmt;

typedef struct {
    TokenData tok_identifier;
    Expr *value;
} VarDeclStmt;

struct stmt {
    StmtKind type;
    union {
        VarDeclStmt var_decl;
        ExprStmt expression_stmt;
        ReturnStmt return_stmt;
        BlockStmt block_stmt;
    };
};

typedef struct {
    Stmt **statements;
    int stmt_count;
    int capacity;
} Program;


// Expressions
Expr *binary_expr(Expr *left, TokenData op, Expr *right);
Expr *unary_expr(TokenData op, Expr *right);
Expr *identifier_expr(TokenData identifier);
Expr *int_literal(char *value);
Expr *string_literal(char *value);
Expr *bool_literal(int value);

Stmt *var_decl_stmt(TokenData identifier, Expr *value);
Stmt *return_stmt(Expr *value);
Stmt *block_stmt(Stmt **statements, int stmt_count);

void free_program(Program *prog);

char *expr_to_string(Expr *expr);
char *stmt_to_string(Stmt *stmt);


#endif