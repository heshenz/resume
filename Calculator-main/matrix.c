#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "matrix.h"

// Matrix creation and destruction
Matrix* matrix_create(int rows, int cols) {
    Matrix* m = malloc(sizeof(Matrix));
    if (!m) return NULL;
    
    m->rows = rows;
    m->cols = cols;
    m->data = malloc(rows * sizeof(double*));
    if (!m->data) {
        free(m);
        return NULL;
    }
    
    for (int i = 0; i < rows; i++) {
        m->data[i] = malloc(cols * sizeof(double));
        if (!m->data[i]) {
            // Clean up on failure
            for (int j = 0; j < i; j++) {
                free(m->data[j]);
            }
            free(m->data);
            free(m);
            return NULL;
        }
    }
    return m;
}

void matrix_destroy(Matrix* m) {
    if (!m) return;
    for (int i = 0; i < m->rows; i++) {
        free(m->data[i]);
    }
    free(m->data);
    free(m);
}

// Helper: skip whitespace
static void skip_spaces(const char **s) {
    while (isspace(**s)) (*s)++;
}

// Matrix parsing: [1,2,3;4,5,6] -> 2x3 matrix
Matrix* parse_matrix(const char** s) {
    skip_spaces(s);
    if (**s != '[') return NULL;
    (*s)++; // skip '['
    
    // First pass: count dimensions
    const char* temp = *s;
    int rows = 1, cols = 0, current_cols = 0;
    
    while (*temp && *temp != ']') {
        skip_spaces(&temp);
        if (isdigit(*temp) || *temp == '.' || *temp == '-') {
            // Parse number (don't actually store it in first pass)
            char *endptr;
            strtod(temp, &endptr);
            temp = endptr;
            current_cols++;
        }
        skip_spaces(&temp);
        if (*temp == ',') {
            temp++;
        } else if (*temp == ';') {
            if (cols == 0) cols = current_cols;
            else if (current_cols != cols) return NULL; // inconsistent columns
            current_cols = 0;
            rows++;
            temp++;
        }
    }
    if (cols == 0) cols = current_cols;
    else if (current_cols != cols) return NULL; // Check final row consistency
    
    if (rows == 0 || cols == 0) return NULL;
    
    // Create matrix
    Matrix* m = matrix_create(rows, cols);
    if (!m) return NULL;
    
    // Second pass: actually parse the numbers
    skip_spaces(s);
    int row = 0, col = 0;
    while (**s && **s != ']') {
        skip_spaces(s);
        if (isdigit(**s) || **s == '.' || **s == '-') {
            char *endptr;
            m->data[row][col] = strtod(*s, &endptr);
            *s = endptr;
            col++;
        }
        skip_spaces(s);
        if (**s == ',') {
            (*s)++;
        } else if (**s == ';') {
            row++;
            col = 0;
            (*s)++;
        }
    }
    
    if (**s == ']') (*s)++; // skip closing ']'
    return m;
}

// Matrix addition
Matrix* matrix_add_m(Matrix* A, Matrix* B) {
    if (!A || !B || A->rows != B->rows || A->cols != B->cols) return NULL;
    
    Matrix* result = matrix_create(A->rows, A->cols);
    if (!result) return NULL;
    
    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < A->cols; j++) {
            result->data[i][j] = A->data[i][j] + B->data[i][j];
        }
    }
    return result;
}

// Matrix subtraction
Matrix* matrix_subtract_m(Matrix* A, Matrix* B) {
    if (!A || !B || A->rows != B->rows || A->cols != B->cols) return NULL;
    
    Matrix* result = matrix_create(A->rows, A->cols);
    if (!result) return NULL;
    
    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < A->cols; j++) {
            result->data[i][j] = A->data[i][j] - B->data[i][j];
        }
    }
    return result;
}

// Matrix multiplication
Matrix* matrix_multiply_m(Matrix* A, Matrix* B) {
    if (!A || !B || A->cols != B->rows) return NULL;
    
    Matrix* result = matrix_create(A->rows, B->cols);
    if (!result) return NULL;
    
    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < B->cols; j++) {
            result->data[i][j] = 0;
            for (int k = 0; k < A->cols; k++) {
                result->data[i][j] += A->data[i][k] * B->data[k][j];
            }
        }
    }
    return result;
}

// Matrix printing
void matrix_print(Matrix* m) {
    if (!m) return;
    
    if (m->rows == 1) {
        // Single row - print on same line
        printf("[");
        for (int j = 0; j < m->cols; j++) {
            if (j > 0) printf(" ");
            printf("%.6g", m->data[0][j]);
        }
        printf("]");
    } else {
        // Multiple rows - start on new line for better alignment
        printf("\n");
        for (int i = 0; i < m->rows; i++) {
            printf("[");
            
            for (int j = 0; j < m->cols; j++) {
                if (j > 0) printf(" ");
                printf("%8.6g", m->data[i][j]);  // Fixed width for better alignment
            }
            
            printf("]");
            if (i < m->rows - 1) {
                printf("\n");
            }
        }
    }
}

// Legacy functions (keeping for compatibility)
void matrix_add(int rows, int cols, double A[rows][cols], double B[rows][cols], double result[rows][cols]) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            result[i][j] = A[i][j] + B[i][j];
        }
    }
}

void matrix_multiply(int rowsA, int colsA, int colsB, double A[rowsA][colsA], double B[colsA][colsB], double result[rowsA][colsB]) {
    for (int i = 0; i < rowsA; i++) {
        for (int j = 0; j < colsB; j++) {
            result[i][j] = 0;
            for (int k = 0; k < colsA; k++) {
                result[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

double matrix_determinant(int n, double matrix[n][n]) {
    // For compatibility - create Matrix struct and call new function
    Matrix* m = matrix_create(n, n);
    if (!m) return 0.0;
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            m->data[i][j] = matrix[i][j];
        }
    }
    
    double det = matrix_determinant_m(m);
    matrix_destroy(m);
    return det;
}

// Scalar multiplication
Matrix* matrix_scalar_multiply(Matrix* A, double scalar) {
    if (!A) return NULL;
    
    Matrix* result = matrix_create(A->rows, A->cols);
    if (!result) return NULL;
    
    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < A->cols; j++) {
            result->data[i][j] = A->data[i][j] * scalar;
        }
    }
    return result;
}

// Matrix transpose
Matrix* matrix_transpose(Matrix* A) {
    if (!A) return NULL;
    
    Matrix* result = matrix_create(A->cols, A->rows);
    if (!result) return NULL;
    
    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < A->cols; j++) {
            result->data[j][i] = A->data[i][j];
        }
    }
    return result;
}

// Create identity matrix
Matrix* matrix_identity(int n) {
    Matrix* result = matrix_create(n, n);
    if (!result) return NULL;
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            result->data[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }
    return result;
}

// Matrix determinant using recursive expansion
double matrix_determinant_m(Matrix* A) {
    if (!A || A->rows != A->cols) return 0.0;
    
    int n = A->rows;
    
    // Base cases
    if (n == 1) {
        return A->data[0][0];
    }
    if (n == 2) {
        return A->data[0][0] * A->data[1][1] - A->data[0][1] * A->data[1][0];
    }
    
    // For larger matrices, use cofactor expansion along first row
    double det = 0.0;
    for (int j = 0; j < n; j++) {
        // Create minor matrix
        Matrix* minor = matrix_create(n-1, n-1);
        if (!minor) return 0.0;
        
        for (int i = 1; i < n; i++) {
            int minor_col = 0;
            for (int k = 0; k < n; k++) {
                if (k != j) {
                    minor->data[i-1][minor_col] = A->data[i][k];
                    minor_col++;
                }
            }
        }
        
        double cofactor = ((j % 2 == 0) ? 1 : -1) * A->data[0][j] * matrix_determinant_m(minor);
        det += cofactor;
        matrix_destroy(minor);
    }
    
    return det;
}

// Utility functions
int matrix_is_square(Matrix* m) {
    return (m && m->rows == m->cols);
}
//do this later, but add is_square to header file and implement with the future addition of is_symmetric and is_identity,
//also add an LU decomposition function to the header file and implement it here
//additionally, implement a function to generate a random matrix of given dimensions with values in a specified range
//add a function to create identity matrix of given size.
int matrix_are_compatible_for_add(Matrix* A, Matrix* B) {
    return (A && B && A->rows == B->rows && A->cols == B->cols);
}

int matrix_are_compatible_for_multiply(Matrix* A, Matrix* B) {
    return (A && B && A->cols == B->rows);
}