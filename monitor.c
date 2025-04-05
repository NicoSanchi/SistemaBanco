#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "comun.h"

// Estructuras
typedef struct {
    char fecha[11]; // YYYY-MM-DD
    char tipo[20];  // TRANSFERENCIA, DEPOSITO, RETIRO
    int origen;
    int destino; // -1 si no aplica
    int cantidad;
} Transaccion;

#define MAX_TRANSACCIONES 1000
Transaccion transacciones[MAX_TRANSACCIONES];
int num_transacciones = 0;


// -------------------- FUNCIONES ---------------------

void cargar_transacciones() {

    EscribirLog("cargando transacciones");

    FILE *f = fopen(configuracion.archivo_transacciones, "r");
    if (!f) {
        perror("Error abriendo transacciones.txt");
        exit(EXIT_FAILURE);
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), f)) {
        Transaccion t;
        char tipo[20];
        int campos = sscanf(linea, "%10[^,],%19[^,],%d,%d,%d", t.fecha, tipo, &t.origen, &t.destino, &t.cantidad);
        
        strcpy(t.tipo, tipo);

        if (strcmp(tipo, "TRANSFERENCIA") == 0 && campos == 5) {
            // ok
        } else if ((strcmp(tipo, "RETIRO") == 0 || strcmp(tipo, "DEPOSITO") == 0) && campos == 4) {
            t.destino = -1;
            t.cantidad = t.destino;
            sscanf(linea, "%10[^,],%19[^,],%d,%d", t.fecha, tipo, &t.origen, &t.cantidad);
        } else {
            continue; // línea malformada
        }

        if (num_transacciones < MAX_TRANSACCIONES)
            transacciones[num_transacciones++] = t;
    }

    fclose(f);
}

// Enviar alerta por cola de mensajes
void enviar_alerta(const char *mensaje) {
    ConectarColaMensajes();

    // Preparar el mensaje
    MensajeAlerta msg;
    msg.tipo = TIPO_ALERTA;
    strncpy(msg.texto, mensaje, sizeof(msg.texto));

    // Enviar el mensaje
    if (msgsnd(id_cola, &msg, sizeof(msg.texto), 0) == -1) {
        perror("Error al enviar alerta");
        EscribirLog("Fallo al enviar la alerta");
    } 
    else
        EscribirLog("Alerta enviada a banco");
    
    kill(getppid(), SIGUSR1);
}

// Anomalía 1: Transferencias de ≥ 20000
void *detectar_transferencias_grandes(void *arg) {

    EscribirLog("Buscando anomalia de transferencia sospechosa...");

    for (int i = 0; i < num_transacciones; i++) {
        if (strcmp(transacciones[i].tipo, "TRANSFERENCIA") == 0 &&
            transacciones[i].cantidad >= configuracion.limite_transferencia) {
            
            char alerta[256];
            snprintf(alerta, sizeof(alerta),
                     "TRANSFERENCIA GRANDE: %d€ de %d a %d el %s\n",
                     transacciones[i].cantidad,
                     transacciones[i].origen,
                     transacciones[i].destino,
                     transacciones[i].fecha);
            enviar_alerta(alerta);
            EscribirLog("Anomalia de transferencia sospechosa encontrada");
        }
    }
    return NULL;
}

// Anomalía 2: Tres retiros consecutivos en un mismo día
void *detectar_retiros_consecutivos(void *arg) {

    EscribirLog("Buscando anomalia de retiros consecutivos...");


    for (int i = 0; i < num_transacciones; i++) {
        if (strcmp(transacciones[i].tipo, "RETIRO") != 0) continue;

        int cuenta = transacciones[i].origen;
        char *fecha = transacciones[i].fecha;
        int contador = 1;

        for (int j = i + 1; j < num_transacciones; j++) {
            if (strcmp(transacciones[j].fecha, fecha) == 0 &&
                strcmp(transacciones[j].tipo, "RETIRO") == 0 &&
                transacciones[j].origen == cuenta) {
                contador++;
                if (contador >= configuracion.umbral_retiros) {
                    char alerta[256];
                    snprintf(alerta, sizeof(alerta), "RETIROS CONSECUTIVOS: %d retiros el %s por la cuenta %d\n", contador, fecha, cuenta);
                    enviar_alerta(alerta);
                    EscribirLog("Anomalia de retiros consecutivos encontrada");
                    break;
                }
            }
        }
    }
    return NULL;
}

// Anomalía 3: Tres transferencias entre mismas cuentas en un día
void *detectar_transferencias_repetidas(void *arg) {

    EscribirLog("Buscando anomalia de transferencias consecutivas...");

    for (int i = 0; i < num_transacciones; i++) {
        if (strcmp(transacciones[i].tipo, "TRANSFERENCIA") != 0) continue;

        int cuenta_origen = transacciones[i].origen;
        int cuenta_destino = transacciones[i].destino;
        char *fecha = transacciones[i].fecha;
        int contador = 1;

        for (int j = i + 1; j < num_transacciones; j++) {
            if (strcmp(transacciones[j].tipo, "TRANSFERENCIA") == 0 &&
                strcmp(transacciones[j].fecha, fecha) == 0 &&
                transacciones[j].origen == cuenta_origen &&
                transacciones[j].destino == cuenta_destino) {
                contador++;
                if (contador >= configuracion.umbral_transferencias) {
                    char alerta[256];
                    snprintf(alerta, sizeof(alerta),
                             "TRANSFERENCIAS CONSECUTIVAS: %d transferencias de %d a %d el %s\n",
                             contador, cuenta_origen, cuenta_destino, fecha);
                    enviar_alerta(alerta);
                    EscribirLog("Anomalia de transferencias consecutivas encontrada");
                    break;
                }
            }
        }
    }
    return NULL;
}

// -------------------- MAIN ---------------------
int main(int argc, char *argv[]) {
    
    inicializar_configuracion();
    inicializar_semaforos();
    conectar_semaforos();

    while(1) {
        cargar_transacciones();

        pthread_t hilos[3];
        pthread_create(&hilos[0], NULL, detectar_transferencias_grandes, NULL);
        pthread_create(&hilos[1], NULL, detectar_retiros_consecutivos, NULL);
        pthread_create(&hilos[2], NULL, detectar_transferencias_repetidas, NULL);

        for (int i = 0; i < 3; i++)
            pthread_join(hilos[i], NULL);

        sleep(200);
    }
    
    destruir_semaforos();

    return 0;
}
