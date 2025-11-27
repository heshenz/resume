#include <stdio.h>
#include <string.h>
#include "arithmetic.h"
#include "matrix.h"
#include "expression.h"

int main() {
	char input[256];
	printf("Advanced Calculator\n");
	printf("Supports: +, -, *, /, ^, parentheses, matrices, functions\n");
	printf("Matrix format: [1,2,3;4,5,6] (comma=column separator, semicolon=row separator)\n");
	printf("Matrix operations: [1,2;3,4] + [5,6;7,8], [1,2;3,4] * [5,6;7,8]\n");
	printf("Scalar-matrix operations: 2 * [1,2;3,4], [1,2;3,4] * 3\n");
	printf("Matrix functions:\n");
	printf("  det([1,2;3,4])       - Calculate determinant\n");
	printf("  transpose([1,2;3,4]) - Matrix transpose (also: trans())\n");
	printf("Examples:\n");
	printf("  Matrix addition: [1,2;3,4] + [5,6;7,8]\n");
	printf("  Matrix multiplication: [1,2;3,4] * [2,0;1,2]\n");
	printf("  Scalar multiplication: 2 * [1,2;3,4]\n");
	printf("  Determinant: det([1,2;3,4])\n");
	printf("  Transpose: transpose([1,2;3,4])\n");
	printf("  Mixed: 2 + 3 * 4\n");
	printf("Enter an expression (or 'exit' to quit):\n");
	while (1) {
		printf("> ");
		if (!fgets(input, sizeof(input), stdin)) break;
		// Remove trailing newline
		input[strcspn(input, "\n")] = 0;
		if (strcmp(input, "exit") == 0) break;
		
		// Use the new mixed expression evaluator
		ExprResult result = eval_expression(input);
		printf("= ");
		expr_result_print(&result);
		printf("\n");
		expr_result_destroy(&result);
	}
	printf("Goodbye!\n");
	return 0;
}
