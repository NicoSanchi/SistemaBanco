#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include "comun.h"

#define TAM_BUFFER 1024


// Función principal que analiza las transacciones en busca de anomalías. El parámetro es el descriptor de escritura del pipe para enviar alertas
void analizar_transacciones(int pipe_monitor) {
    
    inicializar_configuracion();

    // Abrir archivo de transacciones en modo lectura
    FILE *archivo = fopen(configuracion.archivo_transacciones, "r");
    if (archivo == NULL) {
        perror("Error al abrir el archivo de transacciones");
        EscribirLog("Fallo al abrir el archivo de transacciones");
        return;
    }
    else
        EscribirLog("Monitor ha abierto el archivo de transacciones");

    char linea[TAM_BUFFER];
    int contador_retiros[TAM_BUFFER] = {0}; // Contador de retiros por cuenta
    int transferencias_consecutivas = 0;
    int ultima_cuenta_origen = -1;
    int ultima_cuenta_destino = -1;

    // Leer archivo línea por línea
    while (fgets(linea, sizeof(linea), archivo)) {
        // Extraer tipo de operación (RETIRO/TRANSFERENCIA)
        char *tipo = strstr(linea, "] ");
        if (!tipo) continue;
        tipo += 2;

        // Detección de retiros sospechosos
        if (strstr(tipo, "RETIRO")) {
            int cuenta;
            char *token = strstr(linea, "Cuenta: ");
            if (token && sscanf(token, "Cuenta: %d", &cuenta) == 1) {
                contador_retiros[cuenta % TAM_BUFFER]++;
                
                // Verificar si supera el umbral configurado
                if (contador_retiros[cuenta % TAM_BUFFER] > configuracion.umbral_retiros) {
                    char alerta[TAM_BUFFER];
                    snprintf(alerta, sizeof(alerta), "ALERTA: Demasiados retiros consecutivos en cuenta %d", cuenta);
                    write(pipe_monitor, alerta, strlen(alerta) + 1); // Enviar alerta por el pipe
                    contador_retiros[cuenta % TAM_BUFFER] = 0; // Resetear contador

                    EscribirLog("Monitor ha detectado una anomalía en transacciones.txt (retiros consecutivos)");
                }
            }
        }
        // Detección de transferencias sospechosas
        else if (strstr(tipo, "TRANSFERENCIA")) {
            int cuenta_origen, cuenta_destino;
            char *token = strstr(linea, "Cuenta Origen: ");
            if (token && sscanf(token, "Cuenta Origen: %d", &cuenta_origen) == 1) {
                token = strstr(token, "Cuenta Destino: ");
                if (token && sscanf(token, "Cuenta Destino: %d", &cuenta_destino) == 1) {
                    // Verificar si es entre las mismas cuentas
                    if (cuenta_origen == ultima_cuenta_origen && cuenta_destino == ultima_cuenta_destino) {
                        transferencias_consecutivas++;
                        if (transferencias_consecutivas > configuracion.umbral_transferencias) {
                            char alerta[TAM_BUFFER];
                            snprintf(alerta, sizeof(alerta), "ALERTA: Transferencias consecutivas entre cuentas %d %d", 
                                    cuenta_origen, cuenta_destino);
                            write(pipe_monitor, alerta, strlen(alerta) + 1);
                            transferencias_consecutivas = 0;

                            EscribirLog("Monitor ha detectado una anomalía en transacciones.txt (transferencias consecutivas entre cuentas)");
                        }
                    } else {
                        // Actualizar cuentas para la próxima comparación
                        transferencias_consecutivas = 1;
                        ultima_cuenta_origen = cuenta_origen;
                        ultima_cuenta_destino = cuenta_destino;
                    }
                }
            }
        }
    }
    fclose(archivo);
    EscribirLog("Monitor ha cerrado el archivo de transacciones");
}

// Punto de entrada del monitor. argv[1]: Descriptor del pipe pasado como argumento
int main(int argc, char *argv[]) {
    if (argc < 2) {
        exit(EXIT_FAILURE); // Salir si no se proporcionó el descriptor
    }
    
    // Convertir argumento a descriptor de archivo
    int pipe_monitor = atoi(argv[1]);
    
    // Bucle de análisis continuo
    while (1) {
        analizar_transacciones(pipe_monitor);
        sleep(5); // Esperar 5 segundos entre análisis
    }
    return 0;
}