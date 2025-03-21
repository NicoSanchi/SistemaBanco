#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

void *realizar_deposito(void *arg) {
    printf("Realizando depósito...\n");
    return NULL;
}

void *realizar_retiro(void *arg) {
    printf("Realizando retiro...\n");
    return NULL;
}

void *realizar_transferencia(void *arg) {
    printf("Realizando transferencia...\n");
    return NULL;
}

void *consultar_saldo(void *arg) {
    printf("Consultando saldo...\n");
    return NULL;
}

int main(int argc, char* argv[]){
    pthread_t hilo[4];  // ← CORREGIDO para permitir las 4 operaciones


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
                if (pthread_create(&hilo[3], NULL, consultar_saldo, NULL) != 0)
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
