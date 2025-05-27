# MatCom Guard

## División del Proyecto MatCom Guard en Tareas Específicas

### 1. Configuración Inicial y Estructura del Proyecto
- **Tarea 1.1**: Configurar el entorno de desarrollo en una máquina virtual UNIX (ej: Ubuntu).  
- **Tarea 1.2**: Definir la estructura del proyecto en C (ej: módulos para USB, procesos, puertos, interfaz).  
- **Tarea 1.3**: Crear un archivo de configuración (`/etc/matcomguard.conf`) con valores predeterminados (umbrales de CPU/RAM, lista blanca de procesos).  

---

### 2. Detección y Escaneo de Dispositivos USB (RF1)
- **Tarea 2.1**: Implementar monitoreo del directorio de montaje (ej: `/mnt/usb`) usando `inotify` para detectar cambios.  
- **Tarea 2.2**: Crear un sistema de baseline con hash SHA-256 al montar el dispositivo (usar `libssl` para cálculos de hash).  
- **Tarea 2.3**: Escanear recursivamente el sistema de archivos del USB y comparar con el baseline para detectar cambios (archivos nuevos, modificados, eliminados).  
- **Tarea 2.4**: Implementar alertas en tiempo real según umbrales (ej: cambios en >10% de archivos).  
- **Tarea 2.5**: Manejar excepciones (ej: ignorar archivos temporales o extensiones específicas).  

---

### 3. Monitoreo de Procesos e Hilos (RF2)
- **Tarea 3.1**: Leer datos de `/proc` (PID, nombre, uso de CPU/RAM) periódicamente.  
- **Tarea 3.2**: Calcular el uso de recursos entre iteraciones (comparar valores actuales y anteriores).  
- **Tarea 3.3**: Implementar detección de picos según umbrales (CPU > 70%, RAM > 50% por más de 10 segundos).  
- **Tarea 3.4**: Gestionar la lista blanca de procesos (ej: `gcc`, `gnome-shell`).  
- **Tarea 3.5**: Generar alertas cuando se superen los umbrales (excluyendo procesos en lista blanca).  

---

### 4. Escaneo de Puertos Locales (RF3)
- **Tarea 4.1**: Implementar un escáner TCP para puertos 1-1024 (usar sockets en C).  
- **Tarea 4.2**: Identificar servicios asociados a puertos abiertos (ej: SSH en puerto 22).  
- **Tarea 4.3**: Detectar puertos sospechosos (ej: 31337, 4444) y generar alertas.  
- **Tarea 4.4**: Incluir opción para escanear rangos personalizados (ej: `--scan-ports 1-5000`).  

---

### 5. Interfaz Gráfica y Consola (RF4)
- **Tarea 5.1**: Diseñar un menú de consola con 4 opciones:  
  - Escanear sistema de archivos.  
  - Escanear memoria.  
  - Escanear puertos.  
  - Escanear todo.  
- **Tarea 5.2**: Implementar generación de informes en consola (formato legible).  
- **Tarea 5.3**: Integrar exportación de informes a PDF (usar `libharu` o similar).  
- **Tarea 5.4** (Opcional): Desarrollar interfaz gráfica con GTK+ para visualización en tiempo real.  

---

### 6. Integración y Pruebas
- **Tarea 6.1**: Integrar módulos (USB, procesos, puertos) en una única aplicación.  
- **Tarea 6.2**: Realizar pruebas unitarias para cada funcionalidad (ej: inyectar cambios en USB, simular picos de CPU).  
- **Tarea 6.3**: Ejecutar casos de prueba definidos (ej: alertas por malware.exe, detección de puerto 31337).  
- **Tarea 6.4**: Optimizar el rendimiento (ej: reducir uso de recursos en monitoreo continuo).  

---

### 7. Documentación y Entrega
- **Tarea 7.1**: Escribir un manual de usuario (instalación, configuración, uso).  
- **Tarea 7.2**: Documentar el código (comentarios, estructura de archivos).  
- **Tarea 7.3**: Preparar el entregable (binario, archivos de configuración, dependencias).  

---

## Guía Paso a Paso para Configurar la Máquina Virtual (Ubuntu)

Sigue estos pasos para preparar tu entorno de desarrollo:

1. **Instalar Ubuntu**  
   - Usa VirtualBox o VMware para crear una máquina virtual con Ubuntu 22.04 LTS.  
   - Asigna al menos 4 GB de RAM y 20 GB de almacenamiento.  

2. **Actualizar el sistema**  
   ```bash
   sudo apt update && sudo apt upgrade -y
   ```

3. **Instalar herramientas esenciales**  
   ```bash
   sudo apt install build-essential gdb git curl wget -y
   ```

4. **Instalar dependencias para GTK+ (Interfaz Gráfica)**  
   ```bash
   sudo apt install libgtk-3-dev pkg-config -y
   ```

5. **Instalar bibliotecas para SHA-256 y monitoreo**  
   ```bash
   sudo apt install libssl-dev libprocps-dev -y
   ```

6. **Configurar permisos**  
   - Asegúrate de tener permisos para acceder a `/proc`, `/dev`, y directorios de montaje USB.  

---

## Estructura del Proyecto

Para mantener un repositorio y un proyecto limpio y bien estructurado, se recomienda la siguiente estructura de directorios:

```
matcomguard/
├── src/                   # Código fuente
│   ├── usb/               # Módulo de gestión de USB
│   ├── processes/         # Módulo de monitoreo de procesos
│   ├── ports/             # Módulo de escaneo de puertos
│   └── main.c             # Archivo principal
├── include/               # Archivos de encabezado
│   ├── usb.h              # Encabezado del módulo USB
│   ├── processes.h        # Encabezado del módulo de procesos
│   └── ports.h            # Encabezado del módulo de puertos
├── config/                # Archivos de configuración
│   └── matcomguard.conf   # Archivo de configuración principal
├── tests/                 # Pruebas unitarias
│   ├── test_usb.c         # Pruebas del módulo USB
│   ├── test_processes.c   # Pruebas del módulo de procesos
│   └── test_ports.c       # Pruebas del módulo de puertos
├── docs/                  # Documentación
│   └── user_manual.md      # Manual del usuario
├── Makefile               # Archivo Makefile para construir el proyecto
└── README.md              # Este archivo
```

---

### Dependencias y Prioridades
1. **Tareas 1.x y 2.x** son fundamentales para el núcleo del sistema.  
2. **Tareas 3.x y 4.x** pueden desarrollarse en paralelo.  
3. **Tarea 5.x** depende de las funcionalidades básicas implementadas.  
4. **Tareas 6.x y 7.x** son finales y requieren integración previa.  

### Posibles Riesgos
- Manejo de concurrencia en monitoreo en tiempo real.  
- Precisión en la detección de cambios en dispositivos USB.  
- Compatibilidad con diferentes distribuciones UNIX.  

Este desglose permite abordar el proyecto de forma modular, facilitando la asignación de responsabilidades y el seguimiento del progreso.