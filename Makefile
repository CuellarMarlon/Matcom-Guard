CC=gcc
CFLAGS=-Iinclude -Wall -Wextra -g
SRC=src/main.c src/ports.c
OUT=matcomguard

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)