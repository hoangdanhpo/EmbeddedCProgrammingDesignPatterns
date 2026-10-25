CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Werror -pedantic -pthread -Iinc $(EXTRA)
SRCS    := main.c $(wildcard src/*.c)
TARGET  := demo

# EXTRA cho phép thêm cờ khi thí nghiệm, ví dụ: make clean; make run EXTRA=-DI2C_BUS_DEMO_NO_LOCK

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS) $(wildcard inc/*.h src/*.h)
	$(CC) $(CFLAGS) $(SRCS) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe
