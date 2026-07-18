CC = clang
CFLAGS = -Os -flto -ffunction-sections -fdata-sections \
         -fno-unwind-tables -fno-asynchronous-unwind-tables \
         -fno-stack-protector
# LDFLAGS = -lncurses -ltinfo

TARGET = a
SRC = main.c core.c input.c
DEBUG = debug.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

debug:
	$(CC) -O0 -DDEBUG_EXISTS -o $(TARGET) $(SRC) $(DEBUG) $(LDFLAGS)

memory:
	valgrind --leak-check=full ./$(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
