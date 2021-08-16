/*
 * I_Mongo_Store.c
 *
 *  Created on: 25 abr. 2021
 *      Author: utnso
 */


#include "iMongoStore.h"

#define MAXDATASIZE 100 // Maximo numero de bytes que se pueden leer a la vez
#define MYPORT "5006"
#define BACKLOG 10


int main(int argc, char **argv[]) {
	struct sockaddr_in direccPersonal;
	struct sockaddr_in direccDestino;
	int ficheroSocketOriginal;

	config			 	= config_create("src/iMongoStore.config");
	char* puerto  		= strdup(config_get_string_value(config, "PUERTO"));

	inicializarFileSystem(argv[1]);
	int tiempoDeSincronizacion = config_get_int_value(config,"TIEMPO_SINCRONIZACION");

	pthread_t sincronizador;
	pthread_create(&sincronizador,NULL,sincronizarBlocks,&tiempoDeSincronizacion);
	pthread_detach(sincronizador);
	remove("../logIMongoStore.txt");

	llenarPosicionesSabotaje();

	sem_init(&semaforoSabotaje, 0, 0);
	sem_init(&conexionDiscordiadorLiberada, 0, 0);
	signal(SIGUSR1, sabotaje);

	ficheroSocketOriginal = crearSocket();
	direccPersonal        = setDireccion(puerto, "0.0.0.0");
	free(puerto);
	iniciarEscucha(ficheroSocketOriginal, direccPersonal);
	flagExit = 0;
	while(!flagExit){
		int ficheroSocket = aceptarConexion(ficheroSocketOriginal, direccDestino);

		char* strSocket = string_itoa(ficheroSocket);
		char* log 		= NULL;

		int operacion = recibir_operacion(ficheroSocket);
		pthread_t hilo;
		switch(operacion) {
			pthread_t conexionSabotaje;
			case GENERAR_RECURSOS:
					pthread_create(&hilo,NULL, generarRecursos,string_duplicate(strSocket));
					pthread_detach(hilo);
				break;
			case DESCARTAR_RECURSOS:
					pthread_create(&hilo,NULL, descartarRecursos,string_duplicate(strSocket));
					pthread_detach(hilo);
				break;
			case INICIAR_BITACORA:
					pthread_create(&hilo,NULL, iniciarBitacora,string_duplicate(strSocket));
					pthread_detach(hilo);
				break;
			case MOVIMIENTO:
					pthread_create(&hilo,NULL, logearMovimientoHilo,string_duplicate(strSocket));
					pthread_detach(hilo);
				break;
			case COMIENZO_DE_TAREA:
					pthread_create(&hilo,NULL, logearComienzoDeTareaHilo,string_duplicate(strSocket));
					pthread_detach(hilo);
				break;
			case FINALIZACION_DE_TAREA:
					pthread_create(&hilo,NULL, logearFinalizacionDeTareaHilo,string_duplicate(strSocket));
					pthread_detach(hilo);
				break;
			case CORRIDA_EN_PANICO:
					pthread_create(&hilo,NULL, logearCorridaEnPanicoHilo,string_duplicate(strSocket));
					pthread_detach(hilo);
				break;
			case FSCK:
					pthread_create(&hilo,NULL, invocarFsck,string_duplicate(strSocket));
					pthread_detach(hilo);
				break;
			case SABOTAJE_RESUELTO:
					pthread_create(&hilo,NULL, logearSabotajeResueltoHilo,string_duplicate(strSocket));
					pthread_detach(hilo);
				break;
			case ENVIAR_BITACORA:
					pthread_create(&hilo,NULL, enviarBitacora,string_duplicate(strSocket));
					pthread_detach(hilo);
				break;
			case CONEXION_SABOTAJE:
				pthread_create(&conexionSabotaje, NULL, enviarAvisosDeSabotaje, string_duplicate(strSocket));
				pthread_detach(conexionSabotaje);
				break;
			case EXIT:
				flagExit = 1;
				esperarHilosMongo();
				close(ficheroSocket);
				log = string_new();
				string_append(&log, "Se cierra correctamente el modulo iMongoStore.");
				logearArchivoIMongo(log);
				break;
			case -1:
				log = string_new();
				string_append(&log, "Error default en I MongoStore.");
				logearArchivoIMongo(log);
				close(ficheroSocket);
				break;
		}
		free(strSocket);
	}

	sleep(1);
	config_destroy(config);
	while(listaMutexBitacoras->elements_count != 0){
		struct relacionTIDMutex* mutexTid = list_remove(listaMutexBitacoras,0);
		sem_destroy(&mutexTid->mutex);
		free(mutexTid);
	}
	list_destroy(listaMutexBitacoras);

	while(posicionesSabotaje->elements_count != 0){
		char* posicion = list_remove(posicionesSabotaje,0);
		free(posicion);
	}
	list_destroy(posicionesSabotaje);
	sem_destroy(&semaforoSabotaje);
	sem_destroy(&conexionDiscordiadorLiberada);

	return EXIT_SUCCESS;
}
