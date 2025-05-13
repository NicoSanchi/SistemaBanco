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
void GuardarEnArchivo();

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
            GuardarEnArchivo();
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
    GuardarEnArchivo();
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
    int i;
    bool encontrado = false;
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

    for(i=0; i<CUENTAS_TOTALES; i++){
        if(tabla->cuentas[i].numero_cuenta == numero_cuenta_en_int && strcmp(tabla->cuentas[i].titular, titular) == 0){
            encontrado = true;
            break;
        }
    }
    if (encontrado)
    {
        printf("✅ Autenticación exitosa\n\n");
        printf("🚀 Abriendo tu sesión bancaria...\n");
        fflush(stdout);
        sleep(2); // Espera antes de abrir terminal
        EscribirLog("El usuario ha iniciado sesión correctamente");

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

    //inicializar_configuracion();

    // Encabezado visual
    printf("┌───────────────────────────────────────┐\n");
    printf("│        ✍️  REGISTRO DE USUARIO         │\n");
    printf("└───────────────────────────────────────┘\n\n");

    // Inicializamos las varibales
    char nombre[50];
    int numeroCuentaCliente, numeroTransacciones = 0, saldo = 0;

    sem_wait(semaforo_cuentas);

    if (tabla->num_cuentas >= CUENTAS_TOTALES) { // aqui hay que implementar que desaloje a usuarios
        printf("❌ Límite de cuentas alcanzado. No se pueden registrar más usuarios.\n");
        sem_post(semaforo_cuentas);
        EscribirLog("Intento de registrar usuario fallido: límite alcanzado");
        printf("\nPulsa una tecla para continuar...");
        getchar();
        system("clear");
        return;
    }

    // Asignamos el número de cuenta
    if (tabla->num_cuentas > 0)
        numeroCuentaCliente = tabla->cuentas[tabla->num_cuentas - 1].numero_cuenta + 1;
    else
        numeroCuentaCliente = 1000; // Si el primer usuario lo inicializamos a 1000

    printf("👤 Introduce el nombre del titular: ");
    fgets(nombre, sizeof(nombre), stdin);
    nombre[strcspn(nombre, "\n")] = 0;
    
    // Mensaje de progreso añadido
    printf("\n🔄 Creando nueva cuenta...\n\n");
    fflush(stdout); // Asegurar que se muestre inmediatamente
    sleep(2); // Pequeña pausa para efecto visual

    // Generar saldo aleatorio entre 1000 y 10000
    srand(time(NULL));
    saldo = rand() % (10000 - 1000 + 1) + 1000;

    // Guardamos en la memoria compartida la cuenta
    Cuenta nueva;
    nueva.numero_cuenta = numeroCuentaCliente;
    strcpy(nueva.titular, nombre);
    nueva.saldo = saldo;
    nueva.num_transacciones = numeroTransacciones;

    tabla->cuentas[tabla->num_cuentas] = nueva; // Guardamos al nuevo usuario en la posición libre
    tabla->num_cuentas++; // Incrementados el contador de cuentas

    // Confirmación visual
    printf("\n┌───────────────────────────────────────┐\n");
    printf("│          ✅ CUENTA CREADA             │\n");
    printf("├───────────────────────────────────────┤\n");
    printf("│  Titular:     %-23s │\n", nombre);
    printf("│  N° Cuenta:   %-23d │\n", numeroCuentaCliente);
    printf("│  Saldo:       €%-22d │\n", saldo);
    printf("└───────────────────────────────────────┘\n");

    EscribirLog("Nuevo usuario registrado en mc correctamente");

    sem_post(semaforo_cuentas); 

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


void GuardarEnArchivo() {
    sem_wait(semaforo_cuentas); // Asegura exclusión mutua

    FILE *archivo = fopen(configuracion.archivo_cuentas, "w");
    if (!archivo) {
        perror("Error al abrir el archivo de cuentas para guardar");
        sem_post(semaforo_cuentas);
        EscribirLog("Error al intentar guardar las cuentas de mc a cuentas.dat");
        return;
    }

    for (int i = 0; i < tabla->num_cuentas; i++) {
        Cuenta c = tabla->cuentas[i];
        fprintf(archivo, "%d,%s,%.2f,%d\n", c.numero_cuenta, c.titular, c.saldo, c.num_transacciones);
    }

    fclose(archivo);
    sem_post(semaforo_cuentas);
    EscribirLog("Cuentas guardadas en archivo correctamente");
}