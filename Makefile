# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -Isrc/config
LDFLAGS = -lcrypto

# Directorios fuente
SRCDIRS = src src/usb src/config

# Archivos fuente del programa principal
CONFIGSRC = src/config/config_usb.c src/config/ini.c
SRC = src/main.c src/usb/usb.c $(CONFIGSRC)
OBJ = $(SRC:.c=.o)
TARGET = matcomguard

# Archivos para pruebas
TESTSRC = tests/test_usb.c
TESTOBJ = $(TESTSRC:.c=.o)
TESTBIN = test_usb

# Regla por defecto: compilar programa
all: $(TARGET)

# Compilar ejecutable principal
$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

# Compilar pruebas
$(TESTBIN): $(TESTOBJ) src/usb/usb.o src/config/config_usb.o src/config/ini.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilar cualquier .c a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Ejecutar pruebas
.PHONY: test
test: $(TESTBIN)
	./$(TESTBIN)

# Limpieza
clean:
	rm -f $(OBJ) $(TESTOBJ) $(TARGET) $(TESTBIN)

.PHONY: all clean
