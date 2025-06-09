#!/bin/bash

clear
echo "=== Tester interactivo para MatcomGuard ==="
echo "Ejecuta acciones en puertos y verifica cambios en tiempo real"
echo "============================================================="
echo ""

while true; do
    echo "Opciones:"
    echo "1) 🔓 Abrir SSH en puerto 22 (Servicio legítimo)"
    echo "2) 🔒 Cerrar SSH en puerto 22"
    echo "3) ⚠️ Abrir puerto sospechoso (31337)"
    echo "4) 🛑 Cerrar puerto sospechoso (31337)"
    echo "5) 🚨 Levantar servidor HTTP en puerto no estándar (4444)"
    echo "6) 🛑 Cerrar servidor HTTP en puerto no estándar (4444)"
    echo "7) 🔄 Alternar puerto 77 (Abierto/Cerrado rápidamente)"
    echo "8) ⏹️ Salir"
    echo ""

    read -p "Selecciona una opción: " option

    case $option in
        1) 
            echo "🔓 Abriendo servicio SSH en puerto 22..."
            sudo systemctl start sshd
            ;;
        2) 
            echo "🔒 Cerrando servicio SSH en puerto 22..."
            sudo systemctl stop sshd
            ;;
        3) 
            echo "⚠️ Abriendo puerto sospechoso 31337..."
            nohup nc -l 31337 > /dev/null 2>&1 &
            ;;
        4) 
            echo "🛑 Cerrando puerto sospechoso 31337..."
            pkill -f 'nc -l 31337'
            ;;
        5) 
            echo "🚨 Levantando servidor HTTP en puerto no estándar 4444..."
            nohup python3 -m http.server 4444 > /dev/null 2>&1 &
            ;;
        6) 
            echo "🛑 Cerrando servidor HTTP en puerto 4444..."
            pkill -f 'python3 -m http.server 4444'
            ;;
        7) 
            echo "🔄 Alternando puerto 77 (Abierto/Cerrado rápidamente)..."
            pkill -f 'nc -l 77'
            sleep 2
            nohup nc -l 77 > /dev/null 2>&1 &
            sleep 2
            pkill -f 'nc -l 77'
            ;;
        8) 
            echo "⏹️ Saliendo del tester..."
            exit 0
            ;;
        *)
            echo "❌ Opción inválida, intenta de nuevo."
            ;;
    esac

    echo "✅ Acción ejecutada. Observa los cambios en MatcomGuard."
    echo ""
    sleep 3
    clear
done
