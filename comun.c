#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "comun.h"

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
            sscanf(linea, "LIMITE_RETIRO=%d",
                   &config.limite_retiro);
        else if (strstr(linea, "LIMITE_TRANSFERENCIA"))
            sscanf(linea,
                   "LIMITE_TRANSFERENCIA=%d", &config.limite_transferencia);
        else if (strstr(linea, "UMBRAL_RETIROS"))
            sscanf(linea, "UMBRAL_RETIROS=%d",
                   &config.umbral_retiros);
        else if (strstr(linea, "UMBRAL_TRANSFERENCIAS"))
            sscanf(linea,
                   "UMBRAL_TRANSFERENCIAS=%d", &config.umbral_transferencias);
        else if (strstr(linea, "NUM_HILOS"))
            sscanf(linea, "NUM_HILOS=%d",
                   &config.num_hilos);
        else if (strstr(linea, "ARCHIVO_CUENTAS"))
            sscanf(linea, "ARCHIVO_CUENTAS=%s",
                   config.archivo_cuentas);
        else if (strstr(linea, "ARCHIVO_LOG"))
            sscanf(linea, "ARCHIVO_LOG=%s",
                   config.archivo_log);
    }
    fclose(archivo);
    return config;
}

void EscribirLog(const char *mensaje) {
    FILE* fichero;
    fichero = fopen("registro.log", "a");

    if (fichero == NULL)
        return;

    time_t tiempo;
    struct tm *tm_info;
    char hora[26];

    time(&tiempo);
    tm_info = localtime(&tiempo);
    strftime(hora, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(fichero, "[%s] %s\n", hora, mensaje);

    fclose(fichero);
    
}
