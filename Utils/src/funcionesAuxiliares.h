/*
 * FuncionesAuxiliares.h
 *
 *  Created on: 26 abr. 2021
 *      Author: utnso
 */

#ifndef FUNCIONESAUXILIARES_H_
#define FUNCIONESAUXILIARES_H_

#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/bitarray.h>
#include <commons/temporal.h>
#include <stdint.h>
#include <pthread.h>
#include <stdint-gcc.h>
#include <time.h>
#include <math.h>
#include <semaphore.h>
#include <fcntl.h>




// NO SE USA MAS
typedef enum
{
	MENSAJE,
	PAQUETE,
	CHAT,
	MULTIHILO
} op_code;

//GENERALES
typedef struct inicioDeTripulante{
	int posX;
	int posY;
	int pid;
	int tid;
} inicioDeTripulante;

typedef struct inicioDePatota{
	t_list* listaTareas;
	int pid;
	int cantTripulantes;
	t_list* posiciones;
	int primerTid;
} inicioDePatota;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct Tarea{
	char* nombre;
	int parametro;
	int ubicacionEnX;
	int ubicacionEnY;
	int duracion;
} Tarea;

typedef struct tripulante{
	int tid;
	int pid;
	char estado;
} tripulante; // Arreglar

typedef enum
{
	INICIAR_PATOTA,
	INICIAR_TRIPULANTE,
	LISTAR_TRIPULANTES,
	INICIAR_PLANIFICACION,
	EXPULSAR_TRIPULANTE,
	PAUSAR_PLANIFICACION,
	OBTENER_BITACORA,
	PROXIMA_TAREA,
	CAMBIO_DE_ESTADO,
	POSICION_TRIPULANTE,
	CAMBIO_DE_POSICION,
	SABOTAJE,
	EXIT
} comandos_discordiador;

typedef enum
{
	INICIAR_BITACORA,
	GENERAR_RECURSOS,
	DESCARTAR_RECURSOS,
	MOVIMIENTO,
	COMIENZO_DE_TAREA,
	FINALIZACION_DE_TAREA,
	CORRIDA_EN_PANICO,
	SABOTAJE_RESUELTO,
	CONEXION_SABOTAJE,
	ENVIAR_BITACORA,
	FSCK
} comandos_i_mongo_store;

struct semaphore{
	int contador;
	struct t_queue* cola;
}semaphore;

typedef enum {
	GENERAR_OXIGENO,
	CONSUMIR_OXIGENO,
	GENERAR_COMIDA,
	CONSUMIR_COMIDA,
	GENERAR_BASURA,
	DESCARTAR_BASURA
} tareas;


//SOCKETS
int crearSocket();                             //Crea un socket y lo devuelve
int conectarAServidor(char*, char*);           //Recibe el puerto y el ip, crea el socket y lo conecta al servidor que tiene esa ip y ese puerto
struct sockaddr_in setDireccion(char*, char*); //Recibe el puerto y el ip y le setea la direccion al socket
void iniciarEscucha(int, struct sockaddr_in);  //Recibe un socket una direccion y le hace bind y listen
int aceptarConexion(int , struct sockaddr_in); //Recibe un socket y una direccion y acepta la conexion de un cliente
void conectar(int, struct sockaddr_in);        //recibe un socket una direccion y lo conecta al socket con el servidor
int recibir_operacion(int);

//GENERALES
char* mensajeCompleto(char* ,char* ); //Recibe dos cadenas y las concatena
void comprobarErrores(int, char*);    //Recibe un numero y una cadena para mostrar si ese numero es < 0
t_list* strsplit(char*);              //Separara a una cadena por sus ' ' y crea una lista de tokens
void imprimirFecha(FILE*);                 // Imprime el dia y la hora actual
int getValorSemaforo(sem_t);

//SERIALIZACION
void* serializar_paquete(t_paquete*, int);
void eliminar_paquete(t_paquete*);
void agregar_a_serializacion(void*, int*, void*, int); 	//Recibe un buffer, un desplazamiento, un dato y un tamanio y lo serializa en el (buffer + desplazamiento), tambien incrementa el desplazamiento con el tamanio de lo serializado
void* deserializar(int, int, char*);                   	//Recibe un socket, un tamanio y una cadena para mostrar si hay un error en la deserializacion de algun dato
int tamanioTarea(struct Tarea*);						//Calcula el tamanio de una Tarea*
int tamanioListaTareas(t_list*);                       	//Calcula el tamanio de una lista de struct Tarea*

//SEMAFOROS
void semWait(struct semaphore s);
void semSignal(struct semaphore s);


#endif /* FUNCIONESAUXILIARES_H_ */
