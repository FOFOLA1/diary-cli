# Compiler and flags
CC = gcc
# CFLAGS are your compiler flags. -Wall (all warnings) is highly recommended.
CFLAGS = -Wall -Wextra -std=c99 -g

# Your final executable name
TARGET = a.out

# Find all .c source files automatically
SRCS = $(wildcard *.c)
# Create a list of object files (.o) from the source files
OBJS = $(SRCS:.c=.o)

# The default rule, which is run when you just type 'make'
# .PHONY tells make that 'all' isn't a real file.
.PHONY: all
all: $(TARGET)

strings.h: strings.ini
	xxd -i strings.ini > strings.h

main.o: strings.h

# Rule to link the final executable
# This says: To make the TARGET, I first need all the OBJS.
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Pattern rule to compile .c files into .o files
# This says: To make any .o file, I need the corresponding .c file.
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# A rule to clean up your build files
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)