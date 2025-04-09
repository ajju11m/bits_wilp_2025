CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lsqlite3 -lpthread

SOURCES = main.c db.c smartHome_logger.c sensor_data_sampling.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = energy_monitor

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
