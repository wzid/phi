#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef struct expr Expr;
typedef struct stmt Stmt;

// Expressions

// The type of expression
typedef enum {
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_IDENTIFIER,
    EXPR_LITERAL_INT,
    EXPR_LITERAL_STRING,
    EXPR_LITERAL_BOOL,
    EXPR_FUNC_CALL,
} ExprKind;

// Binary expression e.g. 1 + 1 
typedef struct {
    Expr *left;
    TokenData op_token;
    Expr *right;
} BinaryExpr;

// Unary expression e.g. -x or !x
typedef struct {
    TokenData op_token;
    Expr *right;
} UnaryExpr;

// Identifier expression e.g. variable names or function names
typedef struct {
    TokenData tok;
} IdentifierExpr;

// Integer literal expression e.g. 42
typedef struct {
    char *value;
} IntLiteral;

// String literal expression e.g. "hello"
typedef struct {
    char *value;
} StringLiteral;

// Boolean literal expression e.g. true or false
// These are stored as integers (0 or 1)
typedef struct {
    int value;
} BoolLiteral;

// Function call expression e.g. foo(1, 2)
typedef struct {
    TokenData tok_function;
    Expr **args;
    int arg_count;
} FuncCallExpr;

// The main expression struct
// It uses a union to hold the different expression types
struct expr {
    ExprKind type;
    union {
        BinaryExpr binary;
        UnaryExpr unary;
        IdentifierExpr identifier;
        IntLiteral int_literal;
        StringLiteral str_literal;
        BoolLiteral bool_literal;
        FuncCallExpr func_call;
    };
};


//  Statements

// The type of statement
typedef enum {
    STMT_VAR_DECL,
    STMT_GLOBAL_VAR_DECL,
    STMT_FUNC_DECL,
    STMT_FUNC_DECL_EXPLICIT,
    STMT_RETURN,
    STMT_EXPR,
    STMT_BLOCK,
} StmtKind;

// Expression statement e.g. a function call used as a statement
typedef struct {
    Expr *value;
} ExprStmt;

// Return statement e.g. return 42;
typedef struct {
    Expr *value;
} ReturnStmt;

// Block statement
// Usually used for function bodies
typedef struct {
    Stmt **statements; // pointer to an array of statement pointers
    int stmt_count;
} BlockStmt;

// Variable declaration statement e.g. int x = 42;
typedef struct {
    TokenData tok_identifier;
    Expr *value;
} VarDeclStmt;

// Variable declaration statement e.g. int x = 42;
typedef struct {
    TokenData tok_identifier;
    Expr *value;
} GlobalVarDeclStmt;

// Function declaration statement e.g. func foo() { ... }
typedef struct {
    TokenData tok_identifier;
    Stmt *body; // Should always be a BlockStmt
    TokenData tok_return_type;
    
    // These are needed to form function calls
    TokenData *parameter_names; 
    TokenData *parameter_types;
    int parameter_count;
} FuncDeclStmt;

// The main statement struct
// It uses a union to hold the different statement types
struct stmt {
    StmtKind type;
    union {
        GlobalVarDeclStmt global_var_decl;
        VarDeclStmt var_decl;
        FuncDeclStmt func_decl;
        ExprStmt expression_stmt;
        ReturnStmt return_stmt;
        BlockStmt block_stmt;
    };
};

// Program struct
// Holds all the statements in the program
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
Expr *func_call(TokenData tok_function, Expr **args, int arg_count);

Stmt *func_decl_stmt(TokenData identifier, Stmt *body, TokenData return_type, TokenData *parameter_names, TokenData *parameter_types, int parameter_count);
Stmt *var_decl_stmt(TokenData identifier, Expr *value);
Stmt *global_var_decl_stmt(TokenData identifier, Expr *value);
Stmt *return_stmt(Expr *value);
Stmt *block_stmt(Stmt **statements, int stmt_count);

void free_program(Program *prog);

char *expr_to_string(Expr *expr);
char *stmt_to_string(Stmt *stmt);


#endif