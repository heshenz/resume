#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "matrix.h"

// Expression result type - can be either a number, a matrix, or an error
typedef enum {
    EXPR_NUMBER,
    EXPR_MATRIX,
    EXPR_ERROR
} ExprType;

typedef struct {
    ExprType type;
    union {
        double number;
        Matrix* matrix;
        char* error_msg;
    } value;
} ExprResult;

// Expression evaluation functions
ExprResult eval_expression(const char* expr);
ExprResult parse_expr_mixed(const char** s);
ExprResult parse_term_mixed(const char** s);
ExprResult parse_power_mixed(const char** s);
ExprResult parse_factor_mixed(const char** s);
ExprResult parse_function(const char** s);

// Helper functions
void expr_result_destroy(ExprResult* result);
void expr_result_print(ExprResult* result);
ExprResult expr_result_create_number(double num);
ExprResult expr_result_create_matrix(Matrix* m);
ExprResult expr_result_create_error(const char* msg);

#endif // EXPRESSION_H
