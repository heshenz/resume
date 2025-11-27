double add(double a, double b) {
	return a + b;
}

double subtract(double a, double b) {
	return a - b;
}

double multiply(double a, double b) {
	return a * b;
}

double divide(double a, double b) {
	return a / b;
}

// Basic arithmetic expression evaluator
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// Helper: skip whitespace
static void skip_spaces(const char **s) {
	while (isspace(**s)) (*s)++;
}
 
// Forward declarations
static double parse_expr(const char **s);
static double parse_term(const char **s);
static double parse_factor(const char **s);

// Main entry point
double eval_expr(const char* expr) {
	const char *s = expr;
	skip_spaces(&s);
	double result = parse_expr(&s);
	skip_spaces(&s);
	// Optionally check for trailing garbage
	return result;
}

// Parse addition/subtraction
static double parse_expr(const char **s) {
	double value = parse_term(s);
	skip_spaces(s);
	while (**s == '+' || **s == '-') {
		char op = **s;
		(*s)++;
		skip_spaces(s);
		double rhs = parse_term(s);
		if (op == '+') value += rhs;
		else value -= rhs;
		skip_spaces(s);
	}
	return value;
}

// Parse multiplication/division
static double parse_term(const char **s) {
	double value = parse_factor(s);
	skip_spaces(s);
	while (**s == '*' || **s == '/') {
		char op = **s;
		(*s)++;
		skip_spaces(s);
		double rhs = parse_factor(s);
		if (op == '*') value *= rhs;
		else value /= rhs;
		skip_spaces(s);
	}
	return value;
}

// Parse numbers (and parentheses, if you want to extend)
static double parse_factor(const char **s) {
	skip_spaces(s);
	double value = 0.0;
	int sign = 1;
	if (**s == '-') {
		sign = -1;
		(*s)++;
		skip_spaces(s);
	}
	if (isdigit(**s) || **s == '.') {
		char *endptr;
		value = strtod(*s, &endptr);
		*s = endptr;
	}
	skip_spaces(s);
	return sign * value;
}

