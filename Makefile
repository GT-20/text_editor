CC = clang
# CFLAGS = -Wall -Wextra -O2
# LDFLAGS = -lncurses -ltinfo

TARGET = a
SRC = main.c ./term/core.c input.c debug.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
