/*
 * miRamUtils.h
 *
 *  Created on: 30 may. 2021
 *      Author: utnso
 */

#ifndef SRC_MIRAMUTILS_H_
#define SRC_MIRAMUTILS_H_

#include "funcionesAuxiliares.h"
#include <nivel-gui/nivel-gui.h>
#include <nivel-gui/tad_nivel.h>

#define ASSERT_CREATE(nivel, id, err)                                                   \
    if(err) {                                                                           \
        nivel_destruir(nivel);                                                          \
        nivel_gui_terminar();                                                           \
        printf("Error al crear '%c': %s\n", id, nivel_gui_string_error(err));   \
    }

// VARIABLES COMPARTIDAS
typedef struct relacionID{
	int tid;
	char idMapa;
} relacionID;

t_list* relacionesID;
NIVEL* nivel;
char idMapa     = 65;
int mostrarMapa = 1;

pthread_mutex_t mutexMapa    				= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexRelacionesID			= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaSegmentos			= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexTripulante				= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexGenerarEstructuras		= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSwap					= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaFrames			= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaPaginasSwap 		= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaTablasPatotas 	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexActualizarTripulante	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexTareas 				= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexExpulsar 				= PTHREAD_MUTEX_INITIALIZER;


int multihilo = 1;

int tamanioMemoria;
char* esquemaMemoria;
char* criterioSeleccion;
void* baseMemoria;
int tamanioPagina;
char* algoritmoReemplazo;
char* pathSwap;
int tamanioSwap;
int punteroClock = 0;

int cantidadDeTripulantes = 0;

// ESTRUCTURAS

typedef struct PCB{
	uint32_t PID;
	uint32_t direccPrimerTarea;
} PCB;

typedef struct TCB{
	uint32_t TID;
	char Estado;
	uint32_t PosicionX;
	uint32_t PosicionY;
	uint32_t ProximaInstruccion;
	uint32_t DireccionPCB;
} TCB;


//SEGMENTACIOn
typedef struct segmento{
	char tipoDeDato;
	int idSegmento;
	int inicio;
	int tamanio;
} segmento;

typedef struct tablaSegmentosPatota{
	int pid;
	struct segmento* tareas;
	struct segmento* pcb;
	t_list* segmentosTCB;
	sem_t semaforoSegmentosTCB;
} tablaSegmentosPatota;

t_list* listaDeSegmentos;

//PAGINACION

typedef struct infoFrame {
	int inicio;
	int tamanio;
	char tipoDeDato;
	int tid;
	int numeroTarea;
} infoFrame;

typedef struct pagina {
	int numeroPagina;
	int numeroFrame;
	t_list* infoFrame;
	sem_t semaforoInfoFrame;
	sem_t refrescarPagina;
	clock_t instante;
	int presencia;
	int clock;
	int bitDeUso;
} pagina;

typedef struct tablaPaginasPatota {
	int pid;
	t_list* paginas;
	sem_t semaforoPaginas;
} tablaPaginasPatota;

t_list* tablasPatotas;

t_list* listaDeFrames;
t_list* listaPaginasSwap;

pthread_mutex_t mutexCantidadTripulantes 	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBaseMemoria         	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexArchivoLogMiRam		= PTHREAD_MUTEX_INITIALIZER;
//static pthread_mutex_t mutexMemoria             = PTHREAD_MUTEX_INITIALIZER;
//static pthread_mutex_t mutexTablaSegmentos      = PTHREAD_MUTEX_INITIALIZER;

//RECIBIR DE DISCORDIADOR
struct inicioDePatota* recibirInicioDePatota(int);
struct inicioDeTripulante* recibirInicioDeTripulante(int);

//PATOTA
int generarEstructurasPatota(struct inicioDePatota*); //Genera el PCB y una listaTareas para y las inserta en memoria
int tamanioEnMemoriaPatota(struct inicioDePatota*);   //Recibe un inicioDePatota y calcula el tamanio total que va a ocupar en memoria
void actualizarPCB(struct PCB* pcb);
//TRIPULANTE
int generarEstructurasTripulante(int, int, int, int); //Genera el TCB y lo inserta en memoria

//MEMORIA
void* reservarMemoriaRam(int);      //Recibe el tamanio de la memoria que va a reservar para la memoria principal y hace un malloc
void liberarTabla();
void inicializarTabla();
void* imprimirTabla(void*);
int hayMasTareas(int, int);
void liberarIdp(struct inicioDePatota*);


//SEGMENTACION
int calcularInicioEnMemoriaSegmentacion(int, int*, int);          //Recibe el tamanio del segmento que se quiere insertar en memoria y calcula el inicio en donde se puede insertar en memoria dependiendo del algoritmo de busqueda
void inicializarTablaSegmentos();                      //Inicializa la variable global tablaSegmentos haciendo malloc y creando la lista
void liberarTablaSegmentos();
int generarEstructurasPatotaSegmentacion(struct inicioDePatota*);
void generarEstructurasTripulanteSegmentacion(int, int, int, int);
struct tablaSegmentosPatota* buscarTablaSegmentos(int); //Recibe un pid y busca una tabla de patotas
struct segmento* buscarSegmentoTCB(int);               //Recibe un pid y un tid y devuelve el segmento que cumpla con ese tid y que pertenezca a la patota con ese pid
struct segmento* buscarSegmentoTareas(int);            //Recibe un pid y devuelve la lista de tareas asociado a esa patota
struct segmento* buscarSegmentoPCB(int);               //Recibe un pid y devuelve el pcb asociado a esa patota
void crearTablaSegmentosPatota(int, struct segmento*,struct segmento*);
void agregarATablaSegmentosPatota(int, int, struct tablaSegmentosPatota*, int);
void imprimirTablaSegmentos(FILE*);                         //Imprime un dump de la memoria
void imprimirSegmento(struct segmento*, int, FILE*);          //Imprime un segmento en particular
t_list* obtenerTripulantesSegmentacion();
t_list* obtenerTripulantesPaginacion();
t_list* obtenerTareas(int);
struct Tarea* deserializarTarea(void*, int*);
void* serializarTareaAux(t_list*, int);
void* serializarTareasAux(t_list*);
struct Tarea* deserializarTareaAux(char*);
t_list* deserializarTareasAux(char*);
void borrarSegmentoEnListaSegmentos(struct segmento*);
void compactar();
void moverSegmento(struct segmento*, int);
void waitSemaforoSegmentosTCB(struct tablaSegmentosPatota*);
void postSemaforoSegmentosTCB(struct tablaSegmentosPatota*);
void avisarATripulantesSegmentacion( struct segmento*);
struct PCB* buscarPCB(int);
struct PCB* buscarPCBEnTablaSegmentos(int);
void actualizarPCBSegmentacion(struct PCB*);

//PAGINACION
struct PCB* buscarPCBEnTablaPaginas(int);
void inicializarTablaPaginas(); //Inicializa la variable global tablaProcesos haciendo malloc y creando la lista
void liberarTablaPaginas();
int calcularInicoEnMemoriaPaginacion(int);
int cantidadDeFramesLibres();
int proximoFrameLibre();
int generarEstructurasPatotaPaginacion(struct inicioDePatota*);
void generarEstructurasTripulantePaginacion(int, int, int, int);
int tamanioDisponible(int);
struct tablaPaginasPatota* buscarTablaPaginas(int);
struct pagina* ultimaPagina(int pid);
int paginaPid(struct pagina*);
struct tablaPaginasPatota* crearTablaPaginasPatota(int, int);
void agregarPagina(int, int);
void agregarInfoAPagina(int, int, int,int*);
void imprimirTablaPaginas(FILE*, t_list*,int);
void imprimirMarcoOcupado(struct pagina*, int, FILE*);
void imprimirMarcoVacio(int, FILE*);
int cantidadPaginasTablaPatota(struct pagina*);
t_list* obtenerTCBsPaginacion(int pid);
void actualizarPCBPaginacion(struct PCB*);
void avisarAPCBPaginacion(int, int);

// MEMORIA VIRTUAL
void imprimirSwap();
void inicializarSwap();
void* eliminarPaginaSwap(int);
void traerAMemoria(struct pagina*);
struct pagina* reemplazarPaginaClock();
struct pagina* reemplazarPaginaLRU();
int reemplazarPaginaPor(struct pagina*);
void agregarPaginaASwap(struct pagina*);
void refrescarPagina(struct pagina*);
int liberarFrame();
void avisarATripulantesPaginacion(int, int);
int tienePCB(struct pagina*);
struct pagina* traerAMemoriaPCB(struct TCB*);
int tamanioDisponibleEnSwap();
void liberarSwap();
void actualizarBloquesBitacora(FILE*, t_list*);

//INSERTAR EN MEMORIA
void* serializarPCB(struct PCB*);
void* serializarTarea(struct Tarea*, int);
void* serializarTareas(t_list*, int);
void* serializarTCB(struct TCB*);
void* insertarAMemoria(void*, int*, int, int, char);
void insertarAMemoriaConPaginacion(int, int, int, int, void*, char, int, int);
struct TCB* deserializarTCB(void*);
struct PCB* deserializarPCB(void*);
void* obtenerDeMemoria(int*, int, int);
struct Tarea* obtenerTarea(int, int);
struct Tarea* obtenerTareaPaginacion(int, int);
struct Tarea* obtenerTareaSegmentacion(int, int);

//LISTAR TRIPULANTES
void enviarTripulantes(int);                    //Envia los tripulantes serializados al discordiador
void* serializarListaTripulantes(t_list*, int); //Serializa a la lista de todos los tripulantes
int tamanioTripulantes(t_list*);                //Calcula el tamanio de una lista de struct tripulante
t_list* obtenerTripulantes();
t_list* obtenerInformacionPaginacion(int, int, int);
struct PCB* pcbDe(struct TCB*);
int seBuscaPid(int, int);


//EXPULSAR TRIPULANTE
int recibirExpulsionDeTripulante(int); //Recibe el tid del tripulante que se quiere expulsar
void expulsarTripulante(int tid);      //Expulsa al tripulante que tenga el tid que recibe, lo borra de memoria y de la tabla de segmentos
int hayMasTripulantes(struct tablaPaginasPatota* );

//ENVIAR PROXIMA TAREA
int recibirPedidoDeProximaTarea(int);
void enviarProximaTarea(int, int);
struct TCB* buscarTCB(int);
struct TCB* buscarTCBEnTablaSegmentos(int);
struct TCB* buscarTCBEnTablaPaginas(int);

//MAPA
void inicializarMapa();
void liberarMapa();
void inicializarTripulante(int, int, int);
void quitarDeMapa(int);
void dibujarTripulante(int, int, struct relacionID*);

//ACTUALIZAR ESTADO
void actualizarEstado(int);

//DEVOLVER POSICIONES
void enviarPosiciones(int);

//ACTUALIZAR POSICIONES
void actualizarPosicion(int);

//ACTUALIZAR TCB
int buscarInicioTCBPaginacion(int);
struct segmento* buscarSegmentoTCB(int);
struct TCB* actualizarTCB(struct TCB*, char, int, int, int);
struct TCB* actualizarTCBPaginacion(struct TCB*, int, char, int, int, int);

// TRANSFORMAR
t_list* transformarTCBsATripulante(t_list* listaTCBs);
int obtenerPIDDeListaConTID(t_list* tripulantes, int tid);
struct pagina* obtenerPaginaPIDDeTID(int tid);
struct PCB* obtenerPCBDeTCB(struct TCB* tcb);
t_list* actualizarListaTCBs(t_list*);

//FUNCIONES HILOS
void logearArchivoMiRam(char*);
void* iniciarPatotaMiRam(void*);
void* listarTripulantesMiRam(void*);
void* expulsar(void*);
void* proximaTarea(void*);
void* cambiarEstado(void*);
void* enviarPosicionTripulante(void*);
void* cambiarPosicion(void*);


#define TAMANIO_PCB 8
#define TAMANIO_TCB 21

#endif /* SRC_MIRAMUTILS_H_ */
