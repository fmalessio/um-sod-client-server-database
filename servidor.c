#include <libpq-fe.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <mysql.h>

#define IPSerBD "192.168.1.105"
#define PuertoSerBD 6666
#define BUFFER 1024

#define IPPostgrsql "127.0.0.1"
#define PuertoPostgresql "5432"

void funcionPostgresql(int idsockc, char * query);
void funcionMysql(int idsockc, char * query);
void funcionHilo(void);
void EjecutarQueryYEnviarResultado(int idsockc);
void MostrarCatalogo(int idsockc);
char * DetectarTipoQuery(char * query);

struct sockaddr_in c_sock;
int idsockc=0;

int main(int argc, char *argv[])
{
	struct sockaddr_in s_sock;
	int idsocks;
	int lensock = sizeof(struct sockaddr_in);
	idsocks = socket(AF_INET, SOCK_STREAM, 0);
	printf("idsocks %d\n", idsocks);

	s_sock.sin_family      = AF_INET;
	s_sock.sin_port        = htons(PuertoSerBD);
	s_sock.sin_addr.s_addr = inet_addr(IPSerBD);
	memset(s_sock.sin_zero,0,8);
	printf("bind %d\n", bind(idsocks,(struct sockaddr *) &s_sock,lensock));
	printf("listen %d\n", listen(idsocks,5));

	while(1)
	{
		printf("Servidor esperando conexion...\n");
		idsockc = accept(idsocks,(struct sockaddr *)&c_sock,&lensock);
		if(idsockc != -1)
		{
			pthread_t hilo;
			pthread_create(&hilo, NULL, (void*)funcionHilo, NULL);
		}
		else
		{
			printf("Conexion rechazada %d!\n", idsockc);
		}
	}

	return 0 ;
}

void funcionHilo(void)
{
	// UbicacionDelCliente(c_sock);
	EjecutarQueryYEnviarResultado(idsockc);
	pthread_exit(0);
}

void funcionMysql(int idsockc, char * query)
{
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *server = "localhost";
	char *user = "root";
	char *password = "toor";
	char *database = "autosdb";
	int nb;

	conn = mysql_init(NULL);
   /* Connect to database */
   if (!mysql_real_connect(conn, server,
         user, password, database, 0, NULL, 0)) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      exit(1);
   }

   /* send SQL query */
//   if (mysql_query(conn, "show tables")) {
//      fprintf(stderr, "%s\n", mysql_error(conn));
//      exit(1);
//   }

//   res = mysql_use_result(conn);
//   /* output table name */
//   printf("MySQL Tables in mysql database:\n");
//   while ((row = mysql_fetch_row(res)) != NULL)
//      printf("%s \n", row[0]);

   /* send SQL query */
   if (mysql_query(conn, query)) { // "SELECT * from autos"
      fprintf(stderr, "%s\n", mysql_error(conn));
      exit(1);
   }

   res = mysql_use_result(conn);
   /* output table name */
   printf("Autos in mysql database:\n");

	unsigned int num_fields;
	unsigned int i;

	num_fields = mysql_num_fields(res);
	while ((row = mysql_fetch_row(res)))
	{
	   unsigned long *lengths;
	   lengths = mysql_fetch_lengths(res);
	   for(i = 0; i < num_fields; i++)
	   {
		   printf("[%.*s] ", (int) lengths[i],
				  row[i] ? row[i] : "NULL");
	   }
	   printf("\n");
	}

   /* close connection */
   mysql_free_result(res);
   mysql_close(conn);
   write(idsockc, "Respuesta", 1024);
}

void funcionPostgresql(int idsockc, char * query)
{
	PGconn *conn;
	PGresult *res;
	int i,j;
	char respuesta[BUFFER];
	memset(respuesta, 0, BUFFER);

	conn =  PQsetdbLogin(IPPostgrsql, PuertoPostgresql, NULL, NULL, "sodsql", "postgres", "toor");
	if (PQstatus(conn) != CONNECTION_BAD)
	{
		res = PQexec(conn, query); //"SELECT * FROM employees"
		if (res != NULL && PGRES_TUPLES_OK == PQresultStatus(res))
		{
			for (i = 0 ; i <= PQntuples(res)-1;  i++)
			{
				for (j = 0 ; j < PQnfields(res); j++)
				{
					strcat(respuesta,PQgetvalue(res,i,j));
					strcat(respuesta,"\t");
					printf("%s\t", PQgetvalue(res,i,j));
				}
				strcat(respuesta,"\n");
				printf("\n");
			}
			strcat(respuesta,"\0");
			PQclear(res);
		} else {
			printf("Fallo query");
		}
	} else {
		printf("Fallo conexion");
	}
	PQfinish(conn);
	write(idsockc, respuesta, 1024);
}

void EjecutarQueryYEnviarResultado(int idsockc)
{
	char * buf = (char *)malloc(BUFFER);
	int nb;
	char * tipoDB = (char *)malloc(10);
	memset(tipoDB, 0, 10);
	char * tipoQuery = (char *)malloc(10);
	memset(tipoQuery, 0, 10);
    char * query = (char *)malloc(BUFFER);

	printf("Conexion aceptada desde el cliente %d.\n", idsockc);

	// Leemos la opcion de la base (1=mysql, 2=postgres, 0=salir)
	nb = read(idsockc, tipoDB, 10);

	tipoDB[nb] = '\0';
	printf("\nRecibido del cliente %d: %s \n", idsockc, tipoDB);

	// Ciclo para ejecutar queries
	while(strncmp(tipoDB, "0", 1) != 0)
	{
		if (strncmp(tipoDB, "1", 1) == 0) // mysql
		{
		    memset(query, 0, BUFFER);
            nb = read(idsockc, query, BUFFER);
            printf("\nRecibido del cliente %d: query: %s \n", idsockc, query);
			tipoQuery = DetectarTipoQuery(query);

			printf("\nEjecutamos funcion MYSQL %s y tipo de query %s \n", tipoDB, tipoQuery);
			fflush(stdin);
			funcionMysql(idsockc, query);
		}
		if (strncmp(tipoDB, "2", 1) == 0) // postgres
		{
            memset(query, 0, BUFFER);
            nb = read(idsockc, query, BUFFER);
            printf("\nRecibido del cliente %d: query: %s \n", idsockc, query);
			tipoQuery = DetectarTipoQuery(query);

			printf("\nEjecutamos funcion POSTGRES %s y tipo de query %s \n", tipoDB, tipoQuery);
			fflush(stdin);
			funcionPostgresql(idsockc, query);
		}
        if (strncmp(tipoDB, "3", 1) == 0) // catalogo
		{
			printf("\nEjecutamos print de catalogo \n");
			fflush(stdin);
			MostrarCatalogo(idsockc);
		}
		printf("\nIterando ciclo de comandos\n");
		sleep(1);
		nb = read(idsockc, tipoDB, BUFFER);
		buf[nb]='\0';
		printf("\nRecibido del cliente %d: %s\n", idsockc, tipoDB);
	 }
	printf("\nCerrando conexión del cliente %d.\n", idsockc);
	close(idsockc);
}
void MostrarCatalogo(int idsockc){
    //TODO refactor
    printf("\nMostrar catalogo \n");

    funcionMysql(idsockc, "SELECT table_name FROM information_schema.tables where table_schema='autosdb';");
    funcionPostgresql(idsockc, "SELECT table_name FROM information_schema.tables WHERE table_schema='public' AND table_type='BASE TABLE';");
}

// CRUD (C=create, R=read, U=update, D=delete)
char* DetectarTipoQuery(char* query) {
    char create = 'C';
    char read = 'R';
	char update = 'U';
    char delete = 'D';
	char otro = 'X'; // tipo desconocido
    char * tipoQuery = malloc(2 * sizeof(char));

	char * inicioQuery = (char *)malloc(BUFFER); // max para query
	memset(inicioQuery, 0, BUFFER);
	strcpy(inicioQuery, query);

	strtok(inicioQuery, " ");
    if(inicioQuery == NULL) {
		tipoQuery[0] = otro;
	}
	if(strncmp(inicioQuery, "SELECT", 1) == 0) {
		tipoQuery[0] = read;
	}

	// cierro strig
    tipoQuery[2] = '\0';
    return tipoQuery;
}
