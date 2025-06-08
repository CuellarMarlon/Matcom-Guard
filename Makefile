CC=gcc
CFLAGS=-Iinclude -Iinclude/ports -Wall -Wextra -g

SRC=src/main.c \
    src/ports/ports.c \
    src/ports/read_status_ports/read_tcp.c

OUT=matcomguard

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)