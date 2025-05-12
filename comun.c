#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/shm.h>
#include "comun.h"

// Definiciones de los semáforos
sem_t *semaforo_cuentas = NULL;
sem_t *semaforo_log = NULL;
sem_t *semaforo_transacciones = NULL;
sem_t *semaforo_alertas = NULL;

// Definición global para configuración
Config configuracion;

// Definición para la tabla de cuentas
TablaCuentas *tabla = NULL;

//Definición de la variable
int shm_id;

// Definición de la variable global
int id_cola = -1;  // -1 indica que no está conectada


void inicializar_semaforos(){
    semaforo_cuentas = sem_open("/semaforo_cuentas", O_CREAT, 0644, 1); // Inicializa un semaforo para cuentas en 1
    semaforo_log = sem_open("/semaforo_log", O_CREAT, 0644, 1); // Inicializa un semaforo para log en 1
    semaforo_transacciones = sem_open("/semaforo_transacciones", O_CREAT, 0644, 1); // Inicializa un semaforo para transacciones en 1
    semaforo_alertas = sem_open("/semaforo_alertas", O_CREAT, 0644, 1);

    // Comprueba que no falle ni un semaforo
    if(semaforo_cuentas== SEM_FAILED || semaforo_log == SEM_FAILED || semaforo_transacciones == SEM_FAILED || semaforo_alertas == SEM_FAILED){
        perror("⚠Ha ocurrido un error a la hora de crear los semáforos");
        EscribirLog("Error al crear los semáforos");
        exit(EXIT_FAILURE);
    }
}

void conectar_semaforos(){
    // Abre los semaforos
    semaforo_cuentas = sem_open("/semaforo_cuentas", 0);
    semaforo_log = sem_open("/semaforo_log", 0);
    semaforo_transacciones = sem_open("/semaforo_transacciones", 0);
    semaforo_alertas = sem_open("/semaforo_alertas", 0);

    // Comprueba que no falle ni un semaforo
    if(semaforo_cuentas == SEM_FAILED || semaforo_log == SEM_FAILED || semaforo_transacciones == SEM_FAILED  || semaforo_alertas == SEM_FAILED){
        perror("⚠ Error al conectar con los semáforos existentes");
        EscribirLog("Error al conectar los semáforos");
        exit(EXIT_FAILURE);
    }
}

void destruir_semaforos(){
    // Cierra los semaforos
    sem_close(semaforo_cuentas);
    sem_close(semaforo_log);
    sem_close(semaforo_transacciones);
    sem_close(semaforo_alertas);
    // Elimina los semaforos
    sem_unlink("/semaforo_cuentas");
    sem_unlink("/semaforo_log");
    sem_unlink("/semaforo_transacciones");
    sem_unlink("/semaforo_alertas");

    EscribirLog("Los semáforos han sido destruidos");
}

Config leer_configuracion(const char *ruta)
{
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL)
    {
        perror("Error al abrir config.txt");
        exit(1);
    }
    Config config;
    char linea[100];
    while (fgets(linea, sizeof(linea), archivo))
    {
        if (linea[0] == '#' || strlen(linea) < 3)
            continue; // Ignorar comentarios y líneas vacías
        if (strstr(linea, "LIMITE_RETIRO"))
            sscanf(linea, "LIMITE_RETIRO=%d", &config.limite_retiro);
        else if (strstr(linea, "LIMITE_TRANSFERENCIA"))
            sscanf(linea, "LIMITE_TRANSFERENCIA=%d", &config.limite_transferencia);
        else if (strstr(linea, "UMBRAL_RETIROS"))
            sscanf(linea, "UMBRAL_RETIROS=%d", &config.umbral_retiros);
        else if (strstr(linea, "UMBRAL_TRANSFERENCIAS"))
            sscanf(linea, "UMBRAL_TRANSFERENCIAS=%d", &config.umbral_transferencias);
        else if (strstr(linea, "NUM_HILOS"))
            sscanf(linea, "NUM_HILOS=%d", &config.num_hilos);
        else if (strstr(linea, "ARCHIVO_CUENTAS"))
            sscanf(linea, "ARCHIVO_CUENTAS=%s", config.archivo_cuentas);
        else if (strstr(linea, "ARCHIVO_LOG"))
            sscanf(linea, "ARCHIVO_LOG=%s", config.archivo_log);
        else if (strstr(linea, "ARCHIVO_TRANSACCIONES"))
            sscanf(linea, "ARCHIVO_TRANSACCIONES=%s", config.archivo_transacciones);
        else if (strstr(linea, "ARCHIVO_ALERTAS"))
            sscanf(linea, "ARCHIVO_ALERTAS=%s", config.archivo_alertas);     
    }
    fclose(archivo);
    return config;
}

void inicializar_configuracion() {
    configuracion = leer_configuracion("config.txt");  
}

void EscribirLog(const char *mensaje) { // Recibe un mensaje que será el que aparecerá en el log
    FILE* fichero;
    sem_wait(semaforo_log);
    fichero = fopen("registro.log", "a"); // Abrimos el archivo

    if (fichero == NULL){
        sem_post(semaforo_log);
        return;
    }        

    time_t tiempo; // Creamos una varible para tomar la hora a la que se escribio el log
    struct tm *tm_info;
    char hora[26];

    time(&tiempo);
    tm_info = localtime(&tiempo);
    strftime(hora, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(fichero, "[%s] %s\n", hora, mensaje);

    fclose(fichero);
    sem_post(semaforo_log);
}

void CrearColaMensajes() {
    id_cola = msgget(CLAVE_COLA_MENSAJES, IPC_CREAT | 0666);  // Crea o conecta
    if (id_cola == -1) {
        perror("Error al crear/conectar la cola de mensajes");
        EscribirLog("Error al crear/conectar la cola de mensajes");
        exit(EXIT_FAILURE);
    }
    EscribirLog("Cola de mensajes creada/conectada correctamente");
}

void ConectarColaMensajes() {
    id_cola = msgget(CLAVE_COLA_MENSAJES, 0666);  // Solo conecta (sin IPC_CREAT)
    if (id_cola == -1) {
        perror("Error al conectar a la cola de mensajes existente");
        EscribirLog("Error al conectar a la cola de mensajes");
        exit(EXIT_FAILURE);
    }
}

void DestruirColaMensajes() {
    if (id_cola != -1) {
        if (msgctl(id_cola, IPC_RMID, NULL) == -1) {  // Elimina la cola
            perror("Error al destruir la cola de mensajes");
            EscribirLog("Error al destruir la cola de mensajes");
        } else {
            EscribirLog("Cola de mensajes destruida correctamente");
            id_cola = -1;  // Marca como no válida
        }
    }
}

void CrearMemoriaCompartida() {
    FILE *archivo;
    int i = 0;
    shm_id = shmget(IPC_PRIVATE, sizeof(TablaCuentas), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Error a la hora de crear la memoria compartida.");
        EscribirLog("Ha ocurrido un error a la hora de crear la memoria compartida.");
        exit(EXIT_FAILURE);
    }
    tabla = (TablaCuentas *)shmat(shm_id, NULL, 0);
    if(tabla == (void *)-1) { 
        perror("Error al asociar la memoria compartida al proceso");
        EscribirLog("Error al asociar la memoria compartida al proceso");
        exit(EXIT_FAILURE);
    }
    // Inicializamos en 0.
    memset(tabla, 0, sizeof(TablaCuentas));

    archivo = fopen(configuracion.archivo_cuentas, "r");
    if(archivo == NULL){
        perror("Error a la hora de abrir el archivo de cuentas.");
        EscribirLog("Ha ocurrido un error a la hora de abrir el archivo de cuentas.");
        exit(EXIT_FAILURE);
    }

    while(i < CUENTAS_TOTALES && fscanf(archivo, "%d,%49[^,],%f,%d", &tabla->cuentas[i].numero_cuenta, tabla->cuentas[i].titular, &tabla->cuentas[i].saldo, &tabla->cuentas[i].num_transacciones))
    {
        i++;
    }

    fclose(archivo);

    return;

}

void LiberarMemoriaCompartida(){
    shmdt(tabla); // Separamos la región del espacio de direccionamiento del proceso.
    shmctl(shm_id, IPC_RMID, 0); // Eliminamos la región de memoria compartida.
    return;
}
