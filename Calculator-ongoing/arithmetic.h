#ifndef ARITHMETIC_H
#define ARITHMETIC_H

double add(double a, double b);
double subtract(double a, double b);
double multiply(double a, double b);
double divide(double a, double b);

// Evaluates a string arithmetic expression (supports +, -, *, /)
double eval_expr(const char* expr);

#endif // ARITHMETIC_H
