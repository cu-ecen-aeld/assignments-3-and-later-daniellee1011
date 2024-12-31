# Compiler and flags
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror

# Target application
TARGET=writer
SRC=writer.c
OBJ=$(SRC:.c=.o)

# Default target: build the writer application
all: $(TARGET)

# Rule to build the writer application
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Rule to compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove generated files
clean:
	rm -f $(TARGET) $(OBJ)
