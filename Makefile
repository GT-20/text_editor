CC = clang
CFLAGS = -Os -flto -ffunction-sections -fdata-sections \
         -fno-unwind-tables -fno-asynchronous-unwind-tables \
         -fno-stack-protector -Wall -Wextra
# LDFLAGS = -lncurses -ltinfo

TARGET = typr
SRC = main.c core.c input.c selection.c
HDR = $(wildcard *.h)
DEBUG = debug.c

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

debug: $(SRC) $(DEBUG) $(HDR)
	$(CC) -O0 -DDEBUG_EXISTS -Wall -Wextra -o $(TARGET) $(SRC) $(DEBUG) $(LDFLAGS)

memory:
	valgrind --leak-check=full ./$(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
