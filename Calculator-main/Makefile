CC = gcc
CFLAGS = -Wall -Wextra -g
LIBS = -lm
OBJS = calculator.o arithmetic.o calculus.o matrix.o expression.o

calculator: $(OBJS)
	$(CC) -o calculator $(OBJS) $(LIBS)

test_operations: test_operations.o arithmetic.o matrix.o expression.o
	$(CC) -o test_operations test_operations.o arithmetic.o matrix.o expression.o $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: calculator
	./calculator

clean:
	rm -f *.o calculator test_operations
