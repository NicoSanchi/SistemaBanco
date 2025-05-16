#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
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
#include <errno.h>
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
void InicializarDirectoriosTransacciones();
void CrearDirectorioUsuario(int numero_cuenta);
//void *llamar_trasladar_datos(void *arg);
void trasladar_datos();
void *llamar_sacar_cuenta_buffer(void *arg);

int main()
{
    // Configuración inicial de semáforos
    inicializar_semaforos();
    conectar_semaforos();
    CrearColaMensajes();

    inicializar_configuracion();

    // Creamos la memoria compartida
    CrearMemoriaCompartida();

    // Inicializamos los directorios de transacciones
    InicializarDirectoriosTransacciones();

    signal(SIGUSR1, LeerAlertas);  // Manejar señal del monitor
    signal(SIGINT, ManejarSenial); // Si recibe una señal de SIGINT que es de Ctrl C, libera los recursos

    IniciarMonitor();

    
    pthread_t hilo_buffer;
    pthread_create(&hilo_buffer, NULL, llamar_sacar_cuenta_buffer, NULL);
    

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
            trasladar_datos();
            destruir_semaforos();
            DesconectarMC();
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

    // Declaración de variables
    int i, indiceLRU;
    bool encontradoEnMC = false, encontradoEnArchivo = false;
    FILE *archivo;
    char linea[100];
    char numero_cuenta[10];
    int numero_cuenta_en_int;
    char titular[50];
    Cuenta cuentaEncontrada;

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
    sleep(1);


    // Bloqueo para acceder a la memoria compartida
    //sem_wait(semaforo_cuentas);

    // Buscar en memoria compartida (MC)
    for (i = 0; i < tabla->num_cuentas; i++)
    {
        if (tabla->cuentas[i].numero_cuenta == numero_cuenta_en_int &&
            strcmp(tabla->cuentas[i].titular, titular) == 0)
        {
            encontradoEnMC = true; // Credenciales encontradas en MC
            tabla->cuentas[i].ultimoAcceso = time(NULL); // Actualizar último acceso
            EscribirLog("Usuario encontrado en memoria compartida");
            break;
        }
    }

    // Liberar el semáforo tras acceder a MC
    //sem_post(semaforo_cuentas);

    if (encontradoEnMC)
    {
        // Si las credenciales están en MC, se permite el inicio de sesión
        printf("✅ Autenticación exitosa (Memoria)\n\n");
        printf("🚀 Abriendo tu sesión bancaria...\n");
        sleep(1);
        EscribirLog("El usuario ha iniciado sesión correctamente");
    }
    else
    {
        // Si no están en MC, buscar en el archivo
        archivo = fopen(configuracion.archivo_cuentas, "r");
        if (archivo == NULL)
        {
            perror("Error al abrir el archivo de cuentas");
            EscribirLog("Error al abrir el archivo cuentas intentando iniciar sesión");
            exit(EXIT_FAILURE);
        }
        EscribirLog("Archivo de cuentas abierto correctamente intentando iniciar sesión");

        // Leer el archivo línea por línea
        while (fgets(linea, sizeof(linea), archivo))
        {
            char *token = strtok(linea, ",");
            if (token != NULL)
            {
                int numero_cuenta_archivo = atoi(token); // Leer número de cuenta
                token = strtok(NULL, ",");
                char titular_archivo[50];
                strncpy(titular_archivo, token, sizeof(titular_archivo) - 1);
                titular_archivo[sizeof(titular_archivo) - 1] = '\0'; // Leer titular

                // Comparar credenciales
                if (numero_cuenta_archivo == numero_cuenta_en_int &&
                    strcmp(titular_archivo, titular) == 0)
                {
                    // Guardar los datos de la cuenta encontrada
                    cuentaEncontrada.numero_cuenta = numero_cuenta_archivo;
                    strncpy(cuentaEncontrada.titular, titular_archivo, sizeof(cuentaEncontrada.titular) - 1);
                    cuentaEncontrada.saldo = atof(strtok(NULL, ","));
                    cuentaEncontrada.num_transacciones = atoi(strtok(NULL, ","));
                    encontradoEnArchivo = true;
                    break;
                }
            }
        }

        fclose(archivo); // Cerrar el archivo
        EscribirLog("Se ha cerrado el archivo de cuentas correctamente intentando iniciar sesión");

        if (encontradoEnArchivo)
        {
            // Si las credenciales están en el archivo, se permite el inicio de sesión
            printf("✅ Autenticación exitosa (Archivo)\n\n");
            printf("🚀 Abriendo tu sesión bancaria...\n");
            sleep(1);
            EscribirLog("El usuario ha iniciado sesión correctamente");

            // Desalojar al usuario que lleva más tiempo sin acceder (LRU)
            //sem_wait(semaforo_cuentas);

            indiceLRU = EncontrarLRU(); // Encontrar el índice del LRU
            tabla->cuentas[indiceLRU] = cuentaEncontrada; // Reemplazar con la nueva cuenta
            tabla->cuentas[indiceLRU].ultimoAcceso = time(NULL); // Actualizar último acceso

            EscribirLog("Cuenta cargada en memoria compartida, desalojando al que lleva mayor tiempo sin acceder");

            //sem_post(semaforo_cuentas);
        }
        else
        {
            // Si las credenciales no están en MC ni en el archivo, mostrar error
            printf("❌ Error: Credenciales incorrectas\n");
            EscribirLog("El usuario ha intentado iniciar sesión. Fallo al introducir las credenciales");
            printf("\nPresione una tecla para continuar...");
            getchar();
            system("clear");
            return;
        }
    }

    // Crear un proceso hijo para manejar la sesión del usuario
    pid_t pid_banco = getpid(); // PID del padre 
    pid_t pid = fork();
    if (pid == 0)
    {
        // Proceso hijo: compilar y ejecutar el programa del usuario
        char comando[300];
        snprintf(comando, sizeof(comando), "gcc usuario.c comun.c -o usuario -lpthread && ./usuario '%s' '%s' %d", numero_cuenta, titular, pid_banco);
        // Ejecutar gnome-terminal y correr el comando
        execlp("gnome-terminal", "gnome-terminal", "--", "bash", "-c", comando, NULL);
        exit(EXIT_FAILURE); // Si execlp falla
    }
    else if (pid > 0) 
    {
        // Proceso padre: espera breve para dar tiempo al hijo
        usleep(550000); // 0.5 segundos
    }
    else if (pid == -1)
    {
        // Error al crear el proceso hijo
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

    // Bloqueo del semáforo para acceso seguro al archivo
    sem_wait(semaforo_cuentas);

    // Abrir el archivo de cuentas en modo append para agregar nuevos usuarios
    ficheroUsers = fopen(configuracion.archivo_cuentas, "a+");
    if (ficheroUsers == NULL)
    {
        perror("Error a la hora de abrir el archivo");
        EscribirLog("Fallo al abrir el archivo de usuarios");

        sem_post(semaforo_cuentas); // Liberar el semáforo
        fclose(ficheroUsers);
        return;
    }
    else
        EscribirLog("Se ha abierto el archivo de cuentas");


    // Leer el archivo línea por línea para encontrar el último número de cuenta
    while (fgets(linea, sizeof(linea), ficheroUsers))
    {
        char *token = strtok(linea, ","); // Tomamos el numero de cuenta de la linea

        if (token != NULL)
        { 
            // Si el archivo no esta vacio, asignamos al numero de cuenta el numero de cuenta del ultimo cliente
            numeroCuentaCliente = atoi(token);
            hayUsuarios = true;
        }
    }

    // Si habia usuarios existentes, asigna el nuevo numero de cuenta siguiente
    if (hayUsuarios) 
        numeroCuentaCliente++;
    else
        numeroCuentaCliente = 1000; // Si no inicializa a 1000 el numero de cuenta

    // Solicitar el nombre del titular
    printf("👤 Introduce el nombre del titular: ");
    fgets(nombre, sizeof(nombre), stdin);
    nombre[strcspn(nombre, "\n")] = 0; // Eliminar salto de línea

    // Mensaje de progreso
    printf("\n🔄 Creando nueva cuenta...\n\n");
    fflush(stdout); // Asegurar que se muestre inmediatamente
    sleep(1);       // Pequeña pausa para efecto visual

    // Generar saldo aleatorio (1000-10000)
    srand(time(NULL));
    saldo = rand() % (10000 - 1000 + 1) + 1000; // Generamos un numero entre 1000 y 10000 que sera su saldo

    // Moverse al final del archivo para agregar el nuevo usuario
    fseek(ficheroUsers, -1, SEEK_END); 
    char ultimoCaracter = fgetc(ficheroUsers);

    // si hay usuarios y si el ultimo caracter no es un salto de linea, lo añadimos manualmente
    if (ultimoCaracter != '\n' && hayUsuarios) 
        fprintf(ficheroUsers, "\n");           

    // Escribir los datos del nuevo usuario en el archivo
    fprintf(ficheroUsers, "%d,%s,%d,%d", numeroCuentaCliente, nombre, saldo, numeroTransacciones); 

    // Confirmación visual
    printf("\n┌───────────────────────────────────────┐\n");
    printf("│          ✅ CUENTA CREADA             │\n");
    printf("├───────────────────────────────────────┤\n");
    printf("│  Titular:     %-23s │\n", nombre);
    printf("│  N° Cuenta:   %-23d │\n", numeroCuentaCliente);
    printf("│  Saldo:       €%-22d │\n", saldo);
    printf("└───────────────────────────────────────┘\n");

    EscribirLog("Nuevo usuario registrado");

    CrearDirectorioUsuario(numeroCuentaCliente); // Crear el directorio del usuario

    // Guardar el nuevo usuario en la memoria compartida si hay espacio disponible
    if (tabla->num_cuentas < CUENTAS_TOTALES)
    {
        sem_wait(semaforo_cuentas); // Bloqueo del semáforo para acceso seguro a la memoria compartida

        // Crear una nueva cuenta
        Cuenta nuevaCuenta;
        nuevaCuenta.numero_cuenta = numeroCuentaCliente;
        strncpy(nuevaCuenta.titular, nombre, sizeof(nuevaCuenta.titular));
        nuevaCuenta.saldo = saldo;
        nuevaCuenta.num_transacciones = numeroTransacciones;
        nuevaCuenta.ultimoAcceso = time(NULL); // Registrar el tiempo actual como último acceso

        // Agregar la nueva cuenta a la memoria compartida
        tabla->cuentas[tabla->num_cuentas] = nuevaCuenta;
        tabla->num_cuentas++; // Incrementar el contador de cuentas en memoria compartida

        EscribirLog("Nuevo usuario añadido también a memoria compartida");

        sem_post(semaforo_cuentas); // Liberar el semáforo
    }
    else
    {
        // Si no hay espacio en memoria compartida, registrar en el log
        EscribirLog("No hay espacio en memoria compartida para el nuevo usuario. Solo se guardará en el archivo.");
    }

    // Cerrar el archivo
    fclose(ficheroUsers);
    sem_post(semaforo_cuentas);     // Liberar el semáforo
    EscribirLog("Se ha cerrado el archivo de cuentas");

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
    int indiceLRU = 0;

    for (int i = 0; i < tabla->num_cuentas; i++) {

        if (tabla->cuentas[i].ultimoAcceso < min) {
            min = tabla->cuentas[i].ultimoAcceso;
            indiceLRU = i;
        }
    }

    if (tabla->num_cuentas == 0) {
        return 0;
    }

    return indiceLRU;
}


// Función para inicializar directorios
void InicializarDirectoriosTransacciones() {
    // Crear directorio principal
    if (mkdir("./transacciones", 0777) == -1 && errno != EEXIST) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error al crear directorio principal de transacciones: %s", strerror(errno));
        perror(error_msg);
        EscribirLog("Error al crear directorio principal de transacciones");
        return;
    }
    
    // Crear directorios para cuentas existentes
    //sem_wait(semaforo_cuentas);
    for (int i = 0; i < tabla->num_cuentas; i++) {
        CrearDirectorioUsuario(tabla->cuentas[i].numero_cuenta);
    }
    //sem_post(semaforo_cuentas);
}

// Función para crear directorios de cada usuario
void CrearDirectorioUsuario(int numero_cuenta) {
    char path[256];
    
    // Crear directorio principal "transacciones" si no existe
    snprintf(path, sizeof(path), "./transacciones");
    
    if (mkdir(path, 0777) == -1 && errno != EEXIST) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error al crear directorio transacciones: %s", strerror(errno));
        perror(error_msg);
        EscribirLog("Error al crear directorio transacciones");
        return;
    }
    
    // Crear directorio del usuario
    snprintf(path, sizeof(path), "./transacciones/%d", numero_cuenta);
    
    if (mkdir(path, 0777) == -1 && errno != EEXIST) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error al crear directorio para cuenta %d: %s", numero_cuenta, strerror(errno));
        perror(error_msg);
        EscribirLog(error_msg);
    }
}

//void *llamar_trasladar_datos(void *arg){
    //while(1){
        //sem_wait(semaforo_memoria_compartida);
        //trasladar_datos();
        //sem_post(semaforo_memoria_compartida);
        //sleep(10);
    //}
    //return NULL;
//}

void trasladar_datos(){
    FILE *fichero_original;
    fichero_original = fopen(configuracion.archivo_cuentas, "r");
    if(!fichero_original){
        perror("Error a la hora de abrir el archivo de cuentas.");
        EscribirLog("Error a la hora de abrir el archivo de cuentas.");
        return;
    }
    FILE *fichero_temporal;
    fichero_temporal = fopen("archivo_temporal.txt", "w");
    if(!fichero_temporal){
        perror("Error a la hora de crear el archivo temporal.");
        EscribirLog("Error a la hora de crear el archivo temporal.");
        fclose(fichero_original);
        return;
    }
    sem_wait(semaforo_memoria_compartida);
    char linea[256];
    while(fgets(linea, sizeof(linea), fichero_original)){
        int num_cuenta_archivo;
        sscanf(linea, "%d", &num_cuenta_archivo);

        int encontrado = 0;
        int i;
        for(i=0; i<tabla->num_cuentas; i++){
            if(tabla->cuentas[i].numero_cuenta == num_cuenta_archivo){
                // Escribimos la cuenta desde memoria compartida
                fprintf(fichero_temporal, "%d,%s,%f,%d,%ld\n", tabla->cuentas[i].numero_cuenta, tabla->cuentas[i].titular, tabla->cuentas[i].saldo, tabla->cuentas[i].num_transacciones, tabla->cuentas[i].ultimoAcceso);
                encontrado = 1;
                break;
            }
        }
        if(!encontrado)
            fputs(linea, fichero_temporal);
    }
    fclose(fichero_original);
    fclose(fichero_temporal);

    remove(configuracion.archivo_cuentas);
    rename("archivo_temporal.txt", configuracion.archivo_cuentas);
    sem_post(semaforo_memoria_compartida);
    return;
}

void *llamar_sacar_cuenta_buffer(void *arg)
{
    while(1){
        EscribirLog("Se está entrando a la función sacar buffer");
        SacarCuentaBuffer();
        EscribirLog("Se sale de la función sacar buffer");
        sleep(5);
    }
}

//void *gestionar_entrada_salida(void *arg)
//{
    //while(1)
    //{
        //if(buffer.inicio!=buffer.fin){
            /*sem_wait(semaforo_buffer);
            Cuenta op = buffer.operacion[buffer.inicio];
            buffer.inicio = (buffer.inicio+1)%10;
            FILE *archivo = fopen(configuracion.archivo_cuentas, "rb+");
            sem_wait(semaforo_cuentas);
            fseek(archivo, op.numero_cuenta * sizeof(Cuenta), SEEK_SET);
            fwrite(&op, sizeof(Cuenta), 1, archivo);
            sem_post(semaforo_cuentas);
            sem_post(semaforo_buffer);
            fclose(archivo);
            sleep(10);
        }
    }
}*/