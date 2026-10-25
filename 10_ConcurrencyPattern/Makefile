CC      := gcc
# C11 cho <stdatomic.h>; -pthread cho thread của PC (winpthreads / glibc)
CFLAGS  := -std=c11 -Wall -Wextra -Werror -pedantic -pthread -Iinc
SRCS    := main.c $(wildcard src/*.c)
TARGET  := demo
LDLIBS  :=

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS) $(wildcard inc/*.h src/*.h)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe
