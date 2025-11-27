# Matrix Support Documentation

## Matrix Features Added/Enhanced

### 1. Core Matrix Operations
- **Matrix Addition**: `[1,2;3,4] + [5,6;7,8]` → `[6,8;10,12]`
- **Matrix Subtraction**: `[5,6;7,8] - [1,2;3,4]` → `[4,4;4,4]`
- **Matrix Multiplication**: `[1,2;3,4] * [2,0;1,2]` → `[4,4;10,8]`

### 2. Scalar-Matrix Operations
- **Scalar × Matrix**: `2 * [1,2;3,4]` → `[2,4;6,8]`
- **Matrix × Scalar**: `[1,2;3,4] * 3` → `[3,6;9,12]`

### 3. Matrix Format
- Use square brackets: `[...]`
- Comma (`,`) separates elements in a row
- Semicolon (`;`) separates rows
- Example: `[1,2,3;4,5,6]` represents a 2×3 matrix:
  ```
  [1 2 3]
  [4 5 6]
  ```

### 4. Advanced Matrix Functions
- **Matrix Determinant**: Calculated using recursive cofactor expansion
- **Matrix Transpose**: Converts rows to columns
- **Identity Matrix**: Creates n×n identity matrix
- **Utility Functions**: Check matrix compatibility, square matrices

### 5. Integration with Expression Evaluator
- Mixed expressions: `2 + 3 * 4` (works with numbers)
- Parentheses support: `(2 + 3) * 4`
- Operator precedence maintained
- Error handling for incompatible operations

### 6. Memory Management
- Proper allocation and deallocation of matrix memory
- Automatic cleanup of intermediate results
- Safe handling of operation failures

## Files Modified/Enhanced

1. **Makefile**: Added expression.o compilation and -lm flag
2. **matrix.c**: Added determinant calculation, transpose, scalar operations, utilities
3. **matrix.h**: Added new function declarations
4. **expression.c**: Enhanced to support scalar-matrix operations
5. **calculator.c**: Updated help text with matrix examples
6. **test_operations.c**: Comprehensive test suite for all matrix operations

## Example Usage

```bash
# Compile the calculator
make

# Run interactive mode
./calculator.exe

# Run tests
make test_operations
./test_operations.exe
```

## Sample Matrix Operations

```
> [1,2;3,4] + [5,6;7,8]
= [6,8;10,12]

> [1,2;3,4] * [2,0;1,2]
= [4,4;10,8]

> 2 * [1,2;3,4]
= [2,4;6,8]

> [1,0;0,1] * [5,10;15,20]
= [5,10;15,20]
```

The matrix support is now fully integrated into your calculator with proper error handling, memory management, and comprehensive functionality!
