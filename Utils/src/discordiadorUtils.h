/*
 * discordiadorUtils.h
 *
 *  Created on: 30 may. 2021
 *      Author: utnso
 */

#ifndef SRC_DISCORDIADORUTILS_H_
#define SRC_DISCORDIADORUTILS_H_

#include "funcionesAuxiliares.h"


typedef struct tripulantePlanificacion{
	int tid;
	char estado;
	int quantum;
	int tiempoDeTarea;
	struct Tarea* proximaTarea;
	sem_t semaforo;
	int solicitoES;
	struct TareaES* informacionES;
} tripulantePlanificacion;

typedef struct TareaES{
	char* recurso;
	int tipoES;
}TareaES;

// VARIABLES GLOBALES
char* algoritmoPlanificacion = NULL;
int gradoMultitarea;
int quantum;
int contadorPid 			= 1;
int contadorTid 			= 1;
char* ip_ram				= NULL;
char* puerto_ram			= NULL;
char* ip_mongo				= NULL;
char* puerto_mongo			= NULL;
int modoSabotaje 			= 0;
int planificacionPausada 	= 1;
int retardoCicloCPU;
int duracionSabotaje;
int planificadorBloqueado	= 0;
int resolviSabotaje			= 0;
int flagExitDiscordiador	= 0;

sem_t sabotajeResuelto;
sem_t procesadoresLibres;
sem_t listoParaResolverSabotaje;
sem_t tareaCambiada;
sem_t semaforoBloqueados;
sem_t semaforoBloqueado;
sem_t semaforoPlanificador;
sem_t estoyReady;
sem_t conexionMongoLiberada;
sem_t planificadorLiberado;

t_list* listaReady			= NULL;
t_list* listaExec			= NULL;
t_list* listaTripulantes	= NULL;
t_queue* colaSabotaje		= NULL;

pthread_mutex_t mutexListaReady 			= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaExec				= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPlanificacion			= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaTripulantes		= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexArchivoLogDiscordiador	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexContadorPid 			= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexContadorTid 			= PTHREAD_MUTEX_INITIALIZER;

// DISCORDIADOR
int comandoDiscordiador(char*); //Convierte a los comandos de la consola del discordiador en un ENUM
void logearArchivoDiscordiador(char*);

//FUNCIONES HILOS
void* iniciarPatota(void*);
void* listarTripulantes(void*);
void* reanudarPlanificacion(void*);
void* pausarPlanificacion(void*);
void* obtenerBitacora(void*);
void* expulsarTripulanteHilo(void*);

// LISTAR TRIPULANTES
void solicitarTripulantes(int);
t_list* recibirListaTripulantes(int);
void imprimirTripulantes(t_list*);
char* calcularEstado(char);
struct tripulantePlanificacion* obtenerTripulante(int);

// INICIO DE PATOTA
t_list* llenarTareas(char*);                            //Recibe el nombre de un archivo con tareas y las paresea a una lista de struct Tarea*
int tamanioInicioDePatota(struct inicioDePatota*);      //Calcula el tamanio de una estructura inicioDePatota
void* serializarInicioDePatota(struct inicioDePatota*, int);        //Serializa la estrucutra inicioDePatota dentro de un paquete para ser enviada a miRamHQ
int enviarInicioDePatota(int, struct inicioDePatota*); //Envia el los datos necesarios para que miRamHQ genere las estructuras administrativas de las patotas
void errorIniciarPatota();
void waitSemaforoTripulante(struct tripulantePlanificacion*);
void postSemaforoTripulante(struct tripulantePlanificacion*);
int valorSemaforoTripulante(struct tripulantePlanificacion*);
struct tidSemaforo* buscarTidSemaforo(struct tripulantePlanificacion*);

// INICIO DE TRIPULANTE
int tamanioInicioDeTripulante(struct inicioDeTripulante*);      //Calcula el tamanio de una estructura inicioDeTripulante
void* serializarInicioDeTripulante(t_paquete*, int);            //Serializa la estrucutra inicioDeTripulante dentro de un paquete para ser enviada a miRamHQ
void enviarInicioDeTripulante(int, struct inicioDeTripulante*); //Envia el los datos necesarios para que miRamHQ genere las estructuras administrativas de un tripulante
void enviarInicioDeBitacora(int , int);
void* iniciarTripulante(void*);
void informarAMongo(int, int);

// INICIO DE PLANIFICACION
void inicializarColas();
void* solicitarCambioDeEstado(void*);
void pasarDeNewAReady(struct tripulantePlanificacion*);
void pasarDeReadyAExec();

//REALIZAR TAREA
void solicitarProximaTarea(int, int);
Tarea* recibirProximaTarea(int);
void ejecutar(struct tripulantePlanificacion*);		// Manda a un tripulante a realizar su primera tarea, la cual se pedira al modulo miRam.
void dejarDeEjecutar(struct tripulantePlanificacion*, t_list*, pthread_mutex_t);
void quitarDeLista(int tid, t_list*, char*);
void moverseAPosicion(struct tripulantePlanificacion*, int, int, int);
void solicitarPosiciones(int, int);
void solicitarCambioPosicion(int, int, int, int, int);
void* serializarCambioDePosicion( int , int , int , int);
void* serializarMovimientoMongo(int, int, int, int, int, int);
void moverse(struct tripulantePlanificacion*, int, int, int, void (int, int, int*), void (int, int, int*), int);
_Bool tidAscendente(void*, void*);
void realizarTarea(struct tripulantePlanificacion*);
void realizarTareaES(struct tripulantePlanificacion*, char*, int);
void solicitarES(int, char*, int);
void solicitoEntradaSalida(struct tripulantePlanificacion*);
int verificarPlanificacionPausada(struct tripulantePlanificacion*);
void desbloquearse(struct tripulantePlanificacion*);
void pedirProximaTarea(struct tripulantePlanificacion*);
void* planificador(void*);
void informarIoFTarea(int, struct tripulantePlanificacion*, int);


//EXPULSAR_TRIPULANTES
void solicitarExpulsionTripulante(int);
void quitarTripulanteDeLista(int);

//OBTENER BITACORA
void recibirBitacora(int, int);

//SABOTAJE
void* conectarseAMongo(void*);
void* iniciarProtocoloSabotaje(void*);
_Bool tidAscendente(void*, void*);
void resolverSabotaje(struct tripulantePlanificacion*);

//EXIT
void terminarDiscordiador();
void terminarIMongo();
void terminarMiRam();
void esperarHilosDiscordiador();
void liberarTokens(t_list*, char**);

void logearArchivoDiscordiador(char*);
void errorComandoDiscordiador();

#endif /* SRC_DISCORDIADORUTILS_H_ */
