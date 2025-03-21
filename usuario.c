#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

void *realizar_deposito(void *arg)
{
    return NULL;
}

void *realizar_retiro(void *arg)
{
    return NULL;
}
void *realizar_transferencia(void *arg)
{
    return NULL;
}

void *consultar_saldo(void *arg)
{
    return NULL;
}

int main(int argc, char *argv[])
{
    char *numero_cuenta = argv[1];
    char *titular = argv[2];
    int espera;

    printf("Número de cuenta: %s\n", numero_cuenta);
    printf("Titular: %s\n", titular);
    scanf("%d", &espera);
    pthread_t hilo[3];
    
    int opcion;
    while (1)
    {
        printf("1. Depósito\n2. Retiro\n3. Transferencia\n4. Consultar saldo\n5. Salir\n");
        printf("Opción: ");
        scanf("%d", &opcion);
        switch (opcion)
        {
        case 1:
            if ((pthread_create(&hilo[0], NULL, realizar_deposito, NULL)) != 0)
            {
                perror("Error a la hora de crear el hilo.");
                exit(EXIT_FAILURE);
            }
            else
                pthread_join(hilo[0], NULL);
            break;
        case 2:
            if ((pthread_create(&hilo[1], NULL, realizar_retiro, NULL)) != 0)
            {
                perror("Error a la hora de crear el hilo.");
                exit(EXIT_FAILURE);
            }
            else
                pthread_join(hilo[1], NULL);
            break;
        case 3:
            if ((pthread_create(&hilo[2], NULL, realizar_transferencia, NULL)) != 0)
            {
                perror("Error a la hora de crear el hilo.");
                exit(EXIT_FAILURE);
            }
            else
                pthread_join(hilo[2], NULL);
            break;
        case 4:
            if ((pthread_create(&hilo[3], NULL, consultar_saldo, NULL)) != 0)
            {
                perror("Error a la hora de crear el hilo.");
                exit(EXIT_FAILURE);
            }
            else
                pthread_join(hilo[3], NULL);
            break;
        case 5:
            exit(0);
        }
    }
}