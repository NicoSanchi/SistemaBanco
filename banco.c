#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include "comun.h"

#define TAM_BUFFER 1024

// Declaraciones de funciones
void iniciar_sesion();
void RegistrarUsuario();
void iniciar_monitor();
void detener_monitor();
void verificar_alertas();
void ManejarSenial(int senial);

// Variables globales para el monitor
int pipe_monitor[2];  // pipe_monitor[0] lectura, pipe_monitor[1] escritura
pid_t pid_monitor;    // PID del proceso monitor


int main()
{
    signal(SIGINT, ManejarSenial); // Si recibe una señal de SIGINT que es de Ctrl C, libera los recursos 

    // Configuración inicial de semáforos
    inicializar_semaforos();
    conectar_semaforos();

    // Configuración inicial de monitor
    iniciar_monitor();
    system("clear");

    // Bucle principal
    int opcion;
    while (opcion != 3)
    {
        printf("┌──────────────────────────────┐\n");
        printf("│        🔐 SECURE BANK        │\n");
        printf("├──────────────────────────────┤\n");
        printf("│ 1.  👤 Iniciar sesión        │\n");
        printf("│ 2.  👥 Registrarse           │\n");
        printf("│ 3.  👋 Salir                 │\n");
        printf("└──────────────────────────────┘\n");
        printf("\nOpción: ");
        scanf("%d", &opcion);
        while (getchar() != '\n');

        // Verificar alertas de monitor en cada iteración
        verificar_alertas();

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
            printf("\n🔜¡HASTA LUEGO!🔜\n");
            EscribirLog("El usuario ha salido del sistema");
            detener_monitor();
            destruir_semaforos();
            break;

        default:
            printf("\nLa opción seleccionada no es válida.\n");
            printf("\nPresione una tecla para continuar...");
            getchar();
            system("clear");
            break;
        }
    }
    return 0;
}

void ManejarSenial(int senial) { // Funcion por si el banco se cierra con Ctrl C, que se liberen los recursos

    destruir_semaforos();
    detener_monitor();

    EscribirLog("El proceso banco se ha cerrado con Ctrl + C");

    printf("\n\n🚨 Programa terminado con Ctrl + C. Liberando recursos.\n");
    sleep(2);

    exit(EXIT_SUCCESS);
}

void iniciar_sesion()
{
    system("clear");

    inicializar_configuracion();

    bool encontrado = false;
    FILE *archivo;
    char linea[100];        // Buffer para leer cada línea
    char numero_cuenta[10]; // Se recomienda mayor tamaño para seguridad
    char titular[50];

    // Encabezado visual
    printf("┌───────────────────────────────────────┐\n");
    printf("│          INICIO DE SESIÓN             │\n");
    printf("└───────────────────────────────────────┘\n\n");

    // Leer número de cuenta
    printf("🔴 Número de cuenta: ");
    fgets(numero_cuenta, sizeof(numero_cuenta), stdin);
    numero_cuenta[strcspn(numero_cuenta, "\n")] = 0; // Eliminar salto de línea

    // Leer titular de la cuenta
    printf("👤 Titular de la cuenta: ");
    fgets(titular, sizeof(titular), stdin);
    titular[strcspn(titular, "\n")] = 0; // Eliminar salto de línea

    // Animación de carga
    printf("\n🔍 Verificando credenciales...\n\n");
    fflush(stdout);
    sleep(2);

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
        printf("✅ Autenticación exitosa\n\n");
        printf("🚀 Abriendo tu sesión bancaria...\n");
        fflush(stdout);
        sleep(2);  // Espera antes de abrir terminal
        EscribirLog("El usuario ha iniciado sesión correctamente");

        pid_t pid_banco = getpid(); // guarda PID del padre real

        __pid_t pid = fork();
        if (pid == 0)
        {
            char comando[300];
            snprintf(comando, sizeof(comando), "gcc usuario.c comun.c -o usuario && ./usuario '%s' '%s' %d", numero_cuenta, titular, pid_banco);

            // Ejecutar gnome-terminal y correr el comando           
            execlp("gnome-terminal", "gnome-terminal", "--", "bash", "-c", comando, NULL);
            exit(EXIT_FAILURE); // Si execlp falla
        }
        else if (pid > 0) // Proceso padre
        {
            // Espera breve no bloqueante para dar tiempo al hijo
            usleep(550000); // 0.5 segundos
        }
        else if (pid == -1)
        {
            perror("Ha ocurrido un fallo en el sistema");
            EscribirLog("El usuario ha intentado iniciar sesión. Fallo en el sistema");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("❌ Error: Credenciales incorrectas\n");
        EscribirLog("El usuario ha intentado iniciar sesión. Fallo al introducir las credenciales");
        printf("\nPresione una tecla para continuar...");
        getchar();
        system("clear");
    }
    
    system("clear");
}

void RegistrarUsuario()
{
    system("clear");

    inicializar_configuracion();

    // Encabezado visual
    printf("┌───────────────────────────────────────┐\n");
    printf("│        ✍️  REGISTRO DE USUARIO         │\n");
    printf("└───────────────────────────────────────┘\n\n");

    // Inicializamos las varibales
    FILE *ficheroUsers;
    char nombre[50], linea[100], esperar;
    int numeroCuentaCliente, numeroTransacciones = 0, saldo = 0;
    bool hayUsuarios = false;

    sem_wait(semaforo_cuentas);

    ficheroUsers = fopen(configuracion.archivo_cuentas, "a+"); // Abrimos el archivo en formato append
    if (ficheroUsers == NULL)
    {
        perror("Error a la hora de abrir el archivo");
        EscribirLog("Fallo al abrir el archivo de usuarios");

        sem_post(semaforo_cuentas);
        fclose(ficheroUsers);

        return;
    }
    else
        EscribirLog("Se ha abierto el archivo de cuentas");

    while (fgets(linea, sizeof(linea), ficheroUsers)) // Leemos el archivo linea por linea
    {
        char *token = strtok(linea, ","); // Tomamos el numero de cuenta de la linea

        if (token != NULL)
        { // Si el archivo no esta vacio, asignamos al numero de cuenta el numero de cuenta del ultimo cliente
            numeroCuentaCliente = atoi(token);
            hayUsuarios = true;
        }
    }

    if (hayUsuarios) // Si habia usuarios existentes, asigna el nuevo numero de cuenta siguiente
        numeroCuentaCliente++;
    else
        numeroCuentaCliente = 1000; // Si no inicializa a 1000 el numero de cuenta

    printf("👤 Introduce el nombre del titular: ");
    fgets(nombre, sizeof(nombre), stdin);
    nombre[strcspn(nombre, "\n")] = 0;

    // Mensaje de progreso añadido
    printf("\n🔄 Creando nueva cuenta...\n\n");
    fflush(stdout);  // Asegurar que se muestre inmediatamente
    sleep(2);  // Pequeña pausa para efecto visual

    // Generar saldo aleatorio (1000-10000)
    srand(time(NULL));
    saldo = rand() % (10000 - 1000 + 1) + 1000; // Generamos un numero entre 1000 y 10000 que sera su saldo

    fseek(ficheroUsers, -1, SEEK_END); // Nos movemos al final del archivo de usuarios
    char ultimoCaracter = fgetc(ficheroUsers);

    if (ultimoCaracter != '\n' && hayUsuarios) // Y comprobamos si hay usuarios y si el ultimo caracter no es un salto de linea
        fprintf(ficheroUsers, "\n");           // En el caso de que haya usuarios y el utlimo caracter no es un salto de linea, lo añadimos manualmente
    // De modo que se escriba en el archivo de usuarios linea por linea

    fprintf(ficheroUsers, "%d,%s,%d,%d", numeroCuentaCliente, nombre, saldo, numeroTransacciones); // Escribimos en el archivo de usaurio el nuevo usuario

    fclose(ficheroUsers);
    sem_post(semaforo_cuentas);
    EscribirLog("Se ha cerrado el archivo de cuentas");

    // Confirmación visual
    printf("\n┌───────────────────────────────────────┐\n");
    printf("│          ✅ CUENTA CREADA            │\n");
    printf("├───────────────────────────────────────┤\n");
    printf("│  Titular:     %-23s │\n", nombre);
    printf("│  N° Cuenta:   %-23d │\n", numeroCuentaCliente);
    printf("│  Saldo:       €%-22d │\n", saldo);
    printf("└───────────────────────────────────────┘\n");

    EscribirLog("Nuevo usuario registrado");

    printf("\nPulsa una tecla para continuar...");
    getchar();

    system("clear");

    return;
}

// Inicia el proceso monitor y configura la tubería
void iniciar_monitor() {
    // Crear pipe para comunicación
    if (pipe(pipe_monitor) == -1) {
        perror("Error al crear pipe");
        EscribirLog("Error al crear pipe entre monitor y banco");
        exit(EXIT_FAILURE);
    }
    
    // Crear proceso hijo
    pid_monitor = fork();
    if (pid_monitor == 0) { // Código del monitor
        close(pipe_monitor[0]); // Cerrar extremo de lectura
        
        // Convertir descriptor a string para argumento
        char fd_str[16];
        snprintf(fd_str, sizeof(fd_str), "%d", pipe_monitor[1]);
        
        // Ejecutar monitor
        execl("./monitor", "monitor", fd_str, NULL);
        
        // Solo llegamos aquí si hay error
        EscribirLog("Error al ejecutar monitor");
        exit(EXIT_FAILURE);
    }
    
    // Código del banco (proceso padre)
    close(pipe_monitor[1]); // Cerrar extremo de escritura no usado
    EscribirLog("Monitor iniciado correctamente");
}

// Detiene el proceso monitor y cierra la tubería
void detener_monitor() {
    //kill(pid_monitor, SIGTERM);
    close(pipe_monitor[0]);
    EscribirLog("Monitor detenido");
}

// Verifica si hay alertas del monitor y las registra
void verificar_alertas() {

    inicializar_configuracion();

    char alerta[TAM_BUFFER];
    FILE *fichero_alertas;
    
    // Lectura no bloqueante
    ssize_t bytes = read(pipe_monitor[0], alerta, sizeof(alerta)-1);
    
    if (bytes > 0) {
        alerta[bytes] = '\0';

        sem_wait(semaforo_alertas);

        fichero_alertas = fopen(configuracion.archivo_alertas, "a");
        if (fichero_alertas == NULL) {
            perror("Error a la hora de abrir el archivo");
            EscribirLog("Fallo al abrir el archivo de alertas");

            sem_post(semaforo_alertas);

            return;
        }
        else {
            time_t ahora = time(NULL);
            char timestamp[20];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&ahora));
            fprintf(fichero_alertas, "[%s] %s\n", timestamp, alerta);
            fclose(fichero_alertas);
            EscribirLog("Se ha cerrado el archivo de alertas");
        }

        sem_post(semaforo_alertas);
    }
}



