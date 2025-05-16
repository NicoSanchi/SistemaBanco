#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
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
    ConectarMemoriaCompartida();
    ConectarColaMensajes();

    pthread_t usuario;
    pid_t pid_banco = atoi(argv[3]);
    pthread_create(&usuario, NULL, vigilar_banco, &pid_banco);

    int numeroCuenta = atoi(argv[1]); // Guardamos el numero de cuenta como entero para las operaciones
    int opcion = 0;

    for(int i=0; i<tabla->num_cuentas; i++){
        printf("%d,%s,%.2f,%d,%ld\n",
            tabla->cuentas[i].numero_cuenta,
            tabla->cuentas[i].titular,
            tabla->cuentas[i].saldo,
            tabla->cuentas[i].num_transacciones,
            tabla->cuentas[i].ultimoAcceso);
    }

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

        while (getchar() != '\n')
            ; // Limpiar buffer

        switch (opcion)
        {
        case 1:
            if (pthread_create(&hilo[0], NULL, realizar_deposito, &numeroCuenta) != 0)
            {
                perror("Error creando hilo de depósito.");
                EscribirLog("Fallo al crear hilo de depósito");
            }
            else
                pthread_join(hilo[0], NULL);
            break;
        case 2:
            if (pthread_create(&hilo[1], NULL, realizar_retiro, &numeroCuenta) != 0)
            {
                perror("Error creando hilo de retiro.");
                EscribirLog("Fallo al crear hilo de retiro");
            }
            else
                pthread_join(hilo[1], NULL);
            break;
        case 3:
            if (pthread_create(&hilo[2], NULL, realizar_transferencia, &numeroCuenta) != 0)
            {
                perror("Error creando hilo de transferencia.");
                EscribirLog("Fallo al crear hilo de transferencia");
            }
            else
                pthread_join(hilo[2], NULL);
            break;
        case 4:
            if (pthread_create(&hilo[3], NULL, consultar_saldo, &numeroCuenta) != 0)
            {
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
            DesconectarMC();
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

void ManejarSalida(int senial)
{ // Notifica que la sesion de usuario termino porque pulso Ctrl C
    int i;
    int cuenta;
    int transaccion;
    char comando[100];

    sem_getvalue(semaforo_cuentas, &cuenta);
    while (cuenta < 1)
    {
        sem_post(semaforo_cuentas);
        sem_getvalue(semaforo_cuentas, &cuenta);
    }
    sem_getvalue(semaforo_transacciones, &transaccion);
    while (transaccion < 1)
    {
        sem_post(semaforo_transacciones);
        sem_getvalue(semaforo_transacciones, &transaccion);
    }

    printf("\n👋 Saliendo del programa...\n");
    sleep(1);
    EscribirLog("El usuario cerró la sesión con Ctrl + C");
    DesconectarMC();
    snprintf(comando, sizeof(comando), "kill -9 %d", getpid());
    system(comando);
    exit(EXIT_SUCCESS);
}

// COMENTAR
void *vigilar_banco(void *arg)
{
    int i;
    pid_t banco = *(pid_t *)arg;
    char comando[100];
    while (1)
    {
        if (kill(banco, 0) == -1)
        {
            // for(i=0; i<=3; i++){
            //   pthread_join(hilo[i], NULL);
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
    int numero_cuenta = *(int *)arg; // Obtener el número de la cuenta desde el argumento
    int cantidad;
    float nuevoSaldo;
    bool exito = false;

    //sem_wait(semaforo_cuentas);
    sem_wait(semaforo_memoria_compartida);
    // Solicitar la cantidad a ingresar.
    printf("\n💵 Introduzca la cantidad a ingresar: ");
    scanf("%d", &cantidad);
    while (getchar() != '\n');

    // Buscamos en la tabla
    for (int i = 0; i < tabla->num_cuentas; i++)
    {
        if (tabla->cuentas[i].numero_cuenta == numero_cuenta)
        {
            // Actualizar el saldo y transacciones
            tabla->cuentas[i].saldo += cantidad;
            tabla->cuentas[i].num_transacciones += 1;
            nuevoSaldo = tabla->cuentas[i].saldo;
            exito = true;
            break;
        }
    }

    // Mostrar el resultado
    if (exito) {
        printf("\n✅ Ingreso realizado con éxito. Nuevo saldo: %.2f €\n", nuevoSaldo);
        EscribirLog("El usuario ha realizado un ingreso exitosamente");
        RegistrarTransacciones(numero_cuenta, 0, cantidad, "DEPÓSITO");
    }
    else {
        EscribirLog("Erro al realizar el deposito");
    }
    
    printf("\nPresione una tecla para continuar...");
    getchar();
    system("clear");
    sem_post(semaforo_memoria_compartida);
    //sem_post(semaforo_cuentas);

    return (NULL);
}

void *realizar_retiro(void *arg)
{
    int numero_cuenta = *(int *)arg; // Recuperar el número de cuenta desde el argumento
    int cantidad;
    bool exito = false;
    float nuevoSaldo = 0;

    //sem_wait(semaforo_cuentas);
    sem_wait(semaforo_memoria_compartida);
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
        sem_post(semaforo_cuentas);
        system("clear");
        return NULL;
    }

    // Buscamos en la tabla
    for (int i = 0; i < tabla->num_cuentas; i++)
    {
        if (tabla->cuentas[i].numero_cuenta == numero_cuenta)
        {
            // Verificar si hay fondos suficientes
            if (tabla->cuentas[i].saldo >= cantidad)
            {
                // Actualizar el saldo y transacciones
                tabla->cuentas[i].saldo -= cantidad;
                tabla->cuentas[i].num_transacciones += 1;
                nuevoSaldo = tabla->cuentas[i].saldo;
                exito = true;
                EscribirLog("El usuario ha realizado un retiro exitosamente");
            }
            else
            {
                printf("\n❌ Error: Fondos insuficientes. Saldo actual: %.2f €\n", tabla->cuentas[i].saldo);
                EscribirLog("Fondos insuficientes al intentar retiro");
            }
            break;
        }
    }

    // Mostrar el resultado
    if (exito)
    {
        sleep(1);
        printf("\n✅ Retiro realizado con éxito. Nuevo saldo: %.2f €\n", nuevoSaldo);
        RegistrarTransacciones(numero_cuenta, 0, cantidad, "RETIRO");
    }

    printf("\nPresione una tecla para continuar...");
    getchar();

    system("clear");
    //sem_post(semaforo_cuentas);
    sem_post(semaforo_memoria_compartida);

    return (NULL);
}

void *realizar_transferencia(void *arg)
{
    int numero_cuenta_origen = *(int *)arg;
    int cantidad;
    int numero_cuenta_destino;
    bool error = false;
    bool cuenta_destino_en_archivo = false;

    // Solicitar datos de la transferencia
    printf("\n🔀 Introduzca el número de cuenta de destino: ");
    scanf("%d", &numero_cuenta_destino);
    while (getchar() != '\n');

    printf("💵 Introduzca la cantidad a transferir: ");
    scanf("%d", &cantidad);
    while (getchar() != '\n');

    sem_wait(semaforo_memoria_compartida);

    Cuenta *cuenta_origen = NULL;
    Cuenta *cuenta_destino = NULL;

    // Buscar cuentas en memoria compartida
    for (int i = 0; i < tabla->num_cuentas; i++) {
        if (tabla->cuentas[i].numero_cuenta == numero_cuenta_origen) {
            cuenta_origen = &tabla->cuentas[i];
        }
        if (tabla->cuentas[i].numero_cuenta == numero_cuenta_destino) {
            cuenta_destino = &tabla->cuentas[i];
        }
    }

    // Verificar existencia de cuenta origen
    if (!cuenta_origen) {
        printf("\n❌ No se encontró la cuenta de origen %d\n", numero_cuenta_origen);
        EscribirLog("Cuenta de origen no encontrada.");
        error = true;
    }

    // Si cuenta destino no está en memoria, intentar en archivo
    if (!cuenta_destino && !error) {
        FILE *fichero = fopen(configuracion.archivo_cuentas, "r+");
        if (!fichero) {
            perror("Error al abrir el archivo de cuentas.");
            EscribirLog("Error al abrir el archivo de cuentas.");
            error = true;
        } else {
            EscribirLog("Archivo de cuentas abierto correctamente.");
            char linea[256];
            long posicion;
            int num_cuenta_archivo;
            char titular_archivo[50];
            float saldo_archivo;
            int num_transacciones_archivo;
            time_t ultimoAcceso_archivo;

            while (fgets(linea, sizeof(linea), fichero)) {
                posicion = ftell(fichero) - strlen(linea);
                char *token = strtok(linea, ",");

                if (token) {
                    num_cuenta_archivo = atoi(token);
                    if (numero_cuenta_destino == num_cuenta_archivo) {
                        token = strtok(NULL, ",");
                        strncpy(titular_archivo, token, sizeof(titular_archivo) - 1);
                        titular_archivo[sizeof(titular_archivo) - 1] = '\0';

                        token = strtok(NULL, ",");
                        saldo_archivo = atof(token) + cantidad;

                        token = strtok(NULL, ",");
                        num_transacciones_archivo = atoi(token);

                        token = strtok(NULL, ",");
                        ultimoAcceso_archivo = atol(token);

                        fseek(fichero, posicion, SEEK_SET);
                        fprintf(fichero, "%d,%s,%.2f,%d,%ld\n",
                                num_cuenta_archivo,
                                titular_archivo,
                                saldo_archivo,
                                num_transacciones_archivo,
                                ultimoAcceso_archivo);

                        EscribirLog("Cuenta destino encontrada y actualizada en archivo.");
                        cuenta_destino_en_archivo = true;
                        break;
                    }
                }
            }

            fclose(fichero);

            if (!cuenta_destino_en_archivo) {
                printf("\n❌ No se encontró la cuenta de destino %d\n", numero_cuenta_destino);
                EscribirLog("Cuenta de destino no encontrada en memoria ni archivo.");
                error = true;
            }
        }
    }

    // Verificar fondos suficientes (si cuenta origen es válida)
    if (!error && cuenta_origen->saldo < cantidad) {
        printf("\n❌ Fondos insuficientes. Saldo actual: %.2f €\n", cuenta_origen->saldo);
        EscribirLog("Fondos insuficientes para transferencia.");
        error = true;
    }

    // Si hubo error, salir
    if (error) {
        sem_post(semaforo_memoria_compartida);
        printf("\nPresione una tecla para continuar...");
        getchar();
        system("clear");
        return NULL;
    }

    // Realizar transferencia
    cuenta_origen->saldo -= cantidad;
    cuenta_origen->num_transacciones += 1;
    MeterCuentaBuffer(cuenta_origen);

    if (cuenta_destino) {
        cuenta_destino->saldo += cantidad;
        cuenta_destino->num_transacciones += 1;
        MeterCuentaBuffer(cuenta_destino);
        //sem_wait(semaforo_buffer);
        //buffer.operacion[buffer.fin] = *cuenta_destino;
        //buffer.fin = (buffer.fin +1) % 10;
        //sem_post(semaforo_buffer);
    }

    printf("\n✅ Transferencia exitosa. Nuevo saldo: %.2f €\n", cuenta_origen->saldo);
    EscribirLog("Transferencia realizada con éxito.");

    // Registrar transacción
    RegistrarTransacciones(numero_cuenta_origen, numero_cuenta_destino, cantidad, "TRANSFERENCIA");

    printf("\nPresione una tecla para continuar...");
    getchar();
    system("clear");

    sem_post(semaforo_memoria_compartida);
    return NULL;
}


void *consultar_saldo(void *arg)
{
    int numero_cuenta = *(int *)arg; // Recuperar el número de cuenta desde el argumento

    printf("\n📊 Consultando saldo...\n");
    sleep(1);

    //sem_wait(semaforo_cuentas); 
    sem_wait(semaforo_memoria_compartida); 

    bool encontrado = false;

    for (int i = 0; i < tabla->num_cuentas; i++) {
        if (tabla->cuentas[i].numero_cuenta == numero_cuenta) {
            printf("\n📋 Número de cuenta: %d\n", tabla->cuentas[i].numero_cuenta);
            printf("👤 Titular: %s\n", tabla->cuentas[i].titular);
            printf("💰 Saldo actual: %.2f €\n", tabla->cuentas[i].saldo);
            EscribirLog("El usuario ha consultado el saldo exitosamente");
            encontrado = true;
            break;
        }
    }

    if (!encontrado) {
        printf("\n Cuenta no encontrada.\n");
        EscribirLog("Consulta fallida: número de cuenta no encontrado");
    }

    //sem_post(semaforo_cuentas); // Liberar semáforo
    sem_post(semaforo_memoria_compartida);

    printf("\nPresione una tecla para continuar...");
    getchar();
    system("clear");

    return NULL;

}

void RegistrarTransacciones(int cuentaOrigen, int cuentaDestino, int cantidad, const char *tipo_operacion)
{
    sem_wait(semaforo_transacciones);

    // Construir la ruta del archivo transacciones.log del usuario
    char rutaArchivo[256];
    snprintf(rutaArchivo, sizeof(rutaArchivo), "./transacciones/%d/transacciones.log", cuentaOrigen);

    FILE *ficheroTransacciones = fopen(rutaArchivo, "a");
    if (ficheroTransacciones == NULL)
    {
        perror("Error al abrir el archivo de transacciones del usuario");
        EscribirLog("Fallo al abrir el archivo de transacciones del usuario");
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
    EscribirLog("Transacción registrada en el archivo transacciones.log del usuario");
}