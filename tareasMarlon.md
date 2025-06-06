# Lista de tareas detalladas para RF3: Defensa de Puertos Estratégicos

1. **Diseñar el archivo de configuración**
   - Definir el formato (ejemplo: lista de puertos permitidos, IPs de confianza, umbrales).
   - Crear un archivo de ejemplo `/etc/matcomguard.conf`.
   
   ***Objetivo:*** El objetivo es crear un archivo de configuración centralizado que permita definir, de forma flexible y editable, los parámetros de seguridad de tu sistema. Este archivo servirá para que el usuario (o administrador) pueda especificar:
      1. Qué puertos están permitidos (puertos abiertos autorizados).
      2. Qué IPs son de confianza (permitidas para conexiones).
      3. Umbrales de alerta (por ejemplo, cuántos puertos abiertos son sospechosos).
      4. Otras excepciones o parámetros relevantes.
   
   Esto hace que tu sistema sea configurable sin recompilar el código y adaptable a diferentes entornos o necesidades.

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