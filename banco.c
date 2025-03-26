#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include "comun.h"

void iniciar_sesion()
{
    system("clear");
    Config configuracion = leer_configuracion("config.txt");
    //int fd[2];
    bool encontrado = false;
    FILE *archivo;
    char linea[100];        // Buffer para leer cada línea
    char numero_cuenta[10]; // Se recomienda mayor tamaño para seguridad
    char titular[50];

    // Leer número de cuenta
    printf("🔴 Ingrese el número de cuenta: ");
    fgets(numero_cuenta, sizeof(numero_cuenta), stdin);
    numero_cuenta[strcspn(numero_cuenta, "\n")] = 0; // Eliminar salto de línea

    // Leer titular de la cuenta
    printf("🚹 Ingrese el titular de la cuenta: ");
    fgets(titular, sizeof(titular), stdin);
    titular[strcspn(titular, "\n")] = 0; // Eliminar salto de línea

    // Abrir archivo
    archivo = fopen(configuracion.archivo_cuentas, "r");
    if (archivo == NULL)
    {
        perror("Error a la hora de abrir el archivo");
        EscribirLog("Fallo al abrir el archivo de usuarios");
        return;
    }
    else
        EscribirLog("Se ha abierto el archivo de cuentas");

    // Leer línea por línea
    while (fgets(linea, sizeof(linea), archivo))
    {
        // Obtener número de cuenta
        char *token = strtok(linea, ",");
        if (token != NULL)
        {
            char numero_cuenta_archivo[10];
            strncpy(numero_cuenta_archivo, token, sizeof(numero_cuenta_archivo) - 1);
            numero_cuenta_archivo[sizeof(numero_cuenta_archivo) - 1] = '\0';

            // Obtener titular
            token = strtok(NULL, ",");
            if (token != NULL)
            {
                char titular_archivo[50];
                strncpy(titular_archivo, token, sizeof(titular_archivo) - 1);
                titular_archivo[sizeof(titular_archivo) - 1] = '\0';

                // Comparar sin saltos de línea
                if (strcmp(numero_cuenta_archivo, numero_cuenta) == 0 &&
                    strcmp(titular_archivo, titular) == 0)
                {
                    encontrado = true;
                    break;
                }
            }
        }
    }

    fclose(archivo); // Cerrar el archivo
    EscribirLog("Se ha cerrado el archivo de cuentas");

    // Mensaje de salida
    if (encontrado)
    {
        //pipe(fd);
        __pid_t pid = fork();
        if (pid == -1)
        {
            perror("Ha ocurrido un fallo en el sistema");
            EscribirLog("El usuario ha intentado iniciar sesión. Fallo en el sistema");
            exit(EXIT_FAILURE);
        }

        if (pid == 0)
        {
            char comando[300];
            snprintf(comando, sizeof(comando), "gcc usuario.c comun.c -o usuario && ./usuario '%s' '%s'", numero_cuenta, titular);

            // Ejecutar gnome-terminal y correr el comando
            EscribirLog("El usuario ha iniciado sesión correctamente");
            execlp("gnome-terminal", "gnome-terminal", "--", "bash", "-c", comando, NULL);
        }
    }
    else
    {
        printf("Algunos de los datos son incorrectos.\n");
        EscribirLog("El usuario ha intentado iniciar sesión. Fallo al introducir las credenciales");
    }
    //char tecla;
    //printf("Presione una tecla para continuar...");
    //scanf("%c", &tecla);
    system("clear");
}

void RegistrarUsuario() {

    system("clear");

    Config configuracion = leer_configuracion("config.txt");

    // Inicializamos las varibales
	FILE *ficheroUsers;
	char nombre[50], linea[100], esperar;  
    int numeroCuentaCliente, numeroTransacciones = 0, saldo = 0;
    bool hayUsuarios = false;

    srand(time(NULL));
        
	ficheroUsers = fopen(configuracion.archivo_cuentas, "a+"); // Abrimos el archivo en formato append
    if (ficheroUsers == NULL)
    {
        perror("Error a la hora de abrir el archivo");
        EscribirLog("Fallo al abrir el archivo de usuarios");
        return;
    }
    else
        EscribirLog("Se ha abierto el archivo de cuentas");

    while (fgets(linea, sizeof(linea), ficheroUsers)) // Leemos el archivo linea por linea
    {
        char* token = strtok(linea, ","); // Tomamos el numero de cuenta de la linea
        
        if (token != NULL) { // Si el archivo no esta vacio, asignamos al numero de cuenta el numero de cuenta del ultimo cliente
            numeroCuentaCliente = atoi(token); 
            hayUsuarios = true;
        }
    }

    if (hayUsuarios) // Si habia usuarios existentes, asigna el nuevo numero de cuenta siguiente
        numeroCuentaCliente++;
    else
        numeroCuentaCliente = 1000; // Si no inicializa a 1000 el numero de cuenta
	
	printf("Introduce el nombre del titular: ");
	fgets(nombre, sizeof(nombre), stdin);
    nombre[strcspn(nombre, "\n")] = 0;

    saldo = rand() % (10000 - 1000 + 1) + 1000; // Generamos un numero entre 1000 y 10000 que sera su saldo

    fseek(ficheroUsers, -1, SEEK_END); // Nos movemos al final del archivo de usuarios
    char ultimoCaracter = fgetc(ficheroUsers);

    if (ultimoCaracter != '\n' && hayUsuarios) // Y comprobamos si hay usuarios y si el ultimo caracter no es un salto de linea
        fprintf(ficheroUsers, "\n"); // En el caso de que haya usuarios y el utlimo caracter no es un salto de linea, lo añadimos manualmente 
    // De modo que se escriba en el archivo de usuarios linea por linea

    fprintf(ficheroUsers, "%d,%s,%d,%d", numeroCuentaCliente, nombre, saldo, numeroTransacciones); // Escribimos en el archivo de usaurio el nuevo usuario

    printf("\n👋 Hola %s tu número de cuenta es %d\n", nombre, numeroCuentaCliente);
    printf("\nPulsa una tecla para continuar...");
    scanf("%c", &esperar);

	fclose(ficheroUsers);
    EscribirLog("Se ha cerrado el archivo de cuentas");

    EscribirLog("El usuario se ha registrado correctamente");

    system("clear");

    return;

}

int main()
{
    int opcion;
    while (opcion != 3)
    {
        printf("┌──────────────────────────────┐\n");
        printf("│        🔐 MENÚ LOGIN         │\n");
        printf("├──────────────────────────────┤\n");
        printf("│ 1.  👤 Iniciar sesión        │\n");
        printf("│ 2.  👥 Registrarse           │\n");
        printf("│ 3.  👋 Salir                 │\n");
        printf("└──────────────────────────────┘\n");
        printf("\nOpcion: ");
        scanf("%d", &opcion);
        while (getchar() != '\n')
            ;
        switch (opcion)
        {
        case 1:
            iniciar_sesion();
            break;
        case 2:
            system("clear");
            RegistrarUsuario();
            break;
        case 3:
            printf("🔜¡HASTA LUEGO!🔜\n");
            EscribirLog("El usuario ha salido del sistema");
        }
    }
    return 0;
}
