#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include "comun.h"

pthread_t hilo[4];

// Declaraciones de funciones
void *vigilar_banco(void *arg);
void *realizar_deposito(void *arg);
void *realizar_retiro(void *arg);
void *realizar_transferencia(void *arg);
void *consultar_saldo(void *arg);
void RegistrarTransacciones(int cuentaOrigen, int cuentaDestino, int cantidad, const char *tipo_operacion);
void ManejarSalida(int senial);


int main(int argc, char *argv[])
{
    signal(SIGINT, ManejarSalida); // Si el programa captura que el usuario ha pulsado Ctrl C para terminar, notifica por pantalla
    
    inicializar_configuracion();
    conectar_semaforos();

    pthread_t usuario;
    pid_t pid_banco = atoi(argv[3]);
    pthread_create(&usuario, NULL, vigilar_banco, &pid_banco);

    int numeroCuenta = atoi(argv[1]); // Guardamos el numero de cuenta como entero para las operaciones
    int opcion = 0;

    while (1)
    {
        printf("\n🏦--------¡BIENVENIDO %s!--------🏦\n", argv[2]);
        printf("    ┌──────────────────────────────┐\n");
        printf("    │         MENÚ PRINCIPAL       │\n");
        printf("    ├──────────────────────────────┤\n");
        printf("    │ 1.  💸 Depósito              │\n");
        printf("    │ 2.  💵 Retiro                │\n");
        printf("    │ 3.  💰 Transferencia         │\n");
        printf("    │ 4.  💼 Consultar saldo       │\n");
        printf("    │ 5.  👋 Salir                 │\n");
        printf("    └──────────────────────────────┘\n");

        printf("\nOpción: ");
        scanf("%d", &opcion);

        while (getchar() != '\n'); // Limpiar buffer 

        switch (opcion)
        {
        case 1:
            if (pthread_create(&hilo[0], NULL, realizar_deposito, &numeroCuenta) != 0) {
                perror("Error creando hilo de depósito.");
                EscribirLog("Fallo al crear hilo de depósito");
            }
            else
                pthread_join(hilo[0], NULL);
            break;
        case 2:
            if (pthread_create(&hilo[1], NULL, realizar_retiro, &numeroCuenta) != 0) {
                perror("Error creando hilo de retiro.");
                EscribirLog("Fallo al crear hilo de retiro");
            }
            else
                pthread_join(hilo[1], NULL);
            break;
        case 3:
            if (pthread_create(&hilo[2], NULL, realizar_transferencia, &numeroCuenta) != 0) {
                perror("Error creando hilo de transferencia.");
                EscribirLog("Fallo al crear hilo de transferencia");
            }
            else
                pthread_join(hilo[2], NULL);
            break;
        case 4:
            if (pthread_create(&hilo[3], NULL, consultar_saldo, &numeroCuenta) != 0) {
                perror("Error creando hilo de consulta.");
                EscribirLog("Fallo al crear hilo de consulta");
            }
            else
                pthread_join(hilo[3], NULL);
            break;
        case 5:
            printf("\n👋 Saliendo...\n");
            sleep(1);
            EscribirLog("El usuario ha salido del menú de usuario");
            exit(0);
        default:
            printf("\n❌ La opción seleccionada no es válida.\n");
            printf("\nPresione una tecla para continuar...");
            getchar();
            system("clear");
            break;
        }
    }

    return (0);
}

void ManejarSalida(int senial) { // Notifica que la sesion de usuario termino porque pulso Ctrl C
    int i;
    int cuenta;
    int transaccion;
    char comando[100];

    sem_getvalue(semaforo_cuentas, &cuenta);
    while(cuenta<1) {
        sem_post(semaforo_cuentas);
        sem_getvalue(semaforo_cuentas, &cuenta);
    }
    sem_getvalue(semaforo_transacciones, &transaccion);
    while(transaccion<1) {
        sem_post(semaforo_transacciones);
        sem_getvalue(semaforo_transacciones, &transaccion);
    }

    printf("\n👋 Saliendo del programa...\n");
    sleep(1);
    EscribirLog("El usuario cerró la sesión con Ctrl + C");
    snprintf(comando, sizeof(comando), "kill -9 %d", getpid());
    system(comando);
    exit(EXIT_SUCCESS);
}

//COMENTAR
void *vigilar_banco(void *arg)
{
    int i;
    pid_t banco = *(pid_t *)arg;
    char comando[100];
    while (1)
    {
        if (kill(banco, 0) == -1)
        {
            //for(i=0; i<=3; i++){
              //  pthread_join(hilo[i], NULL);
            //}
            snprintf(comando, sizeof(comando), "kill -9 %d", getpid());
            system(comando);
        }
        sleep(1); // No saturar la CPU
    }
    return (NULL);
}

void *realizar_deposito(void *arg)
{
    int numero_cuenta = *(int *)arg; // Obtener el número de la cuenta desde el argumento.
    int cantidad;
    char linea[100];
    char linea_aux[100];
    FILE *fichero;

    

    // Solicitar la cantidad a ingresar.
    printf("\n💵 Introduzca la cantidad a ingresar: ");
    scanf("%d", &cantidad);
    while (getchar() != '\n');

    sem_wait(semaforo_cuentas);
    sleep(10);

    // Abrir el archivo de cuentas en modo lectura y escritura.
    fichero = fopen(configuracion.archivo_cuentas, "r+");
    if (fichero == NULL)
    {
        perror("Error al abrir el archivo de cuentas.");
        EscribirLog("Fallo al abrir el archivo de cuentas");
        sem_post(semaforo_cuentas);
        return NULL;
    }
    else
        EscribirLog("Se ha abierto el archivo de cuentas");

    // Leer línea por línea y buscar la cuenta.
    while (fgets(linea, sizeof(linea), fichero))
    {
        strcpy(linea_aux, linea);
        char *token = strtok(linea, ",");
        if (token != NULL)
        {
            int cuenta_actual = atoi(token);
            // Comparar el número de cuenta buscado
            if (numero_cuenta == cuenta_actual)
            {
                char *titular = strtok(NULL, ",");   // Obtenemos el titular.
                char *saldo_str = strtok(NULL, ","); // Obtenemos el saldo.
                int saldo = atoi(saldo_str);

                saldo += cantidad;                               // Calcular el nuevo saldo.
                char *num_transacciones_str = strtok(NULL, ","); // Obtener las transacciones.
                int num_transaciones = atoi(num_transacciones_str);

                // Actualizar el archivo
                fseek(fichero, -strlen(linea_aux), SEEK_CUR); // Retroceder al inicio de la línea
                fprintf(fichero, "%d,%s,%d,%d\n", cuenta_actual, titular, saldo, num_transaciones + 1);

                // Mostrar el resultado
                printf("\n✅ Ingreso realizado con éxito. Nuevo saldo: %d €\n", saldo);
                EscribirLog("El usuario ha realizado un ingreso exitosamente");
                RegistrarTransacciones(cuenta_actual, 0, cantidad, "DEPÓSITO");
                break;
            }
        }
    }
    fclose(fichero);
    sem_post(semaforo_cuentas);
    EscribirLog("Se ha cerrado el archivo de cuentas");
    printf("\nPresione una tecla para continuar...");
    getchar();
    system("clear");
    return (NULL);
}

void *realizar_retiro(void *arg)
{
    int numero_cuenta = *(int *)arg; // Recuperar el número de cuenta desde el argumento
    int cantidad;
    char linea[100];
    char linea_aux[100];
    FILE *fichero;

    // Solicitar la cantidad a retirar
    printf("\n💵 Introduzca la cantidad a retirar: ");
    scanf("%d", &cantidad);
    while (getchar() != '\n');

    // Verificar el límite de retiro
    if (cantidad > configuracion.limite_retiro)
    {
        printf("\n❌ Error: La cantidad excede el límite de retiro permitido (%d €)\n", configuracion.limite_retiro);
        printf("\nPresione una tecla para continuar...");
        getchar();
        system("clear");
        return NULL;
    }

    sem_wait(semaforo_cuentas);

    // Abrir el archivo de cuentas en modo lectura y escritura
    fichero = fopen(configuracion.archivo_cuentas, "r+");
    if (fichero == NULL)
    {
        perror("Error al abrir el archivo de cuentas");
        EscribirLog("Fallo al abrir el archivo de cuentas");
        sem_post(semaforo_cuentas);
        return NULL;
    }
    else
        EscribirLog("Se ha abierto el archivo de cuentas");

    // Leer línea por línea y buscar la cuenta
    while (fgets(linea, sizeof(linea), fichero))
    {
        strcpy(linea_aux, linea);
        char *token = strtok(linea, ",");
        
        if (token != NULL)
        {
            int cuenta_actual = atoi(token);

            // Comparar con el número de cuenta buscado
            if (cuenta_actual == numero_cuenta)
            {
                // Obtener el titular
                char *titular = strtok(NULL, ",");

                // Obtener el saldo actual
                char *saldo_str = strtok(NULL, ",");
                int saldo = atoi(saldo_str);

                // Obtener el número de transacciones
                char *num_transacciones_str = strtok(NULL, ",");
                int num_transacciones = atoi(num_transacciones_str);

                // Verificar si hay fondos suficientes
                if (saldo < cantidad)
                {
                    printf("\n❌ Error: Fondos insuficientes. Saldo actual: %d €\n", saldo);
                    printf("\nPresione una tecla para continuar...");
                    getchar();
                    fclose(fichero);
                    sem_post(semaforo_cuentas);
                    EscribirLog("Se ha cerrado el archivo de cuentas");
                    system("clear");
                    return NULL;
                }

                // Calcular el nuevo saldo
                int nuevo_saldo = saldo - cantidad;

                // Actualizar el saldo en archivo
                fseek(fichero, -strlen(linea_aux), SEEK_CUR); // Retroceder al inicio de la línea
                fprintf(fichero, "%d,%s,%d,%d\n", cuenta_actual, titular, nuevo_saldo, num_transacciones + 1);

                // Mostrar el resultado
                printf("\n✅ Retiro realizado con éxito. Nuevo saldo: %d €\n", nuevo_saldo);
                EscribirLog("El usuario ha realizado un retiro exitosamente");
                RegistrarTransacciones(cuenta_actual, 0, cantidad, "RETIRO");
                break;
            }
        }
    }

    fclose(fichero);
    sem_post(semaforo_cuentas);
    EscribirLog("Se ha cerrado el archivo de cuentas");
    printf("\nPresione una tecla para continuar...");
    getchar();
    system("clear");
    return NULL;
}

void *realizar_transferencia(void *arg)
{
    int numero_cuenta_origen = *(int *)arg; // Recuperar el número de cuenta de origen desde el argumento
    int cantidad;
    int numero_cuenta_destino;
    char linea[100];
    FILE *fichero;

    // Solicitar datos de la transferencia
    printf("\n🔀 Introduzca el número de cuenta de destino: ");
    scanf("%d", &numero_cuenta_destino);
    while (getchar() != '\n');

    printf("💵 Introduzca la cantidad a transferir: ");
    scanf("%d", &cantidad);
    while (getchar() != '\n');

    sem_wait(semaforo_cuentas);

    // Abrir el archivo de cuentas en modo lectura y escritura
    fichero = fopen(configuracion.archivo_cuentas, "r+");
    if (fichero == NULL)
    {
        perror("Error al abrir el archivo de cuentas");
        EscribirLog("Fallo al abrir el archivo de cuentas");
        sem_post(semaforo_cuentas);
        return (NULL);
    }
    else
        EscribirLog("Se ha abierto el archivo de cuentas");

    // Variables para almacenar los datos de las cuentas
    int cuenta_actual;
    char titular_origen[50], titular_destino[50]; 
    int saldo, num_transacciones;
    long posicion_cuenta_origen = -1, posicion_cuenta_destino = -1;
    int saldo_origen = -1, saldo_destino = -1;

    // Leer línea por línea y buscar las cuentas
    while (fgets(linea, sizeof(linea), fichero))
    {
        long posicion_actual = ftell(fichero) - strlen(linea); // Guardar la posición inicial de la línea

        char *token = strtok(linea, ",");
        if (token != NULL)
        {
            cuenta_actual = atoi(token);

            // Comparar con el número de cuenta de origen
            if (cuenta_actual == numero_cuenta_origen)
            {
                // Obtener el titular
                char *titular = strtok(NULL, ",");
                strncpy(titular_origen, titular, sizeof(titular_origen) - 1);
                titular_origen[sizeof(titular_origen) - 1] = '\0'; // Asegurar que la cadena esté terminada

                // Obtener el saldo actual
                char *saldo_str = strtok(NULL, ",");
                saldo_origen = atoi(saldo_str);

                // Obtener el número de transacciones
                char *num_transacciones_str = strtok(NULL, ",");
                num_transacciones = atoi(num_transacciones_str);

                // Guardar la posición de la cuenta de origen
                posicion_cuenta_origen = posicion_actual;
            }

            // Comparar con el número de cuenta de destino
            if (cuenta_actual == numero_cuenta_destino)
            {
                // Obtener el titular
                char *titular = strtok(NULL, ",");
                strncpy(titular_destino, titular, sizeof(titular_destino) - 1);
                titular_destino[sizeof(titular_destino) - 1] = '\0'; // Asegurar que la cadena esté terminada

                // Obtener el saldo actual
                char *saldo_str = strtok(NULL, ",");
                saldo_destino = atoi(saldo_str);

                // Obtener el número de transacciones
                char *num_transacciones_str = strtok(NULL, ",");
                num_transacciones = atoi(num_transacciones_str);

                // Guardar la posición de la cuenta de destino
                posicion_cuenta_destino = posicion_actual;
            }
        }
    }

    // Verificar si se encontraron ambas cuentas
    if (posicion_cuenta_origen == -1) {
        printf("\n❌ No se encontró la cuenta de origen %d\n", numero_cuenta_origen);
        printf("\nPresione una tecla para continuar...");
        getchar();
        fclose(fichero);
        sem_post(semaforo_cuentas);
        EscribirLog("Se ha cerrado el archivo de cuentas");
        system("clear");
        return NULL;
    }
    if (posicion_cuenta_destino == -1) {
        printf("\n❌ No se encontró la cuenta de destino %d\n", numero_cuenta_destino);
        printf("\nPresione una tecla para continuar...");
        getchar();
        fclose(fichero);
        sem_post(semaforo_cuentas);
        EscribirLog("Se ha cerrado el archivo de cuentas");
        system("clear");
        return NULL;
    }

    // Verificar si hay fondos suficientes
    if (saldo_origen < cantidad) {
        printf("\n❌ Fondos insuficientes. Saldo actual: %d €\n", saldo_origen);
        printf("\nPresione una tecla para continuar...");
        getchar();
        fclose(fichero);
        sem_post(semaforo_cuentas);
        EscribirLog("Se ha cerrado el archivo de cuentas");
        system("clear");
        return NULL;
    }

    // Calcular los nuevos saldos
    int nuevo_saldo_origen = saldo_origen - cantidad;
    int nuevo_saldo_destino = saldo_destino + cantidad;

    // Actualizar la cuenta de origen
    fseek(fichero, posicion_cuenta_origen, SEEK_SET);
    fprintf(fichero, "%d,%s,%d,%d\n", numero_cuenta_origen, titular_origen, nuevo_saldo_origen, num_transacciones + 1);

    // Actualizar la cuenta de destino
    fseek(fichero, posicion_cuenta_destino, SEEK_SET);
    fprintf(fichero, "%d,%s,%d,%d\n", numero_cuenta_destino, titular_destino, nuevo_saldo_destino, num_transacciones + 1);

    // Mostrar el resultado
    printf("\n✅ Transferencia exitosa. Nuevo saldo: %d €\n", nuevo_saldo_origen);
    EscribirLog("El usuario ha realizado una transferencia exitosa");
    RegistrarTransacciones(numero_cuenta_origen, numero_cuenta_destino, cantidad, "TRANSFERENCIA");

    fclose(fichero);
    sem_post(semaforo_cuentas);
    EscribirLog("Se ha cerrado el archivo de cuentas");
    printf("\nPresione una tecla para continuar...");
    getchar();
    system("clear");
    return (NULL);
}

void *consultar_saldo(void *arg)
{
    int numero_cuenta = *(int *)arg; // Recuperar el número de cuenta desde el argumento
    char linea[100];
    FILE *fichero;

    // Mostrar el mensaje
    printf("\n📊 Consultando saldo...\n");
    sleep(1);

    sem_wait(semaforo_cuentas);

    // Abrir el archivo de cuentas
    fichero = fopen(configuracion.archivo_cuentas, "r");
    if (fichero == NULL)
    {
        perror("Error al abrir el archivo de cuentas");
        EscribirLog("Fallo al abrir el archivo de cuentas");
        sem_post(semaforo_cuentas);
        return (NULL);
    }
    else
        EscribirLog("Se ha abierto el archivo de cuentas");

    // Leer línea por línea
    while (fgets(linea, sizeof(linea), fichero))
    {
        char *token = strtok(linea, ",");
        if (token != NULL)
        {
            int cuenta_actual = atoi(token);

            // Comparar con el número de cuenta buscado
            if (cuenta_actual == numero_cuenta)
            {
                // Obtener el titular
                char *titular = strtok(NULL, ",");

                // Obtener el saldo
                char *saldo_str = strtok(NULL, ",");
                int saldo = atoi(saldo_str);

                // Mostrar el saldo
                printf("\n📋 Número de cuenta: %d\n", numero_cuenta);
                printf("👤 Titular: %s\n", titular);
                printf("💰 Saldo actual: %d €\n", saldo);
                EscribirLog("El usuario ha consultado el saldo exitosamente");
                break;
            }
        }
    }

    fclose(fichero);
    sem_post(semaforo_cuentas);
    EscribirLog("Se ha cerrado el archivo de cuentas");
    printf("\nPresione una tecla para continuar...");
    getchar();
    system("clear");
    return (NULL);
}

void RegistrarTransacciones(int cuentaOrigen, int cuentaDestino, int cantidad, const char *tipo_operacion)
{
    sem_wait(semaforo_transacciones);

    FILE *ficheroTransacciones = fopen(configuracion.archivo_transacciones, "a");
    if (ficheroTransacciones == NULL)
    {
        perror("Error al abrir el archivo de transacciones");
        EscribirLog("Fallo al abrir el archivo de transacciones");
        sem_post(semaforo_transacciones);
        return;
    }

    time_t tiempo;
    struct tm *tm_info;
    char fecha[11]; // YYYY-MM-DD + \0

    time(&tiempo);
    tm_info = localtime(&tiempo);
    strftime(fecha, sizeof(fecha), "%Y-%m-%d", tm_info);

    if (strcmp(tipo_operacion, "TRANSFERENCIA") == 0)
    {
        // Formato para transferencias: fecha,tipo,origen,destino,cantidad
        fprintf(ficheroTransacciones, "%s,%s,%d,%d,%d\n", fecha, tipo_operacion, cuentaOrigen, cuentaDestino, cantidad);
    }
    else
    {
        // Formato para depósitos/retiros: fecha,tipo,origen,cantidad
        fprintf(ficheroTransacciones, "%s,%s,%d,%d\n", fecha, tipo_operacion, cuentaOrigen, cantidad);
    }

    fclose(ficheroTransacciones);
    sem_post(semaforo_transacciones);
    EscribirLog("Se ha cerrado el archivo de transacciones");
}