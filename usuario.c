#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "comun.h"

void *realizar_deposito(void *arg) {
    printf("Realizando depósito...\n");
    return NULL;
}

void *realizar_retiro(void *arg) {

    Config configuracion = leer_configuracion("config.txt");

    int numero_cuenta = *(int *)arg; // Recuperar el número de cuenta desde el argumento
    float cantidad; 
    char linea[100];
    FILE *fichero;

    // Solicitar la cantidad a retirar
    printf("Introduzca la cantidad a retirar: ");
    scanf("%f", &cantidad);

    // Verificar el límite de retiro
    if (cantidad > configuracion.limite_retiro) {
        printf("Error: La cantidad excede el límite de retiro permitido (%d)\n", configuracion.limite_retiro);
        return NULL;
    }

    // Abrir el archivo de cuentas en modo lectura y escritura
    fichero = fopen(configuracion.archivo_cuentas, "r+");
    if (fichero == NULL) {
        perror("Error al abrir el archivo de cuentas");
        return NULL;
    }

    // Leer línea por línea y buscar la cuenta
    while (fgets(linea, sizeof(linea), fichero)) {
        char *token = strtok(linea, ",");
        if (token != NULL) {
            int cuenta_actual = atoi(token);

            // Comparar con el número de cuenta buscado
            if (cuenta_actual == numero_cuenta) {
                // Obtener el titular
                char *titular = strtok(NULL, ",");

                // Obtener el saldo actual
                char *saldo_str = strtok(NULL, ",");
                float saldo = atof(saldo_str);

                // Verificar si hay fondos suficientes
                if (saldo < cantidad) {
                    printf("Error: Fondos insuficientes. Saldo actual: %.2f\n", saldo);
                    fclose(fichero);
                    return NULL;
                }

                // Calcular el nuevo saldo
                float nuevo_saldo = saldo - cantidad;

                // Obtener el número de transacciones
                char *num_transacciones_str = strtok(NULL, ",");
                int num_transacciones = atoi(num_transacciones_str);

                // Actualizar el archivo
                fseek(fichero, -strlen(linea), SEEK_CUR); // Retroceder al inicio de la línea
                fprintf(fichero, "%d,%s,%.2f,%d\n", cuenta_actual, titular, nuevo_saldo, num_transacciones + 1);

                // Mostrar el resultado
                printf("Retiro exitoso. Nuevo saldo: %.2f\n", nuevo_saldo);

                fclose(fichero);
                return NULL;
            }
        }
    }

    // Si no se encontró la cuenta
    printf("No se encontró la cuenta con el número %d\n", numero_cuenta);

    fclose(fichero);
    return NULL;
}

void *realizar_transferencia(void *arg) { // Tiene que recibir el numero de cuenta

    FILE* fichero;
    fichero = fopen(fichero, "a+");
    char linea[100];
    int cuentaSeleccionada = 0;

    printf("Introduzca el numero de cuenta al cual quiere realizar la transeferencia: ");

    while (fgets(linea, sizeof(linea), fichero))
    {
        char* numCuenta = strtok(linea, ",");


        if (atoi(numCuenta) != arg) { // Lista a las cuentas a las que puede realizar una transferencia, sin contar la suya propia
            char* nombre = strtok(NULL, ",");

            printf("Numero de cuenta: %d - %s", atoi(numCuenta), nombre);
        }

    }
    scanf("%d", &cuentaSeleccionada);



    printf("Realizando transferencia...\n");

    return NULL;
}

void *consultar_saldo(void *arg) {

    Config configuracion = leer_configuracion("config.txt");

    int numero_cuenta = *(int *)arg; // Recuperar el número de cuenta desde el argumento
    char linea[100];
    FILE *fichero;

    // Mostrar el mensaje 
    printf("Consultando saldo...\n");

    // Abrir el archivo de cuentas
    fichero = fopen(configuracion.archivo_cuentas, "r");
    if (fichero == NULL) {
        perror("Error al abrir el archivo de cuentas");
        return NULL;
    }

    // Leer línea por línea
    while (fgets(linea, sizeof(linea), fichero)) {
        char *token = strtok(linea, ",");
        if (token != NULL) {
            int cuenta_actual = atoi(token);

            // Comparar con el número de cuenta buscado
            if (cuenta_actual == numero_cuenta) {
                // Obtener el titular
                char *titular = strtok(NULL, ",");

                // Obtener el saldo
                char *saldo_str = strtok(NULL, ",");
                float saldo = atof(saldo_str);

                // Mostrar el saldo
                printf("Titular: %s\n", titular);
                printf("Saldo actual: %.2f\n", saldo);

                fclose(fichero);
                return NULL;
            }
        }
    }

    // Si no se encontró la cuenta
    printf("No se encontró la cuenta con el número %d\n", numero_cuenta);

    fclose(fichero);
    return NULL;
}

int main(int argc, char* argv[]) {

    pthread_t hilo[4];  // ← CORREGIDO para permitir las 4 operaciones

    int numeroCuenta = atoi(argv[1]); // Guardamos el numero de cuenta como entero para las operaciones

    int opcion;
    while (1) {
        printf("\n🏦--------¡BIENVENIDO %s--------🏦\n", argv[2]);
        printf("1. 💸Depósito\n2. 📉Retiro\n3. 💰Transferencia\n4. 💼Consultar saldo\n5. 👋Salir\n");
        printf("Opción: ");
        scanf("%d", &opcion);
        while (getchar() != '\n'); // Limpiar buffer del stdin

        switch (opcion) {
            case 1:
                if (pthread_create(&hilo[0], NULL, realizar_deposito, NULL) != 0)
                    perror("Error creando hilo de depósito.");
                else
                    pthread_join(hilo[0], NULL);
                break;
            case 2:
                if (pthread_create(&hilo[1], NULL, realizar_retiro, NULL) != 0)
                    perror("Error creando hilo de retiro.");
                else
                    pthread_join(hilo[1], NULL);
                break;
            case 3:
                if (pthread_create(&hilo[2], NULL, realizar_transferencia, NULL) != 0)
                    perror("Error creando hilo de transferencia.");
                else
                    pthread_join(hilo[2], NULL);
                break;
            case 4:
                if (pthread_create(&hilo[3], NULL, consultar_saldo, &numeroCuenta) != 0)
                    perror("Error creando hilo de consulta.");
                else
                    pthread_join(hilo[3], NULL);
                break;
            case 5:
                printf("Saliendo...\n");
                exit(0);
        }
    }
    
    return 0;
}
