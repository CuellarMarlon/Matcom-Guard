CC = gcc
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INCLUDE_DIR = include
TARGET = $(BIN_DIR)/matcomguard

# Flags de compilación y enlace
CFLAGS = -Wall -Wextra -I$(INCLUDE_DIR) `pkg-config --cflags gtk+-3.0` `pkg-config --cflags vte-2.91`
LDFLAGS = `pkg-config --libs gtk+-3.0` -lcrypto `pkg-config --libs vte-2.91`

# Buscar fuentes y objetos
SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Regla por defecto
all: $(TARGET)

# Enlazar el ejecutable
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compilar archivos .c a .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

mrproper: clean
	rm -f *~ */*~

.PHONY: all clean mrproper