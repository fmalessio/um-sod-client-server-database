#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define ipServidor "192.168.1.105"
#define puertoServidor 6666
#define BUFFER 1024

void EnviarQuery(int idsockc);
void MostrarCatalogo(int idsockc);
void EjecutarQueryPredefinida(int idsockc);
char* SeleccionarBase();

int main (int argc, char *argv[])
{
	// Conectar con el servidor de archivos
	int idsockc,idso;
	struct sockaddr_in c_sock;
	int lensock;

	idsockc = socket(AF_INET, SOCK_STREAM, 0);

	printf("Conexion con el servidor: %d.\n", idsockc);
	c_sock.sin_family = AF_INET;
	c_sock.sin_addr.s_addr = inet_addr(ipServidor);
	c_sock.sin_port = htons(puertoServidor);

	lensock = sizeof(struct sockaddr_in);
	lensock = sizeof(c_sock);

	idso = connect(idsockc,(struct sockaddr *)&c_sock,lensock);

	if(idso!=-1)
	{
		int op;
		do{
			system("clear");
			printf("************** \n");
			printf("**** MENU **** \n");
			printf("************** \n");
			printf("1. Ejecutar Query Manual \n");
			printf("2. Mostrar Catálogo \n");
			printf("3. Mostrar contenido de tablas\n");
			printf("0. Salir \n");
			printf("\n\nOpcion: ");

			scanf("%d",&op);

			fflush(stdin);

			switch(op)
			{
				case 1:
				{
					EnviarQuery(idsockc);
				} break;
				case 2:
				{
					MostrarCatalogo(idsockc);
				} break;
				case 3:
				{
					EjecutarQueryPredefinida(idsockc);
				} break;
			}
		} while(op != 0);
	}
	else
	{
		printf("Conexion rechazada! %d\n",idso);

	}

	printf("\nDesconectando del servidor: %d.\n\n", idsockc);
	write(idsockc, "0", 1);
	close(idsockc);
}

char* SeleccionarBase()
{
	char * op = (char *) malloc(2);
	memset(op, 0, 2);
	system("clear");
	printf("************** \n");
	printf("1. MySQL \n");
	printf("2. PostgreSQL\n");
	printf("0. Salir \n");
	printf("\n\nOpcion: ");

	scanf("%s",&op);
	fflush(stdin);

	return op;
}

void EnviarQuery(int idsockc)
{
	int op;
	system("clear");
	printf("************** \n");
	printf("1. MySQL \n");
	printf("2. PostgreSQL\n");
	printf("0. Salir \n");
	printf("\n\nOpcion: ");

	scanf("%d",&op);
	fflush(stdin);

	printf("\nBase de datos selccionada %d: ", op);

	// Seleccionamos opcion del menu
	if(op == 1){
		write(idsockc, "1", 1);
	}
	else {
		if(op == 2)
			write(idsockc, "2", 1);
		else {
			printf("\nPresione una tecla para volver.");
			getchar();
			return;
		}
	}

	char * buf = (char *)malloc(BUFFER);
	int nb;

	char query[2048];
	char enter;

	printf("\nIngrese la query: \n");
	scanf("%c", &enter);
	gets(query);

	printf("\nQuery: %s", query);
	write(idsockc, query, BUFFER);

	sleep(1);

	// Recibe el resultado de la query
	nb = read(idsockc, buf, BUFFER);
	buf[nb] = '\0';
	printf("\nResultado de la query \n%s", buf);

	printf("\nPresione una tecla para volver.");
	getchar();
	getchar();
}

void MostrarCatalogo(int idsockc)
{
    char * buf = (char *)malloc(BUFFER);
	int nb;
    write(idsockc, "3", 1);
    sleep(1);
    nb = read(idsockc, buf, BUFFER);
	buf[nb] = '\0';
	printf("\nCatalogo MySQL: \n%s", buf);

	nb = read(idsockc, buf, BUFFER);
	buf[nb] = '\0';
	printf("\nCatalogo PostgreSQL: \n%s", buf);

	printf("\nPresione una tecla para volver.");
	getchar();
	getchar();
}

void EjecutarQueryPredefinida(int idsockc){
	int op;
	system("clear");
	printf("************** \n");
	printf("\nEjecutar query predefinida para:\n");
	printf("1. MySQL \n");
	printf("2. PostgreSQL\n");
	printf("0. Salir \n");
	printf("\n\nOpcion: ");

	scanf("%d",&op);
	fflush(stdin);

	printf("\nBase de datos selccionada %d: ", op);

	// Seleccionamos opcion del menu
	if(op == 1){
		write(idsockc, "4", 1);
	}
	else {
		if(op == 2)
			write(idsockc, "5", 1);
		else {
			printf("\nPresione una tecla para volver.");
			getchar();
			return;
		}
	}

	char * buf = (char *)malloc(BUFFER);
	int nb, times, i;
    sleep(1);

	nb = read(idsockc, buf, 1);
	times = buf[0]-48;
	buf[nb] = '\0';

    for(i=0; i<times; i++){
        nb = read(idsockc, buf, BUFFER);
        buf[nb] = '\0';
        printf("\nConsulta tabla %d: \n%s", i, buf);
    }

	printf("\nPresione una tecla para volver.");
	getchar();
	getchar();
}
