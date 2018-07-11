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
void EnviarRecibirMensaje(int idsockc, char *msj);
void EditarArchivo();
void EnviarArchivoServidor(int idsockc);
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
			printf("3. Ejecutar Query Predefinida\n");
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

	write(idsockc, "salir", 5);

	close(idsockc);
}

void EnviarRecibirMensaje(int idsockc, char *msj)
{
	int nb;
	char buf[BUFFER];
	// Envia mensaje
	nb=strlen(msj);
	printf("Mensaje a enviar al servidor %d: %s\n",idsockc,msj);
	write(idsockc,msj,nb);

	sleep(1);

	// Recibe respuesta
	nb=read(idsockc,buf,BUFFER);
	buf[nb]='\0';
	printf("Recibido del servidor %d: %s\n", idsockc, buf);
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
	//char* opDB = (char *) malloc(2);
	//memset(opDB, 0, 2);

	//strcpy(opDB, SeleccionarBase());
	//char * op = (char *) malloc(1);
	//memset(op, 0, 1);
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

	// Escribimos comando para enviar un archivo

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
	// sleep(2);

	char * buf = (char *)malloc(BUFFER);
	int nb;
	char * nombreArchivo = (char *)malloc(50);
	memset(nombreArchivo, 0, 50);
	FILE *archivo;

	//char * query = (char *)malloc(BUFFER);
	//memset(query, 0, BUFFER);

	char query[2048];
	char enter;

	/*printf("Lista de archivos disponibles: \n");
	system("ls");
	printf("Ingrese el nombre del archivo que quiere enviar: \n");

	scanf("%s", &nombreArchivo[0]);

	if((archivo = fopen(nombreArchivo, "rb")) == NULL)
	{
		printf("\nNo se puede abrir el archivo!");
		exit(0);
	}*/

 //   strcpy(query, "select * from empleado");
	printf("\nIngrese la query: \n");
	//fflush(stdin);
	scanf("%c", &enter);
	gets(query);
	printf("\nQuery: %s", query);
	write(idsockc, query, BUFFER);

	sleep(1);

	/*int seguirEnviando = 0;
	buf[0]='\0';
	printf("\nBuffer1... %s \n", buf);
	// Leemos la data y la enviamos
	while(seguirEnviando == 0)
	{
		// Particionamos el archivo en 256 bytes
		// unsigned char buff[1024] = {0};
		int nread = fread(buf, 1, BUFFER, archivo);
		printf("\nBuffer2... %s \n", buf);
		printf("\nBytes leidos %d \n", nread);

		if(nread > 0)
		{
			printf("Enviando... \n");
			write(idsockc, buf, nread);
		}
		if (nread < BUFFER)
		{
			if (feof(archivo))
			{
				printf("End of file\n");
				printf("File transfer completed for id: %d\n", idsockc);
				sleep(1);
				write(idsockc, "fin", 3);
			}
			if (ferror(archivo))
				printf("Error reading\n");

			printf("1\n");
			seguirEnviando++;
			fclose(archivo);
			printf("2\n");
		}
	}*/

	// Recibe el nombre del archivo
	nb = read(idsockc, buf, BUFFER);
	buf[nb] = '\0';
	printf("\nResultado de la query \n %s", buf);

	printf("\nPresione una tecla para volver.");
	getchar();
	getchar();

}

void EditarArchivo() {
	char * nombreArchivo = (char *) malloc(50);
	memset(nombreArchivo, 0, 50);

	printf("Lista de archivos disponibles: \n");
	system("ls");
	printf("Ingrese el nombre del archivo que quiere enviar: \n");

	scanf("%s", &nombreArchivo[0]);

	// Abrimos la edicion
	system("gedit archivo1.txt");
}
void MostrarCatalogo(int idsockc)
{
    char * buf = (char *)malloc(BUFFER);
	int nb;
    write(idsockc, "3", 1);//mostrar catalogo
    sleep(1);
    nb = read(idsockc, buf, BUFFER);
	buf[nb] = '\0';
	printf("\nCatalogo MySQL: \n %s", buf);

	nb = read(idsockc, buf, BUFFER);
	buf[nb] = '\0';
	printf("\nCatalogo PostgreSQL: \n %s", buf);

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

	// Escribimos comando para enviar un archivo
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
	//times = buf[i++];
	buf[nb] = '\0';
	printf("\nCantidad de consutlas: \n %s", buf);
	//printf("\nCantidad de consutlas: \n %d", times);

    for(i=0; i<3; i++){
        nb = read(idsockc, buf, BUFFER);
        buf[nb] = '\0';
        printf("\nConsulta: \n %s", buf);
    }

	printf("\nPresione una tecla para volver.");
	getchar();
	getchar();
}
