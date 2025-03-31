#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include "comun.h"

// Definiciones de los semáforos
sem_t *semaforo_cuentas = NULL;
sem_t *semaforo_log = NULL;
sem_t *semaforo_transacciones = NULL;
sem_t *semaforo_alertas = NULL;

// Definición global para configuración
Config configuracion;


void inicializar_semaforos(){
    semaforo_cuentas = sem_open("/semaforo_cuentas", O_CREAT, 0644, 1);
    semaforo_log = sem_open("/semaforo_log", O_CREAT, 0644, 1);
    semaforo_transacciones = sem_open("/semaforo_transacciones", O_CREAT, 0644, 1);
    semaforo_alertas = sem_open("/semaforo_alertas", O_CREAT, 0644, 1);

    if(semaforo_cuentas== SEM_FAILED || semaforo_log == SEM_FAILED || semaforo_transacciones == SEM_FAILED || semaforo_alertas == SEM_FAILED){
        perror("⚠Ha ocurrido un error a la hora de crear los semáforos");
        EscribirLog("Error al crear los semáforos");
        exit(EXIT_FAILURE);
    }
}

void conectar_semaforos(){
    semaforo_cuentas = sem_open("/semaforo_cuentas", 0);
    semaforo_log = sem_open("/semaforo_log", 0);
    semaforo_transacciones = sem_open("/semaforo_transacciones", 0);
    semaforo_alertas = sem_open("/semaforo_alertas", 0);

    if(semaforo_cuentas == SEM_FAILED || semaforo_log == SEM_FAILED || semaforo_transacciones == SEM_FAILED  || semaforo_alertas == SEM_FAILED){
        perror("⚠ Error al conectar con los semáforos existentes");
        EscribirLog("Error al conectar los semáforos");
        exit(EXIT_FAILURE);
    }
}

void destruir_semaforos(){
    sem_close(semaforo_cuentas);
    sem_close(semaforo_log);
    sem_close(semaforo_transacciones);
    sem_close(semaforo_alertas);
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
