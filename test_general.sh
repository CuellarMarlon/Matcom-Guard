#!/bin/bash

DEFAULT_MEM_MB=300
FAKE_USB_DIR="/tmp/fake_usb"
CPU_HOG_PID_FILE="/tmp/cpu_hog.pid"
MEM_HOG_PID_FILE="/tmp/mem_hog.pid"

show_menu() {
    echo ""
    echo "=== Menú de acciones MatcomGuard ==="
    echo "1 - Crear carpeta fake_usb y archivos base"
    echo "2 - Eliminar carpeta fake_usb"
    echo "3 - Agregar un nuevo archivo"
    echo "4 - Eliminar un archivo"
    echo "5 - Modificar tamaño de archivo"
    echo "6 - Cambiar permisos de archivo"
    echo "7 - Renombrar archivo"
    echo "8 - Iniciar CPU hog"
    echo "9 - Detener CPU hog"
    echo "10 - Iniciar Memory hog"
    echo "11 - Detener Memory hog"
    echo "12 - Mostrar PIDs activos"
    echo "13 - Ajustar tamaño de Memory hog (solo modificable en script)"
    echo "14 - Abrir SSH en puerto 22"
    echo "15 - Cerrar SSH en puerto 22"
    echo "16 - Abrir puerto sospechoso 31337"
    echo "17 - Cerrar puerto sospechoso 31337"
    echo "18 - Servidor HTTP puerto 4444"
    echo "19 - Cerrar servidor HTTP"
    echo "20 - Alternar puerto 77"
    echo "0 - Salir"
    echo ""
}

crear_fake_usb() { mkdir -p "$FAKE_USB_DIR"; echo "Archivo original 1" > "$FAKE_USB_DIR/file1.txt"; echo "Archivo original 2" > "$FAKE_USB_DIR/file2.txt"; echo "✅ Carpeta y archivos creados."; }
eliminar_fake_usb() { rm -rf "$FAKE_USB_DIR"; echo "🗑 Carpeta eliminada."; }
agregar_archivo() { echo "Este es un nuevo archivo" > "$FAKE_USB_DIR/file_nuevo.txt"; echo "✅ Archivo nuevo creado."; }
eliminar_archivo() { rm -f "$FAKE_USB_DIR/file2.txt"; echo "🗑 Archivo eliminado."; }
modificar_tamano() { [[ -f "$FAKE_USB_DIR/file1.txt" ]] && echo "Línea adicional de prueba" >> "$FAKE_USB_DIR/file1.txt" && echo "📏 Tamaño modificado." || echo "⚠️ El archivo file1.txt no existe."; }
cambiar_permisos() { [[ -f "$FAKE_USB_DIR/file1.txt" ]] && chmod 777 "$FAKE_USB_DIR/file1.txt" && echo "🔐 Permisos cambiados a 777." || echo "⚠️ El archivo file1.txt no existe."; }
renombrar_archivo() { [[ -f "$FAKE_USB_DIR/file1.txt" ]] && mv "$FAKE_USB_DIR/file1.txt" "$FAKE_USB_DIR/file1_renombrado.txt" && echo "🔁 Archivo renombrado." || echo "⚠️ El archivo file1.txt no existe."; }
start_cpu_hog() { [[ -f "$CPU_HOG_PID_FILE" ]] && kill -0 "$(cat "$CPU_HOG_PID_FILE")" 2>/dev/null && echo "⚠️ CPU hog ya está corriendo con PID $(cat "$CPU_HOG_PID_FILE")" && return; ( while :; do :; done ) & echo $! > "$CPU_HOG_PID_FILE"; echo "✅ CPU hog iniciado con PID $(cat "$CPU_HOG_PID_FILE")"; }
stop_cpu_hog() { [[ -f "$CPU_HOG_PID_FILE" ]] && kill -0 "$(cat "$CPU_HOG_PID_FILE")" 2>/dev/null && kill "$(cat "$CPU_HOG_PID_FILE")" && rm -f "$CPU_HOG_PID_FILE" && echo "🛑 CPU hog detenido." || echo "⚠️ No hay CPU hog activo."; }
start_mem_hog() { [[ -f "$MEM_HOG_PID_FILE" ]] && kill -0 "$(cat "$MEM_HOG_PID_FILE")" 2>/dev/null && echo "⚠️ Memory hog ya está corriendo con PID $(cat "$MEM_HOG_PID_FILE")" && return; ( dd if=/dev/zero bs=1M count="$DEFAULT_MEM_MB" | tail -f /dev/null ) & echo $! > "$MEM_HOG_PID_FILE"; echo "✅ Memory hog iniciado con PID $(cat "$MEM_HOG_PID_FILE") (~${DEFAULT_MEM_MB} MB)"; }
stop_mem_hog() { [[ -f "$MEM_HOG_PID_FILE" ]] && kill -0 "$(cat "$MEM_HOG_PID_FILE")" 2>/dev/null && kill "$(cat "$MEM_HOG_PID_FILE")" && rm -f "$MEM_HOG_PID_FILE" && echo "🛑 Memory hog detenido." || echo "⚠️ No hay Memory hog activo."; }
show_pids() { [[ -f "$CPU_HOG_PID_FILE" ]] && kill -0 "$(cat "$CPU_HOG_PID_FILE")" 2>/dev/null && echo "CPU hog PID: $(cat "$CPU_HOG_PID_FILE")" || echo "CPU hog: detenido"; [[ -f "$MEM_HOG_PID_FILE" ]] && kill -0 "$(cat "$MEM_HOG_PID_FILE")" 2>/dev/null && echo "Memory hog PID: $(cat "$MEM_HOG_PID_FILE") (~${DEFAULT_MEM_MB} MB)" || echo "Memory hog: detenido"; }
ajustar_mem_size() { echo "Actualmente el tamaño está fijado a $DEFAULT_MEM_MB MB."; echo "Para cambiarlo, edita el script y modifica la variable DEFAULT_MEM_MB."; }
abrir_ssh() { sudo systemctl start sshd && echo "🔓 SSH abierto."; }
cerrar_ssh() { sudo systemctl stop sshd && echo "🔒 SSH cerrado."; }
abrir_puerto_31337() { nohup nc -l 31337 > /dev/null 2>&1 & echo "⚠️ Puerto 31337 abierto."; }
cerrar_puerto_31337() { pkill -f 'nc -l 31337' && echo "🛑 Puerto 31337 cerrado."; }
abrir_servidor_http() { nohup python3 -m http.server 4444 > /dev/null 2>&1 & echo "🚨 Servidor HTTP en 4444 activo."; }
cerrar_servidor_http() { pkill -f 'python3 -m http.server 4444' && echo "🛑 Servidor HTTP cerrado."; }
alternar_puerto_77() { pkill -f 'nc -l 77'; sleep 1; nohup nc -l 77 > /dev/null 2>&1 & sleep 1; pkill -f 'nc -l 77'; echo "🔄 Puerto 77 alternado."; }

cleanup_and_exit() {
    echo ""
    echo "🧹 Deteniendo procesos antes de salir..."
    stop_cpu_hog
    stop_mem_hog
    echo "👋 Saliendo."
    exit 0
}

trap cleanup_and_exit SIGINT SIGTERM

# Menú interactivo si no se pasa argumento
while true; do
    show_menu
    read -rp "Selecciona una opción (0 para salir): " opc
    case $opc in
        1) crear_fake_usb ;;
        2) eliminar_fake_usb ;;
        3) agregar_archivo ;;
        4) eliminar_archivo ;;
        5) modificar_tamano ;;
        6) cambiar_permisos ;;
        7) renombrar_archivo ;;
        8) start_cpu_hog ;;
        9) stop_cpu_hog ;;
        10) start_mem_hog ;;
        11) stop_mem_hog ;;
        12) show_pids ;;
        13) ajustar_mem_size ;;
        14) abrir_ssh ;;
        15) cerrar_ssh ;;
        16) abrir_puerto_31337 ;;
        17) cerrar_puerto_31337 ;;
        18) abrir_servidor_http ;;
        19) cerrar_servidor_http ;;
        20) alternar_puerto_77 ;;
        0) cleanup_and_exit ;;
        *) echo "❌ Opción inválida. Intenta de nuevo." ;;
    esac
done
