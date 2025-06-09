CC=gcc
CFLAGS=-Iinclude -Iinclude/ports -Wall -Wextra -g

SRC=src/main.c \
    src/ports/scan_ports/active_scanner.c

OUT=matcomguard

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)