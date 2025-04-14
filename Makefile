CC = gcc
CFLAGS = -Wall -Wextra -g -Iinc
LDFLAGS = -lsqlite3 -lpthread

SRC_DIR = src
DATA_DIR = data
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = energy_monitor

# Default target
all: setup $(EXECUTABLE)

# Setup data directory for DB and JSON files
setup:
	mkdir -p $(DATA_DIR)

# Linking object files into the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Compiling .c files to .o
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
	rm -rf $(DATA_DIR)

.PHONY: all clean
