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

// Variables globales
Transaccion transacciones[MAX_TRANSACCIONES];
int num_transacciones = 0;

// Declaración de prototipos de funciones
void cargar_transacciones();
void enviar_alerta(const char *mensaje);
void *detectar_transferencias_grandes(void *arg);
void *detectar_retiros_consecutivos(void *arg);
void *detectar_transferencias_repetidas(void *arg);


// Función principal que inicializa configuración y semáforos, y ejecuta continuamente la detección de anomalías cada 5 minutos.
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

        sleep(300);
    }
    
    destruir_semaforos();

    return 0;
}


// Carga las transacciones desde el archivo especificado en la configuración.
// Valida el formato de cada línea y almacena las transacciones válidas en el arreglo global, para que los hilos las analicen.
void cargar_transacciones() {

    EscribirLog("cargando transacciones");

    sem_wait(semaforo_transacciones);  // Protección para lectura segura

    FILE *f = fopen(configuracion.archivo_transacciones, "r");
    if (!f) {
        perror("Error abriendo transacciones.txt");
        EscribirLog("Error abriendo transacciones.txt");
        sem_post(semaforo_transacciones);  // Liberar antes de salir
        exit(EXIT_FAILURE);
    }

    num_transacciones = 0;  // Reiniciar el contador

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
    sem_post(semaforo_transacciones);  // Liberar
}


// Envía una alerta al proceso banco mediante una cola de mensajes.
// Luego, lanza una señal para notificar que hay una alerta disponible.
void enviar_alerta(const char *mensaje) {
    sem_wait(semaforo_alertas);
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
    
    sem_post(semaforo_alertas);  // SIEMPRE se hace, ocurra o no error
    
    kill(getppid(), SIGUSR1);
}


// Anomalía 1: Transferencias de ≥ 20000
// Si ocurre, genera y envía una alerta al proceso banco
void *detectar_transferencias_grandes(void *arg) {

    EscribirLog("Buscando anomalia de transferencia sospechosa...");

    for (int i = 0; i < num_transacciones; i++) {
        if (strcmp(transacciones[i].tipo, "TRANSFERENCIA") == 0 &&
            transacciones[i].cantidad >= configuracion.limite_transferencia) {
            
            char alerta[256];
            snprintf(alerta, sizeof(alerta), "TRANSFERENCIA GRANDE: %d€ de %d a %d el %s\n", transacciones[i].cantidad, transacciones[i].origen, transacciones[i].destino, transacciones[i].fecha);
            EscribirLog("Anomalia de transferencia sospechosa encontrada");
            enviar_alerta(alerta);
        }
    }
    return NULL;
}


// Anomalía 2: Tres retiros consecutivos en un mismo día
// Si ocurre, genera y envía una alerta al proceso banco
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
                    EscribirLog("Anomalia de retiros consecutivos encontrada");
                    enviar_alerta(alerta);
                    break;
                }
            }
        }
    }
    return NULL;
}


// Anomalía 3: Tres transferencias entre mismas cuentas en un día
// Si ocurre, genera y envía una alerta al proceso banco
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
                    snprintf(alerta, sizeof(alerta), "TRANSFERENCIAS CONSECUTIVAS: %d transferencias de %d a %d el %s\n", contador, cuenta_origen, cuenta_destino, fecha);
                    EscribirLog("Anomalia de transferencias consecutivas encontrada");
                    enviar_alerta(alerta);
                    break;
                }
            }
        }
    }
    return NULL;
}


