gcc monitor.c comun.c -o monitor -pthread
gcc banco.c comun.c -o banco -pthread
gcc usuario.c comun.c -o usuario -pthread