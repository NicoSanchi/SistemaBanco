#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>


// Estructura para almacenar la configuración
typedef struct Config {
    int limite_retiro;
    int limite_transferencia;
    int umbral_retiros;
    int umbral_transferencias;
    int num_hilos;
    char archivo_cuentas[50];
    char archivo_log[50];
} Config;

Config configuracion;

// Función para leer config.txt al inicio del programa
Config LeerConfiguracion(const char *ruta) {
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        perror("Error al abrir config.txt");
        exit(1);
    }
    Config config;
    char linea[100];
    while (fgets(linea, sizeof(linea), archivo)) {
        if (linea[0] == '#' || strlen(linea) < 3)
            continue; // Ignorar comentarios y líneas vacías
        if (strstr(linea, "LIMITE_RETIRO"))
            sscanf(linea, "LIMITE_RETIRO=%d", &config.limite_retiro);
        else if (strstr(linea, "LIMITE_TRANSFERENCIA"))
            sscanf(linea, "LIMITE_TRANSFERENCIA=%d", &config.limite_transferencia);
        else if (strstr(linea, "UMBRAL_RETIROS"))
            sscanf(linea, "UMBRAL_RETIROS=%d", &config.umbral_retiros);
        else if (strstr(linea, "UMBRAL_TRANSFERENCIAS"))
            sscanf(linea, "UMBRAL_TRANSFERENCIAS=%d", &config.umbral_transferencias);
        else if (strstr(linea, "NUM_HILOS"))
            sscanf(linea, "NUM_HILOS=%d", &config.num_hilos);
        else if (strstr(linea, "ARCHIVO_CUENTAS"))
            sscanf(linea, "ARCHIVO_CUENTAS=%s", config.archivo_cuentas);
        else if (strstr(linea, "ARCHIVO_LOG"))
            sscanf(linea, "ARCHIVO_LOG=%s", config.archivo_log);
    }
    fclose(archivo);
    return config;
}


void iniciar_sesion()
{
    system("clear");
    int fd[2];
    bool encontrado = false;
    FILE *archivo;
    char linea[100];        // Buffer para leer cada línea
    char numero_cuenta[10]; // Se recomienda mayor tamaño para seguridad
    char titular[50];

    // Leer número de cuenta
    printf("🔴 Ingrese el numero de cuenta: ");
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
        return;
    }

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

    // Mensaje de salida
    if (encontrado)
    {
        pipe(fd);
        __pid_t pid = fork();
        if (pid == -1)
        {
            perror("Ha ocurrido un fallo en el sistema");
            exit(EXIT_FAILURE);
        }

        if (pid == 0)
        {
            char comando[300];
            snprintf(comando, sizeof(comando), "gcc usuario.c -o usuario && ./usuario '%s' '%s'", numero_cuenta, titular);

            // Ejecutar gnome-terminal y correr el comando
            execlp("gnome-terminal", "gnome-terminal", "--", "bash", "-c", comando, NULL);
        }
    }
    else
    {
        printf("Algunos de los datos son incorrectos.\n");
    }
    char tecla;
    printf("Presione una tecla para continuar...");
    scanf("%c", &tecla);
    system("clear");
}

void RegistrarUsuario() {

    system("clear");

	FILE *ficheroUsers;
	char nombre[100], linea[100], esperar;
    int numeroCuentaCliente = 0, numeroTransacciones = 0, saldo = 0;
    bool hayUsuarios = false;

    srand(time(NULL));
        
	ficheroUsers = fopen(configuracion.archivo_cuentas, "a+"); // Abrimos el archivo en formato append

    while (fgets(linea, sizeof(linea), ficheroUsers))
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
	
	printf("Introduce el nombre de usuario que quieres: ");
	fgets(nombre, sizeof(nombre), stdin);
    nombre[strcspn(nombre, "\n")] = 0;

    saldo = rand() % (10000 - 1000 + 1) + 1000; // Generamos un numero entre 1000 y 10000 que sera su saldo

    fseek(ficheroUsers, -1, SEEK_END);
    char ultimoCaracter = fgetc(ficheroUsers);

    if (ultimoCaracter != '\n' && hayUsuarios)
        fprintf(ficheroUsers, "\n");

    fprintf(ficheroUsers, "%d,%s,%d,%d", numeroCuentaCliente, nombre, saldo, numeroTransacciones); // Escribimos en el archivo de usaurio el nuevo usuario

    printf("\nHola %s tu numero de cuenta es %d\n", nombre, numeroCuentaCliente);
    printf("\nPulsa una tecla para continuar...");
    scanf("%c", &esperar);

	fclose(ficheroUsers);

    system("clear");

    return;

}

int main()
{
    configuracion = LeerConfiguracion("config.txt");

    int opcion;
    while (opcion != 3)
    {
        system("clear");
        printf("1. 👤Iniciar sesión.\n");
        printf("2. 👥Registrarse.\n");
        printf("3. 👋Salir.\n");
        printf("Opción: ");
        scanf("%d", &opcion);
        while (getchar() != '\n');
        switch (opcion)
        {
        case 1:
            iniciar_sesion();
            break;
        case 2:
            RegistrarUsuario();
            break;
        case 3:
            printf("🔜¡HASTA LUEGO!🔜\n");
        }
    }

    return (0);
}
