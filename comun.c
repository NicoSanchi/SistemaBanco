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
sem_t *semaforo_memoria_compartida = NULL;
sem_t *semaforo_buffer = NULL;

// Definición global para configuración
Config configuracion;

// Definición para la tabla de cuentas
TablaCuentas *tabla = NULL;

// Definición para el buffer
//bufferEstructurado buffer;

//Definición de la variable
int shm_id;

// Definición de la variable global
int id_cola = -1;  // -1 indica que no está conectada
int id_cola_cuentas = -1; // -1 indica que no está conectada


void inicializar_semaforos(){
    semaforo_cuentas = sem_open("/semaforo_cuentas", O_CREAT, 0644, 1); // Inicializa un semaforo para cuentas en 1
    semaforo_log = sem_open("/semaforo_log", O_CREAT, 0644, 1); // Inicializa un semaforo para log en 1
    semaforo_transacciones = sem_open("/semaforo_transacciones", O_CREAT, 0644, 1); // Inicializa un semaforo para transacciones en 1
    semaforo_alertas = sem_open("/semaforo_alertas", O_CREAT, 0644, 1); // Inicializa un semáforo para las alertas en 1.
    semaforo_memoria_compartida = sem_open("/semaforo_memoria_compartida", O_CREAT, 0644, 1); // Inicializa un semáforo para la memoria compartida en 1.
    semaforo_buffer = sem_open("/semaforo_buffer", O_CREAT, 0644, 1); // Inicializa un semáforo para la memoria compartida en 1.

    // Comprueba que no falle ni un semaforo
    if(semaforo_cuentas== SEM_FAILED || semaforo_log == SEM_FAILED || semaforo_transacciones == SEM_FAILED || semaforo_alertas == SEM_FAILED || semaforo_memoria_compartida == SEM_FAILED || semaforo_buffer == SEM_FAILED){
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
    semaforo_memoria_compartida = sem_open("/semaforo_memoria_compartida", 0);
    semaforo_buffer = sem_open("/semaforo_buffer", 0);

    // Comprueba que no falle ni un semaforo
    if(semaforo_cuentas == SEM_FAILED || semaforo_log == SEM_FAILED || semaforo_transacciones == SEM_FAILED  || semaforo_alertas == SEM_FAILED || semaforo_memoria_compartida == SEM_FAILED || semaforo_buffer == SEM_FAILED){
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
    sem_close(semaforo_memoria_compartida);
    sem_close(semaforo_buffer);
    // Elimina los semaforos
    sem_unlink("/semaforo_cuentas");
    sem_unlink("/semaforo_log");
    sem_unlink("/semaforo_transacciones");
    sem_unlink("/semaforo_alertas");
    sem_unlink("/semaforo_memoria_compartida");
    sem_unlink("/semaforo_buffer");

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
        else if (strstr(linea, "MAX_CUENTAS"))
            sscanf(linea, "MAX_CUENTAS=%d", &config.max_cuentas);
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

    id_cola_cuentas = msgget(CLAVE_COLA_CUENTAS, IPC_CREAT | 0666);
    if (id_cola_cuentas == -1) {
        perror("Error al crear la cola de mensajes para las cuentas");
        EscribirLog("Error al crear la cola de mensajes para las cuentas");
        exit(EXIT_FAILURE);
    }
    EscribirLog("La cola de mensajes para las cuentas ha sido conectada correctamente.");
}

void ConectarColaMensajes() {
    id_cola = msgget(CLAVE_COLA_MENSAJES, 0666);  // Solo conecta (sin IPC_CREAT)
    if (id_cola == -1) {
        perror("Error al conectar a la cola de mensajes existente");
        EscribirLog("Error al conectar a la cola de mensajes");
        exit(EXIT_FAILURE);
    }
    EscribirLog("La cola se ha conectado correctamente");
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
    if (id_cola_cuentas != -1) {
        if (msgctl(id_cola_cuentas, IPC_RMID, NULL) == -1) {  // Elimina la cola
            perror("Error al destruir la cola de mensajes");
            EscribirLog("Error al destruir la cola de mensajes");
        } else {
            EscribirLog("Cola de mensajes destruida correctamente");
            id_cola_cuentas = -1;  // Marca como no válida
        }
    }
}

void MeterCuentaBuffer(struct Cuenta cuenta) {
    id_cola_cuentas= msgget(CLAVE_COLA_CUENTAS, 0666);  // Solo conecta (sin IPC_CREAT)
    if (id_cola_cuentas == -1) {
        perror("Error al conectar a la cola de mensajes existente");
        EscribirLog("Error al conectar a la cola de mensajes");
        exit(EXIT_FAILURE);
    }
    bufferEstructurado buffer;
    memset(&buffer, 0, sizeof(bufferEstructurado));

    if(buffer.num_cuentas < 10){
        buffer.operacion[buffer.num_cuentas] = cuenta;
        buffer.num_cuentas++;
    
        struct {
            long tipo;
            bufferEstructurado buffer;
        } MensajeCuenta;

        MensajeCuenta.tipo = 1;
        MensajeCuenta.buffer = buffer;

        if(msgsnd(id_cola_cuentas, &MensajeCuenta, sizeof(bufferEstructurado), 0) == -1){
            perror("Error al enviar el mensaje a la cola de mensajes");
        }
        else{
            EscribirLog("Error al enviar el mensaje a la cola de mensajes.");
        }
    }
    else{
        EscribirLog("El buffer ya contiene las 10 cuentas.");
    }
}

void SacarCuentaBuffer() {

    id_cola_cuentas = msgget(CLAVE_COLA_CUENTAS, 0666);
    if (id_cola_cuentas == -1) {
        perror("Error al conectar a la cola de mensajes existente");
        EscribirLog("Error al conectar a la cola de mensajes");
        exit(EXIT_FAILURE);
    }
    EscribirLog("Cola de mensajes conectada.");

    struct {
        long tipo;
        bufferEstructurado buffer;
    } MensajeCuenta;

    while(msgrcv(id_cola_cuentas, &MensajeCuenta, sizeof(bufferEstructurado), 0, IPC_NOWAIT)!=-1){
        for(int i=0; i<MensajeCuenta.buffer.num_cuentas; i++){
            Cuenta cuenta = MensajeCuenta.buffer.operacion[i];
    
            FILE *archivo = fopen(configuracion.archivo_cuentas, "r");
            if(archivo == NULL){
                perror("Error a la hora de abrir el archivo");
                EscribirLog("Error a la hora de abrir el archivo de cuentas.");
                return;
            }
            sem_wait(semaforo_cuentas);

            FILE *archivo_temporal = fopen("archivo_temporal.txt", "w");
            if(archivo_temporal == NULL){
                perror("Error a la hora de abrir el archivo");
                EscribirLog("Error a la hora de abrir el archivo de cuentas temporal");
                return;
            }
            sem_wait(semaforo_memoria_compartida);
            char linea[530];
            while(fgets(linea, sizeof(linea), archivo)){
                Cuenta cuenta_archivo;
                if(sscanf(linea, "%d,%49[^,],%f,%d,%ld\n", &cuenta_archivo.numero_cuenta, cuenta_archivo.titular, &cuenta_archivo.saldo, &cuenta_archivo.num_transacciones, &cuenta_archivo.ultimoAcceso)){
                    if(cuenta_archivo.numero_cuenta == cuenta.numero_cuenta){
                        cuenta_archivo.saldo = cuenta.saldo;
                        cuenta_archivo.num_transacciones = cuenta.num_transacciones;
                        cuenta_archivo.ultimoAcceso = cuenta.ultimoAcceso;
                    }
                    fprintf(archivo_temporal, "%d,%s,%f,%d,%ld\n", cuenta_archivo.numero_cuenta, cuenta_archivo.titular, cuenta_archivo.saldo, cuenta_archivo.num_transacciones, cuenta_archivo.ultimoAcceso);
                }
            }
            fclose(archivo);
            fclose(archivo_temporal);
            sem_post(semaforo_memoria_compartida);
            sem_post(semaforo_cuentas);
            remove(configuracion.archivo_cuentas);
            rename("archivo_temporal.txt", configuracion.archivo_cuentas);
        }
        
    }
}  

void CrearMemoriaCompartida() {
    FILE *archivo;
    int i = 0;

    // Generar la clave para la memoria compartida
    key_t clave_memoria = ftok("comun.h", 'M');
    if (clave_memoria == -1) {
        perror("Error al generar la clave de memoria compartida con ftok");
        EscribirLog("Error al generar clave de memoria compartida con ftok");
        exit(EXIT_FAILURE);
    }

    // Crear segmento de memoria compartida
    shm_id = shmget(clave_memoria, sizeof(TablaCuentas), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Error al crear la memoria compartida");
        EscribirLog("Error al crear la memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Asociar la memoria compartida al proceso
    tabla = (TablaCuentas *)shmat(shm_id, NULL, 0);
    if (tabla == (void *)-1) {
        perror("Error al asociar la memoria compartida al proceso");
        EscribirLog("Error al asociar la memoria compartida al proceso");
        exit(EXIT_FAILURE);
    }

    // Inicializar la tabla de cuentas en memoria compartida
    memset(tabla, 0, sizeof(TablaCuentas));

    // Abrir el archivo cuentas.dat para cargar los datos
    archivo = fopen(configuracion.archivo_cuentas, "r");
    if (archivo == NULL) {
        perror("Error al abrir el archivo de cuentas");
        EscribirLog("Error al abrir el archivo de cuentas");
        exit(EXIT_FAILURE);
    }

    // Leer los datos del archivo y cargarlos en la tabla de memoria compartida
    while (i < CUENTAS_TOTALES && fscanf(archivo, "%d,%49[^,],%f,%d,%ld",
                                         &tabla->cuentas[i].numero_cuenta,  // Número de cuenta
                                         tabla->cuentas[i].titular,         // Titular de la cuenta
                                         &tabla->cuentas[i].saldo,          // Saldo de la cuenta
                                         &tabla->cuentas[i].num_transacciones, // Número de transacciones
                                         &tabla->cuentas[i].ultimoAcceso    // Último acceso (timestamp)
                                         ) == 5) { // Comprobar que se leyeron correctamente los 5 campos
        i++;
    }

    // Actualizar el número de cuentas cargadas
    tabla->num_cuentas = i;

    // Cerrar el archivo
    fclose(archivo);

    EscribirLog("Memoria compartida creada e inicializada correctamente con los datos de cuentas.dat");
}

void LiberarMemoriaCompartida(){
    //shmdt(tabla); // Separamos la región del espacio de direccionamiento del proceso.
    shmctl(shm_id, IPC_RMID, 0); // Eliminamos la región de memoria compartida.
    EscribirLog("Memoria compartida liberada");
    return;
}

void ConectarMemoriaCompartida() {
    // Generar la clave para la memoria compartida
    key_t clave_memoria = ftok("comun.h", 'M');
    if (clave_memoria == -1) {
        perror("Error al generar la clave de memoria compartida con ftok");
        EscribirLog("Error al generar clave de memoria compartida con ftok");
        exit(EXIT_FAILURE);
    }

    // Conectar al segmento de memoria compartida existente
    shm_id = shmget(clave_memoria, sizeof(TablaCuentas), 0666);
    if (shm_id == -1) {
        perror("Error al conectar con la memoria compartida");
        EscribirLog("Error al conectar con la memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Asociar la memoria compartida al proceso
    tabla = (TablaCuentas *)shmat(shm_id, NULL, 0);
    if (tabla == (void *)-1) {
        perror("Error al asociar la memoria compartida al proceso");
        EscribirLog("Error al asociar la memoria compartida al proceso");
        exit(EXIT_FAILURE);
    }

    EscribirLog("Memoria compartida conectada correctamente");
    return;
}

void DesconectarMC() {
    shmdt(tabla); // Solo desconecta
    return;
}




