# Calculator
Calculator in C, constantly developed to help me with math

## Features

### Basic Arithmetic
- Addition, subtraction, multiplication, division
- Exponentiation with `^` operator
- Parentheses support for expression grouping
- Proper operator precedence

### Matrix Operations
- **Matrix format**: `[1,2,3;4,5,6]` (comma=column separator, semicolon=row separator)
- **Matrix addition/subtraction**: `[1,2;3,4] + [5,6;7,8]`
- **Matrix multiplication**: `[1,2;3,4] * [2,0;1,2]`
- **Scalar-matrix operations**: `2 * [1,2;3,4]` or `[1,2;3,4] * 3`
- **Advanced operations**: Determinant calculation, transpose, identity matrices

### Mixed Expressions
- Combine numbers and matrices in complex expressions
- Example: `2 + 3 * 4`, `(2 + 3) * [1,2;3,4]`

## Building

```bash
make                    # Build main calculator
make test_operations    # Build test program
make clean             # Clean build files
```

## Usage

```bash
./calculator.exe        # Interactive mode
./test_operations.exe   # Run matrix operation tests
```

## Examples

```
> 2 + 3 * 4
= 14

> [1,2;3,4] + [5,6;7,8]
= [6,8;10,12]

> 2 * [1,2;3,4]
= [2,4;6,8]

> [1,2;3,4] * [2,0;1,2]
= [4,4;10,8]
```
