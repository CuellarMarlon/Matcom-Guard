#!/bin/bash

FAKE_USB_DIR="/tmp/fake_usb"
FILE1="$FAKE_USB_DIR/file1.txt"
FILE2="$FAKE_USB_DIR/file2.txt"
FILE_NEW="$FAKE_USB_DIR/file_nuevo.txt"

function show_menu() {
    echo ""
    echo "=== Menú de acciones sobre $FAKE_USB_DIR ==="
    echo "1 - Crear carpeta fake_usb y archivos base"
    echo "2 - Eliminar carpeta fake_usb"
    echo "3 - Agregar un nuevo archivo"
    echo "4 - Eliminar un archivo"
    echo "5 - Modificar tamaño de archivo (agregar texto)"
    echo "6 - Cambiar permisos de un archivo"
    echo "7 - Renombrar archivo"
    echo "0 - Salir"
    echo "============================================"
}

function crear_fake_usb() {
    mkdir -p "$FAKE_USB_DIR"
    echo "Archivo original 1" > "$FILE1"
    echo "Archivo original 2" > "$FILE2"
    echo "✅ Carpeta y archivos creados."
}

function eliminar_fake_usb() {
    rm -rf "$FAKE_USB_DIR"
    echo "🗑️ Carpeta eliminada."
}

function agregar_archivo() {
    echo "Este es un nuevo archivo" > "$FILE_NEW"
    echo "✅ Archivo nuevo creado: $FILE_NEW"
}

function eliminar_archivo() {
    rm -f "$FILE2"
    echo "🗑️ Archivo eliminado: $FILE2"
}

function cambiar_tamano() {
    echo "Línea adicional de prueba" >> "$FILE1"
    echo "📏 Tamaño de $FILE1 modificado (se agregó texto)."
}

function cambiar_permiso() {
    chmod 777 "$FILE1"
    echo "🔐 Permisos de $FILE1 cambiados a 777."
}

function renombrar_archivo() {
    mv "$FILE1" "$FAKE_USB_DIR/file1_renombrado.txt"
    echo "🔁 Archivo renombrado a file1_renombrado.txt"
}

# Loop de menú
while true; do
    show_menu
    read -rp "Elige una opción: " opc
    case $opc in
        1) crear_fake_usb ;;
        2) eliminar_fake_usb ;;
        3) agregar_archivo ;;
        4) eliminar_archivo ;;
        5) cambiar_tamano ;;
        6) cambiar_permiso ;;
        7) renombrar_archivo ;;
        0) echo "👋 Saliendo..."; break ;;
        *) echo "❌ Opción inválida." ;;
    esac
done
