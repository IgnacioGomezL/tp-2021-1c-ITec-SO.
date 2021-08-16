/*
 * miRamHQ.c
 *
 *  Created on: 26 abr. 2021
 *      Author: utnso
 */

#define BACKLOG 10
#define STDIN 0

#include "miRamHQ.h"

int main(void) {
	int ficheroSocketOriginal;
	struct sockaddr_in direccPersonal;
	struct sockaddr_in direccDestino;
	t_config* config 	= config_create("src/miRamHQ.config");
	char* puerto  		= strdup(config_get_string_value(config, "PUERTO"));

	tamanioMemoria    	= config_get_int_value(config,"TAMANIO_MEMORIA");
	esquemaMemoria    	= config_get_string_value(config,"ESQUEMA_MEMORIA");
	criterioSeleccion 	= config_get_string_value(config,"CRITERIO_SELECCION");
	tamanioPagina     	= config_get_int_value(config,"TAMANIO_PAGINA");
	algoritmoReemplazo 	= config_get_string_value(config,"ALGORITMO_REEMPLAZO");
	pathSwap		 	= config_get_string_value(config,"PATH_SWAP");
	tamanioSwap			= config_get_int_value(config,"TAMANIO_SWAP");
	baseMemoria       	= reservarMemoriaRam(tamanioMemoria);
	inicializarTabla();
	remove("../logMiRamHQ.txt");

	if(mostrarMapa) inicializarMapa();
	inicializarSwap();

	void singalHandler(int n){
		pthread_t th;
		switch (n) {
			case SIGUSR1:
				compactar();
			break;
			case SIGUSR2:
				pthread_create(&th, NULL, imprimirTabla, NULL);
				pthread_detach(th);
			break;
		}
	}
	signal(SIGUSR1, singalHandler);
	signal(SIGUSR2, singalHandler);

	ficheroSocketOriginal = crearSocket();
	direccPersonal        = setDireccion(puerto, "0.0.0.0");
	free(puerto);
	iniciarEscucha(ficheroSocketOriginal, direccPersonal);

	int exit = 0;
	while(!exit){
		int ficheroSocketNuevo = aceptarConexion(ficheroSocketOriginal, direccDestino);
		char* strSocket = string_itoa(ficheroSocketNuevo);
		int operacion = recibir_operacion(ficheroSocketNuevo);
		pthread_t th;
		char* log;
		switch(operacion) {
			case INICIAR_PATOTA:
					pthread_create(&th, NULL, iniciarPatotaMiRam, string_duplicate(strSocket));
					pthread_detach(th);
				break;
			case LISTAR_TRIPULANTES:
					pthread_create(&th, NULL, listarTripulantesMiRam, string_duplicate(strSocket));
					pthread_detach(th);
				break;
			case EXPULSAR_TRIPULANTE:
					pthread_create(&th, NULL, expulsar, string_duplicate(strSocket));
					pthread_detach(th);
				break;
			case PROXIMA_TAREA:
					pthread_create(&th, NULL, proximaTarea, string_duplicate(strSocket));
					pthread_detach(th);
				break;
			case CAMBIO_DE_ESTADO:
					pthread_create(&th, NULL, cambiarEstado, string_duplicate(strSocket));
					pthread_detach(th);
				break;
			case POSICION_TRIPULANTE:
					pthread_create(&th, NULL, enviarPosicionTripulante, string_duplicate(strSocket));
					pthread_detach(th);
				break;
			case CAMBIO_DE_POSICION:
					pthread_create(&th, NULL, cambiarPosicion, string_duplicate(strSocket));
					pthread_detach(th);
				break;
			case EXIT:
				close(ficheroSocketNuevo);
				exit = 1;
				log = string_new();
				string_append(&log, "Se cierra correctamente el modulo MiRamHQ.");
				logearArchivoMiRam(log);
				break;
			case -1:
				log = string_new();
				string_append(&log, "El cliente se desconecto.");
				logearArchivoMiRam(log);
				close(ficheroSocketNuevo);
				break;
			default:
				log = string_new();
				string_append(&log, "Error default en Mi Ram HQ.");
				logearArchivoMiRam(log);
		}
		free(strSocket);
	}

	liberarTabla();
	liberarSwap();
	if(mostrarMapa) liberarMapa();
	config_destroy(config);
	free(baseMemoria);

	return EXIT_SUCCESS;
}
