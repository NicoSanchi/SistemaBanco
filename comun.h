#ifndef COMUN_H
#define COMUN_H

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
} Config;

typedef struct Cuenta
{
    int numero_cuenta;
    char titular[50];
    float saldo;
    int num_transacciones;
} Cuenta;

Config leer_configuracion(const char *ruta);

void EscribirLog(const char *mensaje);

#endif