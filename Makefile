CC = gcc
CFLAGS = -g -Wall -Werror
BIN = poc
LDLIBS = -lbluetooth
all: $(BIN)

clean:
	rm -f $(BIN)

fmt:
	clang-format -i poc.c
