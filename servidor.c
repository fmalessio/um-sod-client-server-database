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
void EjecutarConsultasPredefinidasPostgreSQL(int idsockc);
void EjecutarConsultasPredefinidasMySQL(int idsockc);
char * DetectarTipoQuery(char * query);
FILE * log_file;
void logger(char * message);

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

    logger("Iniciando la aplicacion");

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

    return 0;
}

void funcionHilo(void)
{
    logger("Hilo creado");
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
	char * tipoQuery = (char *)malloc(10);
	memset(tipoQuery, 0, 10);

    char * respuesta = (char *)malloc(BUFFER);
    memset(respuesta, 0, BUFFER);

    tipoQuery = DetectarTipoQuery(query);

    conn = mysql_init(NULL);
    /* Conectamos a mysql */
    if (!mysql_real_connect(conn, server,
            user, password, database, 0, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        write(idsockc, mysql_error(conn), BUFFER);
        mysql_close(conn);
        return;
    }

	// R: Opcion read. Ejemplo: "SELECT * from autos"
	if(strncmp(tipoQuery, "R", 1) == 0) {

        /* Ejecucion de query */
        if (mysql_query(conn, query)) {
            fprintf(stderr, "%s\n", mysql_error(conn));
            write(idsockc, mysql_error(conn), BUFFER);
            mysql_close(conn);
            return;
        }

        res = mysql_use_result(conn);

        unsigned int num_fields;
        unsigned int i;
		num_fields = mysql_num_fields(res);
        while ((row = mysql_fetch_row(res)))
        {
            for(i = 0; i < num_fields; i++)
            {
                strcat(respuesta, row[i]);
                strcat(respuesta, "\t");
            }
            strcat(respuesta, "\n");
        }
        printf("%s\n", respuesta);

		mysql_free_result(res);
        write(idsockc, respuesta, BUFFER);
	}	
	else {
		// C: create, U: update, D: delete.
        // Ejemplo: "UPDATE autos SET color = 'rojo' WHERE id = 1"
		MYSQL_STMT  * stmt;
		stmt = mysql_stmt_init(conn);
		if (!stmt)
		{
			printf(" mysql_stmt_init(), Out of memory\r\n");
			mysql_close(conn);
			return;
		}
		if (mysql_stmt_prepare(stmt, query, strlen(query)))
		{
			printf("mysql_stmt_prepare(), error\r\n");
			printf("Error: %s\r\n", mysql_stmt_error(stmt));
            write(idsockc, mysql_stmt_error(stmt), BUFFER);
            mysql_close(conn);
			return;
		}
        /* Ejecutar query */
		if (mysql_stmt_execute(stmt))
		{
			printf("mysql_stmt_execute(), error\r\n");
			printf("Error: %s\r\n", mysql_stmt_error(stmt));
            write(idsockc, mysql_stmt_error(stmt), BUFFER);
            mysql_close(conn);
			return;
		}
        /* Cerrar statement */
        if (mysql_stmt_close(stmt))
        {
            printf("Error cerrando statement\r\n");
            printf("%s\r\n", mysql_stmt_error(stmt));
            write(idsockc, mysql_stmt_error(stmt), BUFFER);
            mysql_close(conn);
			return;
        }

		write(idsockc, "Query ejecutada corractamente.", BUFFER);
	}

	/* Cerramos conexion */
	mysql_close(conn);
    logger("MySQL termino con exito");
	printf("\nFin ejecución MySQL correcto");
}


void funcionPostgresql(int idsockc, char * query)
{
    PGconn *conn;
    PGresult *res;
    int i,j;
    char respuesta[BUFFER];
    memset(respuesta, 0, BUFFER);
    char * tipoQuery = (char *)malloc(10);
    memset(tipoQuery, 0, 10);

    tipoQuery = DetectarTipoQuery(query);

    // Creamos conexion
    conn =  PQsetdbLogin(IPPostgrsql, PuertoPostgresql, NULL, NULL, "sodsql", "postgres", "toor");
    if (PQstatus(conn) != CONNECTION_BAD)
    {
        // Ejecutamos query
        res = PQexec(conn, query);

	    if(strncmp(tipoQuery, "R", 1) == 0) {
            // R: read. Ejemplo: "SELECT * FROM employees"
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
        }
        else {
            // C: create, U: update, D: delete.
            if(res != NULL && PGRES_COMMAND_OK == PQresultStatus(res))
            {
                strcat(respuesta, "Query ejecutada corractamente.");                
            } else
            {
                strcat(respuesta, "Error ejecutando la query.");
            }
            strcat(respuesta,"\0");
        }
    } else {
        printf("Fallo conexion");
    }

    // Cerramos conexion
    PQfinish(conn);
    logger("Postgres termino con exito");
    write(idsockc, respuesta, BUFFER);
}

void EjecutarQueryYEnviarResultado(int idsockc)
{
    char * buf = (char *)malloc(BUFFER);
    int nb;
    char * tipoDB = (char *)malloc(10);
    memset(tipoDB, 0, 10);
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
            logger("MySQL seleccionado");
            memset(query, 0, BUFFER);
            nb = read(idsockc, query, BUFFER);
            printf("\nRecibido del cliente %d: query: %s \n", idsockc, query);
            logger(query);

            printf("\nEjecutamos funcion MYSQL %s", tipoDB);
            fflush(stdin);
            funcionMysql(idsockc, query);
        }
        if (strncmp(tipoDB, "2", 1) == 0) // postgres
        {
            logger("PostgreSQL seleccionado");
            memset(query, 0, BUFFER);
            nb = read(idsockc, query, BUFFER);
            printf("\nRecibido del cliente %d: query: %s \n", idsockc, query);
            logger(query);

            printf("\nEjecutamos funcion POSTGRES %s", tipoDB);
            fflush(stdin);
            funcionPostgresql(idsockc, query);
        }
        if (strncmp(tipoDB, "3", 1) == 0) // catalogo
        {
            logger("Se ha ejecutado mostrar catalogo");
            printf("\nEjecutamos print de catalogo \n");
            fflush(stdin);
            MostrarCatalogo(idsockc);
        }
        if ( (strncmp(tipoDB, "4", 1) == 0) || (strncmp(tipoDB, "5", 1) == 0)) // catalogo
        {
            printf("\nEjecutamos print de catalogo \n");
            fflush(stdin);
            if(strncmp(tipoDB, "4", 1) == 0){
                EjecutarConsultasPredefinidasMySQL(idsockc);
            }
            else{
                EjecutarConsultasPredefinidasPostgreSQL(idsockc);
            }
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

void EjecutarConsultasPredefinidasMySQL(int idsockc){
    printf("\nConsultas Predefinidas MySQL:\n");

    write(idsockc, "1", 1);

    funcionMysql(idsockc, "SELECT * from autos");
}

void EjecutarConsultasPredefinidasPostgreSQL(int idsockc){
    printf("\nConsultas Predefinidas PostgreSQL\n");

    write(idsockc, "3", 1); // para mas de 9 revisar

    funcionPostgresql(idsockc, "SELECT * from employees");
    funcionPostgresql(idsockc, "SELECT * from pepe");
    funcionPostgresql(idsockc, "SELECT * from empleado");
}

// CRUD (C=create, R=read, U=update, D=delete)
char* DetectarTipoQuery(char* query) {
    char create = 'C';
    char read = 'R';
    char update = 'U';
    char delete = 'D';
    char otro = 'X'; // tipo desconocido
    char * tipoQuery = malloc(2 * sizeof(char));

    char * inicioQuery = (char *)malloc(BUFFER);
    memset(inicioQuery, 0, BUFFER);
    strcpy(inicioQuery, query);

    strtok(inicioQuery, " ");
    if(inicioQuery == NULL) {
        tipoQuery[0] = otro;
    }
    if(strncmp(inicioQuery, "SELECT", 1) == 0) {
        tipoQuery[0] = read;
    }
    if(strncmp(inicioQuery, "INSERT", 1) == 0) {
        tipoQuery[0] = create;
    }
    if(strncmp(inicioQuery, "UPDATE", 1) == 0) {
        tipoQuery[0] = update;
    }
    if(strncmp(inicioQuery, "DELETE", 1) == 0) {
        tipoQuery[0] = delete;
    }

    // cierro string
    tipoQuery[2] = '\0';
    printf("\nTipo de query detectado: %s", tipoQuery);
    return tipoQuery;
}

void logger(char * message) {
    log_file = fopen("log.log", "a+");
    if(log_file) {
        fputs("\n", log_file);
        fputs(message, log_file);
        fclose(log_file);
    }
}