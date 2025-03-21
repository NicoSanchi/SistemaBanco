#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define MAX_LINE 200

void MostrarMenu();
void EscribirLog(const char *mensaje);
void ComprabarUsuario();
void RegistrarUsuario();
void MenuProcesos();

int main() {
	
    return(0);
}

void MenuProcesos() {

}

void MostrarMenu() {

	int opcion;
	printf("Hola bienvenido a este banco virtual!!\n");
	printf("Pulsa 1 para iniciar sesion");
	printf("Pulsa 2 para registrarte\n");
	scanf("%d", &opcion);

	switch (opcion)
	{
	case 1:
		ComprobarUsuario();
		break;
	case 2:
		RegistrarUsuario();
		break;
	default:
		printf("Opcion no válida");
		break;
	}

	return;
}

void ComprabarUsuario() {
	FILE *ficheroUsuarios;
	char usuarioIngresado[100], contraseniaIngresada[100], linea[MAX_LINE];
	int usuarioEncontrado;

	ficheroUsuarios = fopen("cuentas.dat", "r");

	if (ficheroUsuarios == NULL) {
		ficheroUsuarios = fopen("cuentas.dat", "w");
		printf("No hay usuarios registrados. Registrate primero");
		return;
	}

	printf("Introduce el nombre de usuario:\n");
	fgets(usuarioIngresado, sizeof(usuarioIngresado), stdin);
	printf("Introduce la contraseña:\n");
	fgets(contraseniaIngresada, sizeof(contraseniaIngresada), stdin);

	while (fgets(linea, sizeof(linea), stdin))
	{
		char* nombre = strtok(linea, ";");
		char* contrasenia = strtok(NULL, "\n");

		if (nombre != NULL && contrasenia != NULL) {
			if (strcmp(nombre, usuarioIngresado) == 0 && strcmp(contrasenia, contraseniaIngresada) == 0) {
                usuarioEncontrado = 1;
                break;
            }
		}
	}

	fclose(ficheroUsuarios);

	

}

void RegistrarUsuario() {
	FILE *ficheroUsers;
	char nombre[100], contrasenia[100], repetirConstrasenia[100];

    // Hacerlo con struct

	ficheroUsers = fopen("cuentas.dat", "a");

	
	printf("Introduce el nombre de usuario que quieres\n");
	fgets(nombre, sizeof(nombre), stdin);

	do {
		printf("Introduce la contraseña que quieras\n");
		fgets(contrasenia, sizeof(contrasenia), stdin);
		printf("Repite la contraseña");
		fgets(repetirConstrasenia, sizeof(repetirConstrasenia), stdin);

	} while (repetirConstrasenia != contrasenia);
	{
		printf("Las contraseñas no coinciden");
	}

	fprintf(ficheroUsers, "%s;%s", nombre, contrasenia);

	fclose(ficheroUsers);

}

void EscribirLog(const char *mensaje) {
    FILE* fichero;
    fichero = fopen("registro.log", "a");

    if (fichero == NULL)
        return;

    time_t tiempo;
    struct tm *tm_info;
    char hora[26];

    time(&tiempo);
    tm_info = localtime(&tiempo);
    strftime(hora, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(fichero, "[%s] %s\n", hora, mensaje);

    fclose(fichero);
    
}
