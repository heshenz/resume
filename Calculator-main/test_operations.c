#include <stdio.h>
#include "expression.h"
#include "matrix.h"

int main() {
    printf("Matrix Operations Test\n\n");
    
    // Test matrix addition
    printf("Test 1: Matrix Addition [1,2;3,4] + [5,6;7,8]\n");
    ExprResult result1 = eval_expression("[1,2;3,4] + [5,6;7,8]");
    printf("Result: ");
    expr_result_print(&result1);
    printf("\n\n");
    expr_result_destroy(&result1);
    
    // Test matrix multiplication
    printf("Test 2: Matrix Multiplication [1,2;3,4] * [5,6;7,8]\n");
    ExprResult result2 = eval_expression("[1,2;3,4] * [5,6;7,8]");
    printf("Result: ");
    expr_result_print(&result2);
    printf("\n\n");
    expr_result_destroy(&result2);
    
    // Test matrix subtraction
    printf("Test 3: Matrix Subtraction [5,6;7,8] - [1,2;3,4]\n");
    ExprResult result3 = eval_expression("[5,6;7,8] - [1,2;3,4]");
    printf("Result: ");
    expr_result_print(&result3);
    printf("\n\n");
    expr_result_destroy(&result3);
    
    // Test scalar-matrix multiplication
    printf("Test 4: Scalar-Matrix Multiplication 2 * [1,2;3,4]\n");
    ExprResult result4 = eval_expression("2 * [1,2;3,4]");
    printf("Result: ");
    expr_result_print(&result4);
    printf("\n\n");
    expr_result_destroy(&result4);
    
    // Test matrix-scalar multiplication
    printf("Test 5: Matrix-Scalar Multiplication [1,2;3,4] * 3\n");
    ExprResult result5 = eval_expression("[1,2;3,4] * 3");
    printf("Result: ");
    expr_result_print(&result5);
    printf("\n\n");
    expr_result_destroy(&result5);
    
    // Test mixed operations with numbers
    printf("Test 6: Mixed Operations 2 + 3 * 4\n");
    ExprResult result6 = eval_expression("2 + 3 * 4");
    printf("Result: ");
    expr_result_print(&result6);
    printf("\n\n");
    expr_result_destroy(&result6);
    
    // Test matrix determinant calculation manually
    printf("Test 7: Matrix Determinant Calculation\n");
    Matrix* test_matrix = matrix_create(2, 2);
    test_matrix->data[0][0] = 1; test_matrix->data[0][1] = 2;
    test_matrix->data[1][0] = 3; test_matrix->data[1][1] = 4;
    
    double det = matrix_determinant_m(test_matrix);
    printf("Determinant of [1,2;3,4]: %.6g\n", det);
    matrix_destroy(test_matrix);
    printf("\n");
    
    // Test matrix transpose
    printf("Test 8: Matrix Transpose\n");
    Matrix* original = matrix_create(2, 3);
    original->data[0][0] = 1; original->data[0][1] = 2; original->data[0][2] = 3;
    original->data[1][0] = 4; original->data[1][1] = 5; original->data[1][2] = 6;
    
    Matrix* transposed = matrix_transpose(original);
    printf("Original: ");
    matrix_print(original);
    printf("\nTransposed: ");
    matrix_print(transposed);
    printf("\n\n");
    
    matrix_destroy(original);
    matrix_destroy(transposed);
    
    return 0;
}
