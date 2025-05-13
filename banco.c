#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include "comun.h"

#define TAM_BUFFER 1024

pid_t pid_monitor = -1;

// Declaraciones de funciones
void MenuInicio();
void ManejarSenial(int senial);
void iniciar_sesion();
void RegistrarUsuario();
void IniciarMonitor();
void detener_monitor();
void LeerAlertas(int sig);
int EncontrarLRU();

int main()
{
    // Configuración inicial de semáforos
    inicializar_semaforos();
    conectar_semaforos();
    CrearColaMensajes();

    inicializar_configuracion();

    // Creamos la memoria compartida
    CrearMemoriaCompartida();

    signal(SIGUSR1, LeerAlertas);  // Manejar señal del monitor
    signal(SIGINT, ManejarSenial); // Si recibe una señal de SIGINT que es de Ctrl C, libera los recursos

    IniciarMonitor();

    MenuInicio();

    return 0;
}

void MenuInicio()
{
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
            printf("\n🔜¡HASTA LUEGO!🔜\n\n");
            EscribirLog("El usuario ha salido del sistema");
            detener_monitor();
            sem_wait(semaforo_cuentas);
            sem_post(semaforo_cuentas);
            destruir_semaforos();
            LiberarMemoriaCompartida();
            // unlink(PIPE_ALERTAS);
            break;

        default:
            printf("\nLa opción seleccionada no es válida.\n");
            printf("\nPresione una tecla para continuar...");
            getchar();
            system("clear");
            break;
        }
    }
}

void ManejarSenial(int senial)
{ // Funcion por si el banco se cierra con Ctrl C, que se liberen los recursos
    sem_wait(semaforo_cuentas);
    sem_post(semaforo_cuentas);
    destruir_semaforos();
    detener_monitor();
    LiberarMemoriaCompartida();
    // unlink(PIPE_ALERTAS);
    EscribirLog("El proceso banco se ha cerrado con Ctrl + C");
    printf("\n\n🚨 Programa terminado con Ctrl + C. Liberando recursos.\n\n");
    sleep(1);
    exit(EXIT_SUCCESS);
}

void iniciar_sesion()
{
    system("clear");

    //inicializar_configuracion();
    int i, indiceLRU;
    bool encontrado = false, usuarioEnMemoria = false;
    FILE *archivo;
    char linea[100];        // Buffer para leer cada línea
    char numero_cuenta[10]; // Se recomienda mayor tamaño para seguridad
    int numero_cuenta_en_int;
    char titular[50];

    // Encabezado visual
    printf("┌───────────────────────────────────────┐\n");
    printf("│          INICIO DE SESIÓN             │\n");
    printf("└───────────────────────────────────────┘\n\n");

    // Leer número de cuenta
    printf("🔴 Número de cuenta: ");
    fgets(numero_cuenta, sizeof(numero_cuenta), stdin);
    numero_cuenta[strcspn(numero_cuenta, "\n")] = 0; // Eliminar salto de línea
    numero_cuenta_en_int = atoi(numero_cuenta);
    // Leer titular de la cuenta
    printf("👤 Titular de la cuenta: ");
    fgets(titular, sizeof(titular), stdin);
    titular[strcspn(titular, "\n")] = 0; // Eliminar salto de línea

    // Animación de carga
    printf("\n🔍 Verificando credenciales...\n\n");
    fflush(stdout);
    sleep(2);
    //-----------------------------------------------------------------------------------------
    archivo = fopen(configuracion.archivo_cuentas, "r");
    
    if (archivo == NULL) {
        perror("Error al abrir el archivo de cuentas");
        EscribirLog("Error al abrir el archivo cuentas intentando iniciar sesión");
        exit(EXIT_FAILURE);
    }

    EscribirLog("Archivo de cuentas abierto correctamente intentando iniciar sesión");

    Cuenta cuentaEncontrada;

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
                    // Guardamos los datos de la cuenta
                    cuentaEncontrada.numero_cuenta = atoi(numero_cuenta_archivo);
                    strncpy(cuentaEncontrada.titular, titular_archivo, sizeof(cuentaEncontrada.titular) - 1);
                    cuentaEncontrada.saldo = atof(strtok(NULL, ","));
                    cuentaEncontrada.num_transacciones = atoi(strtok(NULL, ","));

                    encontrado = true;
                    break;
                }
            }
        }
    }

    fclose(archivo); // Cerrar el archivo
    EscribirLog("Se ha cerrado el archivo de cuentas correctamente intentando iniciar sesión");

    if (!encontrado) {
        printf("❌ Error: Credenciales incorrectas\n");
        EscribirLog("El usuario ha intentado iniciar sesión. Fallo al introducir las credenciales");
        printf("\nPresione una tecla para continuar...");
        getchar();
        system("clear");
        return;
    }

    // Si las credenciales son correctas buscamos en memoria si el usuario ya está conectado
    printf("✅ Autenticación exitosa\n\n");
    printf("🚀 Abriendo tu sesión bancaria...\n");

    EscribirLog("El usuario ha iniciado sesión correctamente");

    for(i=0; i<CUENTAS_TOTALES; i++){

        if(tabla->cuentas[i].numero_cuenta == numero_cuenta_en_int && strcmp(tabla->cuentas[i].titular, titular) == 0){
            usuarioEnMemoria = true;
            break;
        }

    }

    if (!usuarioEnMemoria) {

        indiceLRU = EncontrarLRU();
        tabla->cuentas[indiceLRU] = cuentaEncontrada;
        
    }

    tabla->cuentas[i].ultimoAcceso = time(NULL);

    pid_t pid_banco = getpid(); // guarda PID del padre real

     __pid_t pid = fork();
    if (pid == 0)
    {
        char comando[300];
        snprintf(comando, sizeof(comando), "gcc usuario.c comun.c -o usuario -lpthread && ./usuario '%s' '%s' %d", numero_cuenta, titular, pid_banco);

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
    fflush(stdout); // Asegurar que se muestre inmediatamente
    sleep(2);       // Pequeña pausa para efecto visual

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
    printf("│          ✅ CUENTA CREADA             │\n");
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

// Función para crear el proceso monitor
void IniciarMonitor()
{
    pid_monitor = fork();

    if (pid_monitor < 0)
    {
        perror("Error en fork");
        EscribirLog("Error en fork para crear monitor");
        exit(EXIT_FAILURE);
    }

    if (pid_monitor == 0)
    {
        // Proceso hijo
        execlp("./monitor", "monitor", NULL); // Ejecuta el proceso monitor

        // Si falla:
        perror("Error ejecutando monitor");
        EscribirLog("Error al ejecutar monitor");
        exit(EXIT_FAILURE);
    }
    else
        EscribirLog("Monitor iniciado en segundo plano");
}

void detener_monitor()
{
    if (pid_monitor > 0) {
        kill(pid_monitor, SIGTERM);  // Envía señal
        waitpid(pid_monitor, NULL, 0);  // Opcional: elimínalo si no es necesario
    }
    DestruirColaMensajes();
    EscribirLog("Monitor detenido");
}

void LeerAlertas(int sig)
{
    MensajeAlerta msg;

    sem_wait(semaforo_alertas); // Bloqueo para acceder a la cola y alertas.log
    ConectarColaMensajes();

    if (msgrcv(id_cola, &msg, sizeof(msg.texto), TIPO_ALERTA, IPC_NOWAIT) == -1)
    {
        perror("No se pudo leer alerta de la cola");
        sem_post(semaforo_alertas);  // No olvidar liberar
        return;
    }

    EscribirLog("Alerta recibida");

    FILE *fichero_alertas = fopen("alertas.log", "a");
    if (!fichero_alertas)
    {
        perror("No se pudo abrir alertas.log");
        EscribirLog("Error al abrir el archvio de alertas");
        return;
    }

    // Escribir la alerta en el archivo
    fprintf(fichero_alertas, "🚨 ALERTA DEL MONITOR 🚨\n%s\n", msg.texto);
    fclose(fichero_alertas);
    //printf("\n\n🚨 Se ha registrado una nueva alerta\n");

    sem_post(semaforo_alertas);
}

int EncontrarLRU() {

    time_t min = time(NULL); 
    int indiceLRU = -1;

    for (int i = 0; i < tabla->num_cuentas; i++) {

        if (tabla->cuentas[i].ultimoAcceso < min) {
            min = tabla->cuentas[i].ultimoAcceso;
            indiceLRU = i;
        }
    }

    return indiceLRU;
}