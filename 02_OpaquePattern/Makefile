CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Werror -pedantic -Iinc
SRCS    := main.c $(wildcard src/*.c)
TARGET  := demo

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS) $(wildcard inc/*.h src/*.h)
	$(CC) $(CFLAGS) $(SRCS) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe
