CC=gcc
CFLAGS=-Iinclude -Iinclude/ports -Wall -Wextra -g

SRC=src/main.c \
    src/ports/ports.c \
    src/ports/parse_status_ports/parse_files.c \
    src/ports/analysis_ports/port_analyzer.c \
    src/ports/analysis_ports/process_lookup.c

OUT=matcomguard

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)