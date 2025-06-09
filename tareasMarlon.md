# Lista de tareas detalladas para RF3: Defensa de Puertos Estratégicos

1. **Diseñar el archivo de configuración**
   - Definir el formato (ejemplo: lista de puertos permitidos, IPs de confianza, umbrales).
   - Crear un archivo de ejemplo `/etc/matcomguard.conf`.

2. **Leer y parsear el archivo de configuración**
   - Implementar funciones para cargar puertos permitidos y excepciones desde el archivo.

3. **Leer el estado actual de los puertos**
   - Implementar funciones para leer y parsear `/proc/net/tcp`, `/proc/net/udp`, `/proc/net/tcp6`, `/proc/net/udp6`.
   - Extraer información relevante: IP local, puerto local, estado de la conexión, PID.

4. **Identificar puertos abiertos y conexiones activas**
   - Listar todos los puertos en estado LISTEN y conexiones establecidas.
   - Asociar cada puerto/proceso con su PID y nombre de proceso (usando `/proc/[pid]/cmdline`).

5. **Comparar con la configuración**
   - Verificar si los puertos abiertos están permitidos.
   - Verificar si las conexiones activas son a/desde IPs permitidas.
   
6. **Detectar anomalías**
   - Marcar puertos abiertos no autorizados.
   - Marcar conexiones a IPs o puertos no permitidos.
   - (Opcional) Detectar patrones sospechosos (ej: muchos puertos abiertos en poco tiempo).

7. **Generar y registrar alertas**
   - Implementar función para mostrar o guardar alertas cuando se detecte una anomalía.
   - Registrar detalles: fecha, hora, puerto, proceso, IP, tipo de alerta.

8. **(Opcional) Tomar acciones automáticas**
   - Implementar función para bloquear puertos o IPs usando iptables/firewalld (requiere root).
   - (Opcional) Cerrar procesos sospechosos.

9. **Crear un ciclo de monitoreo periódico**
   - Ejecutar el monitoreo cada X segundos/minutos (configurable).
   - Mantener bajo consumo de recursos.

10. **Proteger el propio sistema de monitoreo**
    - Asegurarse de que el proceso no pueda ser fácilmente terminado o modificado por usuarios no autorizados.

11. **Documentar el código y el uso**
    - Escribir comentarios y documentación para facilitar el mantenimiento y la configuración.

12. **Lo que tengo hecho**
    - Parseo de archivos tcp, udp, tcp6, udp6. Esto permite obtener todas las conexiones y puertos abiertos en el sistema, tanto IPv4 como IPv6, TCP y UDP. Esto esta implementado mediante funciones que leen cada archivo (parse_tcp_line4() y parse_tcp_line6()) y estas funciones llenar arrays de estructuras (FileEntry4 y  FileEntry6) con la informacion relevante (IP, puerto, estado, UID, inode).
    - Carga de configuracion en el archivo matcomguard.conf. Define las reglas de seguridad: puertos permitidos, IPs confiables, procesos permitidos, umbrales, etc. Esto esta implementado con una funcion que lee la configuracion y  la guarda en una estructura PortGuardConfig.
    - Analisis de puertos y conexiones. Esto es para detectar violaciones de seguridad (puertos no permitidos, conexiones sospechosas). En la implementacion tengo la funcion analyze_ports() que compara cada puerto/conexion con la configuracion y genera alertas si encuentra algo no permitido.
    - Asociacion de puertos a procesos. Permite saber que procesos estan usando cada puerto/conexion es fundamental para la seguridad y para tomar acciones. En la implementacion esta la funcion find_process_by_inode() que busca el PID y el nombre del proceso a partid del inode del socket, recorriendo /proc/[pid]/fd/.
    