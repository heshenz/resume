#ifndef MATRIX_H
#define MATRIX_H

// Matrix structure
typedef struct {
    int rows;
    int cols;
    double **data;
} Matrix;

// Matrix creation and destruction
Matrix* matrix_create(int rows, int cols);
void matrix_destroy(Matrix* m);

// Matrix parsing and utilities
Matrix* parse_matrix(const char** s);
int matrix_is_square(Matrix* m);
int matrix_are_compatible_for_add(Matrix* A, Matrix* B);
int matrix_are_compatible_for_multiply(Matrix* A, Matrix* B);

// Matrix operations
Matrix* matrix_add_m(Matrix* A, Matrix* B);
Matrix* matrix_subtract_m(Matrix* A, Matrix* B);
Matrix* matrix_multiply_m(Matrix* A, Matrix* B);
Matrix* matrix_scalar_multiply(Matrix* A, double scalar);
Matrix* matrix_transpose(Matrix* A);
double matrix_determinant_m(Matrix* A);
Matrix* matrix_identity(int n);
void matrix_print(Matrix* m);

// Legacy functions (keeping for compatibility)
void matrix_add(int rows, int cols, double A[rows][cols], double B[rows][cols], double result[rows][cols]);
void matrix_multiply(int rowsA, int colsA, int colsB, double A[rowsA][colsA], double B[colsA][colsB], double result[rowsA][colsB]);
double matrix_determinant(int n, double matrix[n][n]);

#endif // MATRIX_H
