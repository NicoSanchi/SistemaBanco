#ifndef COMUN_H
#define COMUN_H

#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <errno.h>

// Definición de la clave para la cola de mensajes 
#define CLAVE_COLA_MENSAJES 1234
#define CLAVE_BUFFER 5678
#define TIPO_ALERTA 1

#define CUENTAS_TOTALES 100

// Estructura del mensaje de alerta
typedef struct {
    long tipo;          // Tipo de mensaje (debe ser long)
    char texto[1024];   // Contenido de la alerta
} MensajeAlerta;


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
    time_t ultimoAcceso;
} Cuenta;

typedef struct TablaCuentas
{
    Cuenta cuentas[CUENTAS_TOTALES];
    int num_cuentas;
} TablaCuentas;

typedef struct {
    Cuenta cuentas[10];
    bool acceso;
    int NumeroCuentas;
}Buffer;

// Declaraciones de variables globales (definidas en comun.c)
extern Config configuracion; 
extern TablaCuentas *tabla; 
extern sem_t *semaforo_cuentas;
extern sem_t *semaforo_log;
extern sem_t *semaforo_transacciones;
extern sem_t *semaforo_alertas;
extern sem_t *semaforo_memoria_compartida;
extern int id_cola;
extern int shm_id; 

// Declaraciones de funciones
void inicializar_configuracion(); 
void EscribirLog(const char *mensaje);
void inicializar_semaforos();
void destruir_semaforos();
void conectar_semaforos();
void CrearColaMensajes();
void ConectarColaMensajes();
void DestruirColaMensajes();
void CrearMemoriaCompartida();
void LiberarMemoriaCompartida();
void MeterCuentaBuffer(struct Cuenta cuenta);
void SacarCuentaBuffer();
#endif