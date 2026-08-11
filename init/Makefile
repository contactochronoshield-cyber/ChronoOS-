CC = gcc

CFLAGS = -Wall -Wextra -Iinclude

TARGET = chrono-init

SRC = src/main.c src/logger.c

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
