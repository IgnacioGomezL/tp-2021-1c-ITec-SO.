/*
 * iMongoStoreUtils.h
 *
 *  Created on: 1 jul. 2021
 *      Author: utnso
 */

#ifndef SRC_DISCORDIADORUTILS_H_
#define SRC_DISCORDIADORUTILS_H_

#define handle_error(msg) \
   do { perror(msg); exit(EXIT_FAILURE); } while (0)

#include "funcionesAuxiliares.h"
#include <sys/mman.h>
#include <dirent.h>

typedef struct relacionTIDMutex{
	int tid;
	sem_t mutex;
}relacionTIDMutex;


t_config* config;
char* puntoMontaje;
void* mapeadoBlocks;
void* copiaBlocks;
t_list* posicionesSabotaje;
t_list* listaMutexBitacoras;
int contadorPosicion	= 0;
int Mongomultihilo		= 1;
int flagExit			= 0;

pthread_mutex_t mutexCopiaBloques		= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBitmap				= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexMD5				= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexFiles				= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaMutex			= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexArchivoLogIMongo	= PTHREAD_MUTEX_INITIALIZER;
sem_t semaforoSabotaje;
sem_t conexionDiscordiadorLiberada;
sem_t sincronizacion;

//GENERAL
void llenarPosicionesSabotaje();
char* rutaPuntoMontaje();
int ultimoBloque(FILE*);
int desplazamientoLinea(FILE*, int);
void liberarListaBloques(t_list*);
int actualizarLineaArchivo(FILE*, int, char*, int);
int obtenerIntLista(t_list*, int, int);
//SUPERBLOCK
void inicializarFileSystem(char*);
int proximoBloqueLibre();
char* obtenerBitmap();
char* obtenerPunteroABit();
int obtenerCantidadBloquesSuperbloque();
int obtenerBlockSize();
void cleanBit(int index);
void actualizarBitmap(int, char);
void imprimirBitmap();
FILE* abrirSuperBloque();
//BLOCKS
void imprimirBlocks();
void escribirCaracteres(FILE*, char*, int, int, int, char);
void escribirBlocks( char , int , int);
//FILES
char* ruteFiles(int);
t_list* obtenerBloquesFile(FILE*);
int agregarCaracteresA(char*, int, int);
int borrarCaracteresA(char*, int);
void borrarBloquesFile(FILE*, char*);
void eliminarBloquesFile(FILE*, char*, int);
int primerBloqueAEscribir(FILE*);
char obtenerCaracterLlenadoFile(FILE*);
int obtenerSizeFile(FILE*);
int obtenerCantidadDeBloquesFile(FILE*);
int* obtenerBloques(FILE*);
void* obtenerDe(FILE* , int , int , int);
void* obtenerDeAux(FILE* , int , int);
FILE* crearArchivoFile(char*, char);
char caracterArchivo(char*);
int agregarBloqueAFile(FILE*, char*);
void actualizarFileSize(FILE*, char*, int, int);
char* generarMD5(FILE*);
int siguienteBloqueFile (FILE*, int);
char* generarMetadataBloques(t_list*);
char* obtenerMD5(FILE*);
void actualizarMD5(FILE*, char*);
void actualizarBloquesFile(FILE*, t_list*, char*);
//BITACORAS
char* rutaBitacoras(int);
char* nombreBitacora(int);
char* rutaBitacora(int);
void crearArchivoBitacora(int);
t_list* obtenerBloquesBitacora(FILE*);
int agregarBloqueABitacora(FILE*);
void logearMovimiento(int, int, int, int, int);
void logearComienzoDeTarea(int , char*);
void logearFinalizacionDeTarea(int , char*);
void logearCorridaEnPanico(int);
void logearSabotajeResuelto(int);
char* obtenerRestoArchivo(FILE* , int , int );
t_list* generarListaBloques(char*);
void logear(int, char*);
struct relacionTIDMutex* obtenerMutexTid(int);
void actualizarBloquesBitacora(FILE*, t_list*);
int existeTripulante(int);
//BITMAP
void cleanBitmap(t_bitarray*);

//CONEXION
int recibirTID(int);
void recibirMovimientoYLoguear(int , int);
char* deserializarTarea(int socket);

// SINCRONIZACION
void* sincronizarBlocks(void*);

//SABOTAJE
void* enviarAvisosDeSabotaje(void*);
void sabotaje(int);

//FSCK
int fsckSuperBloque();
int fsckFile();
void fsck();
t_list* leerBloquesArchivos();
t_list* obtenerNombresArchivos(char*);
void actualizarCantidadBloquesSuperBloque(int);
void reemplazarBitmap(char*);
void obtenerBloquesDeArchivos(t_list*, char*, int);
t_list* obtenerBloquesFB(FILE* , int);
int verificarSabotajeFile(char*);
void restaurarArchivo(FILE* , int , char* , t_list*);
int verificarPertenencia(int, char);
void vaciarBloques(t_list* , int);
int tieneCaracterLlenado(int  , char);
int lista_find(t_list*, int);
void vaciarBloqueCambiado(t_list*, char);
void vaciarBloque(int);
void truncarArchivo(FILE*, char*);
int sePasaDeTamanio(int);
int estaVacio(int);
int ultimoBloqueMovido(int, int, int);

//FUNCIONES HILOS
void* generarRecursos(void*);
void* descartarRecursos(void*);
void* iniciarBitacora(void*);
void* logearMovimientoHilo(void*);
void* logearComienzoDeTareaHilo(void*);
void* logearFinalizacionDeTareaHilo(void*);
void* logearCorridaEnPanicoHilo(void*);
void* logearSabotajeResueltoHilo(void*);
void* enviarBitacora(void*);
void* invocarFsck(void*);
void logearArchivoIMongo(char*);
void esperarHilosMongo();

#endif /* SRC_IMONGOSTOREUTILS_H_ */
