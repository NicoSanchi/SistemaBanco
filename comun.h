#ifndef COMUN_H
#define COMUN_H

#include <semaphore.h>

typedef struct Config
{
    int limite_retiro;
    int limite_transferencia;
    int umbral_retiros;
    int umbral_transferencias;
    int num_hilos;
    char archivo_cuentas[50];
    char archivo_log[50];
    char archivo_transacciones[50];
    char archivo_alertas[50];
} Config;

typedef struct Cuenta
{
    int numero_cuenta;
    char titular[50];
    float saldo;
    int num_transacciones;
} Cuenta;

// Declaraciones de variables globales (definidas en comun.c)
extern Config configuracion;  
extern sem_t *semaforo_cuentas;
extern sem_t *semaforo_log;
extern sem_t *semaforo_transacciones;
extern sem_t *semaforo_alertas;

// Declaraciones de funciones
void inicializar_configuracion(); 
void EscribirLog(const char *mensaje);
void inicializar_semaforos();
void destruir_semaforos();
void conectar_semaforos();

#endif