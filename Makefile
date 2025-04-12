CC = gcc
CFLAGS = -Wall -Wextra -g -Iinc
LDFLAGS = -lsqlite3 -lpthread

SRC_DIR = src
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = energy_monitor

# Default target
all: $(EXECUTABLE)

# Linking object files into the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Compiling .c files to .o
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean
