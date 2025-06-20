#!/bin/bash

# Script para probar el monitoreo de procesos (CPU y RAM)
# Dependencias: dd, tail, kill, sleep

CPU_HOG_PID=""
MEM_HOG_PID=""
DEFAULT_MEM_MB=300

function show_menu() {
    echo ""
    echo "=== Menú de simulación de carga de procesos ==="
    echo "1) Iniciar CPU hog (uso intenso de CPU en background)"
    echo "2) Detener CPU hog"
    echo "3) Iniciar Memory hog (uso intenso de RAM en background)"
    echo "4) Detener Memory hog"
    echo "5) Mostrar PIDs activos"
    echo "6) Ajustar tamaño de Memory hog (MB) [actual: $DEFAULT_MEM_MB MB]"
    echo "0) Salir"
    echo "=============================================="
}

function start_cpu_hog() {
    if [[ -n "$CPU_HOG_PID" && -e "/proc/$CPU_HOG_PID" ]]; then
        echo "⚠️  Ya hay un CPU hog corriendo con PID $CPU_HOG_PID"
        return
    fi
    ( while :; do :; done ) &
    CPU_HOG_PID=$!
    echo "✅ CPU hog iniciado con PID $CPU_HOG_PID"
}

function stop_cpu_hog() {
    if [[ -n "$CPU_HOG_PID" ]]; then
        if kill "$CPU_HOG_PID" 2>/dev/null; then
            echo "🛑 CPU hog (PID $CPU_HOG_PID) detenido."
        else
            echo "⚠️  No se pudo detener el CPU hog (PID $CPU_HOG_PID)."
        fi
        CPU_HOG_PID=""
    else
        echo "ℹ️  No hay CPU hog activo."
    fi
}

function start_mem_hog() {
    if [[ -n "$MEM_HOG_PID" && -e "/proc/$MEM_HOG_PID" ]]; then
        echo "⚠️  Ya hay un Memory hog corriendo con PID $MEM_HOG_PID"
        return
    fi
    # Reserva memoria y la mantiene en uso con tail -f
    ( dd if=/dev/zero bs=1M count="$DEFAULT_MEM_MB" | tail -f /dev/null ) &
    MEM_HOG_PID=$!
    echo "✅ Memory hog iniciado con PID $MEM_HOG_PID (~$DEFAULT_MEM_MB MB reservados)"
}

function stop_mem_hog() {
    if [[ -n "$MEM_HOG_PID" ]]; then
        if kill "$MEM_HOG_PID" 2>/dev/null; then
            echo "🛑 Memory hog (PID $MEM_HOG_PID) detenido."
        else
            echo "⚠️  No se pudo detener el Memory hog (PID $MEM_HOG_PID)."
        fi
        MEM_HOG_PID=""
    else
        echo "ℹ️  No hay Memory hog activo."
    fi
}

function show_pids() {
    echo "=== Estado de hogs activos ==="
    if [[ -n "$CPU_HOG_PID" && -e "/proc/$CPU_HOG_PID" ]]; then
        echo "CPU hog activo: PID $CPU_HOG_PID"
    else
        echo "CPU hog: detenido"
    fi
    if [[ -n "$MEM_HOG_PID" && -e "/proc/$MEM_HOG_PID" ]]; then
        echo "Memory hog activo: PID $MEM_HOG_PID (~${DEFAULT_MEM_MB} MB)"
    else
        echo "Memory hog: detenido"
    fi
}

function adjust_mem_size() {
    read -rp "🧮 Ingresa nuevo tamaño de RAM en MB: " val
    if [[ "$val" =~ ^[0-9]+$ && "$val" -gt 0 ]]; then
        DEFAULT_MEM_MB=$val
        echo "✅ Nuevo tamaño para Memory hog: $DEFAULT_MEM_MB MB"
    else
        echo "❌ Valor inválido."
    fi
}

function cleanup_and_exit() {
    echo ""
    echo "🧹 Deteniendo procesos antes de salir..."
    stop_cpu_hog
    stop_mem_hog
    echo "👋 Saliendo."
    exit 0
}

trap cleanup_and_exit SIGINT SIGTERM

# Bucle principal del menú
while true; do
    show_menu
    read -rp "Selecciona una opción: " opc
    case $opc in
        1) start_cpu_hog ;;
        2) stop_cpu_hog ;;
        3) start_mem_hog ;;
        4) stop_mem_hog ;;
        5) show_pids ;;
        6) adjust_mem_size ;;
        0) cleanup_and_exit ;;
        *) echo "❌ Opción inválida." ;;
    esac
done
