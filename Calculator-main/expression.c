#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include "expression.h"
#include "matrix.h"
#include "arithmetic.h"

// Helper: skip whitespace
static void skip_spaces(const char **s) {
    while (isspace(**s)) (*s)++;
}

// Helper functions for ExprResult
ExprResult expr_result_create_number(double num) {
    ExprResult result;
    result.type = EXPR_NUMBER;
    result.value.number = num;
    return result;
}

ExprResult expr_result_create_matrix(Matrix* m) {
    ExprResult result;
    result.type = EXPR_MATRIX;
    result.value.matrix = m;
    return result;
}

ExprResult expr_result_create_error(const char* msg) {
    ExprResult result;
    result.type = EXPR_ERROR;
    result.value.error_msg = malloc(strlen(msg) + 1);
    if (result.value.error_msg) {
        strcpy(result.value.error_msg, msg);
    }
    return result;
}

void expr_result_destroy(ExprResult* result) {
    if (result->type == EXPR_MATRIX && result->value.matrix) {
        matrix_destroy(result->value.matrix);
        result->value.matrix = NULL;
    } else if (result->type == EXPR_ERROR && result->value.error_msg) {
        free(result->value.error_msg);
        result->value.error_msg = NULL;
    }
}

void expr_result_print(ExprResult* result) {
    if (result->type == EXPR_NUMBER) {
        printf("%.6g", result->value.number);
    } else if (result->type == EXPR_MATRIX && result->value.matrix) {
        matrix_print(result->value.matrix);
    } else if (result->type == EXPR_ERROR && result->value.error_msg) {
        printf("Error: %s", result->value.error_msg);
    }
}

// Main entry point
ExprResult eval_expression(const char* expr) {
    const char *s = expr;
    skip_spaces(&s);
    ExprResult result = parse_expr_mixed(&s);
    skip_spaces(&s);
    return result;
}

// Parse addition/subtraction (supports matrix + matrix, number + number)
ExprResult parse_expr_mixed(const char **s) {
    ExprResult left = parse_term_mixed(s);
    skip_spaces(s);
    
    while (**s == '+' || **s == '-') {
        char op = **s;
        (*s)++;
        skip_spaces(s);
        ExprResult right = parse_term_mixed(s);
        
        if (op == '+') {
            if (left.type == EXPR_NUMBER && right.type == EXPR_NUMBER) {
                left.value.number += right.value.number;
            } else if (left.type == EXPR_MATRIX && right.type == EXPR_MATRIX) {
                Matrix* result_matrix = matrix_add_m(left.value.matrix, right.value.matrix);
                expr_result_destroy(&left);
                expr_result_destroy(&right);
                left = expr_result_create_matrix(result_matrix);
            } else {
                // Type mismatch - return error (for now, return left as-is)
                expr_result_destroy(&right);
            }
        } else { // subtraction
            if (left.type == EXPR_NUMBER && right.type == EXPR_NUMBER) {
                left.value.number -= right.value.number;
            } else if (left.type == EXPR_MATRIX && right.type == EXPR_MATRIX) {
                Matrix* result_matrix = matrix_subtract_m(left.value.matrix, right.value.matrix);
                expr_result_destroy(&left);
                expr_result_destroy(&right);
                left = expr_result_create_matrix(result_matrix);
            } else {
                expr_result_destroy(&right);
            }
        }
        skip_spaces(s);
    }
    return left;
}

// Parse multiplication/division (supports matrix * matrix, number * number)
ExprResult parse_term_mixed(const char **s) {
    ExprResult left = parse_power_mixed(s);
    skip_spaces(s);
    
    while (**s == '*' || **s == '/') {
        char op = **s;
        (*s)++;
        skip_spaces(s);
        ExprResult right = parse_power_mixed(s);
        
        if (op == '*') {
            if (left.type == EXPR_NUMBER && right.type == EXPR_NUMBER) {
                left.value.number *= right.value.number;
            } else if (left.type == EXPR_MATRIX && right.type == EXPR_MATRIX) {
                Matrix* result_matrix = matrix_multiply_m(left.value.matrix, right.value.matrix);
                expr_result_destroy(&left);
                expr_result_destroy(&right);
                left = expr_result_create_matrix(result_matrix);
            } else if (left.type == EXPR_NUMBER && right.type == EXPR_MATRIX) {
                // Scalar * Matrix
                Matrix* result_matrix = matrix_scalar_multiply(right.value.matrix, left.value.number);
                expr_result_destroy(&left);
                expr_result_destroy(&right);
                left = expr_result_create_matrix(result_matrix);
            } else if (left.type == EXPR_MATRIX && right.type == EXPR_NUMBER) {
                // Matrix * Scalar
                Matrix* result_matrix = matrix_scalar_multiply(left.value.matrix, right.value.number);
                expr_result_destroy(&left);
                expr_result_destroy(&right);
                left = expr_result_create_matrix(result_matrix);
            } else {
                // Type mismatch
                expr_result_destroy(&right);
            }
        } else { // division
            if (left.type == EXPR_NUMBER && right.type == EXPR_NUMBER) {
                left.value.number /= right.value.number;
            } else {
                // Matrix division not typically defined
                expr_result_destroy(&right);
            }
        }
        skip_spaces(s);
    }
    return left;
}

// Parse exponentiation (numbers only for now)
ExprResult parse_power_mixed(const char **s) {
    ExprResult left = parse_factor_mixed(s);
    skip_spaces(s);
    
    if (**s == '^') {
        (*s)++;
        skip_spaces(s);
        ExprResult right = parse_power_mixed(s);
        
        if (left.type == EXPR_NUMBER && right.type == EXPR_NUMBER) {
            left.value.number = pow(left.value.number, right.value.number);
        }
        expr_result_destroy(&right);
        skip_spaces(s);
    }
    return left;
}

// Parse numbers, matrices, and parentheses
ExprResult parse_factor_mixed(const char **s) {
    skip_spaces(s);
    
    int sign = 1;
    if (**s == '-') {
        sign = -1;
        (*s)++;
        skip_spaces(s);
    }
    
    // Check for functions (like det, transpose, etc.)
    if (isalpha(**s)) {
        ExprResult result = parse_function(s);
        if (sign == -1 && result.type == EXPR_NUMBER) {
            result.value.number *= -1;
        } else if (sign == -1 && result.type == EXPR_MATRIX) {
            // Apply negative sign to all elements
            if (result.value.matrix) {
                for (int i = 0; i < result.value.matrix->rows; i++) {
                    for (int j = 0; j < result.value.matrix->cols; j++) {
                        result.value.matrix->data[i][j] *= -1;
                    }
                }
            }
        }
        return result;
    }
    
    // Check for matrix
    if (**s == '[') {
        Matrix* m = parse_matrix(s);
        if (!m) {
            // Matrix parsing failed - return error message
            return expr_result_create_error("Invalid matrix format");
        }
        if (sign == -1) {
            // Apply negative sign to all elements
            for (int i = 0; i < m->rows; i++) {
                for (int j = 0; j < m->cols; j++) {
                    m->data[i][j] *= -1;
                }
            }
        }
        return expr_result_create_matrix(m);
    }
    
    // Check for parentheses
    if (**s == '(') {
        (*s)++; // skip '('
        ExprResult result = parse_expr_mixed(s);
        skip_spaces(s);
        if (**s == ')') {
            (*s)++; // skip ')'
        }
        if (sign == -1 && result.type == EXPR_NUMBER) {
            result.value.number *= -1;
        }
        return result;
    }
    
    // Parse number
    if (isdigit(**s) || **s == '.') {
        char *endptr;
        double value = strtod(*s, &endptr);
        *s = endptr;
        return expr_result_create_number(sign * value);
    }
    
    // Default: return zero
    return expr_result_create_number(0.0);
}

// Parse function calls like det([1,2;3,4]), transpose([1,2;3,4])
ExprResult parse_function(const char** s) {
    skip_spaces(s);
    
    // Read function name
    const char* start = *s;
    while (isalpha(**s)) (*s)++;
    
    int name_len = *s - start;
    char func_name[32];
    if (name_len >= (int)sizeof(func_name)) name_len = (int)sizeof(func_name) - 1;
    strncpy(func_name, start, name_len);
    func_name[name_len] = '\0';
    
    skip_spaces(s);
    
    // Expect opening parenthesis
    if (**s != '(') {
        // If no parenthesis, treat as variable (return 0 for now)
        return expr_result_create_number(0.0);
    }
    
    (*s)++; // skip '('
    skip_spaces(s);
    
    // Parse argument (should be a matrix for our functions)
    ExprResult arg = parse_expr_mixed(s);
    
    skip_spaces(s);
    if (**s == ')') (*s)++; // skip ')'
    
    // Handle different functions
    if (strcmp(func_name, "det") == 0) {
        if (arg.type == EXPR_MATRIX && arg.value.matrix) {
            double determinant = matrix_determinant_m(arg.value.matrix);
            expr_result_destroy(&arg);
            return expr_result_create_number(determinant);
        }
    } else if (strcmp(func_name, "transpose") == 0 || strcmp(func_name, "trans") == 0) {
        if (arg.type == EXPR_MATRIX && arg.value.matrix) {
            Matrix* transposed = matrix_transpose(arg.value.matrix);
            expr_result_destroy(&arg);
            return expr_result_create_matrix(transposed);
        }
    }
    
    // If function not recognized or argument invalid, clean up and return 0
    expr_result_destroy(&arg);
    return expr_result_create_number(0.0);
}
