#include "./discordiador.h"

int main(int argc, char **argv[]) {
	t_config* config   		= config_create("src/discoriador.config");

	algoritmoPlanificacion 	= config_get_string_value(config, 	"ALGORITMO");
	gradoMultitarea 		= config_get_int_value(config, 		"GRADO_MULTITAREA");
	quantum 				= config_get_int_value(config, 		"QUANTUM");
	ip_mongo	  	   		= config_get_string_value(config, 	"IP_I_MONGO_STORE");
	puerto_mongo	 		= config_get_string_value(config, 	"PUERTO_I_MONGO_STORE");
	ip_ram       			= config_get_string_value(config, 	"IP_MI_RAM_HQ");
	puerto_ram   			= config_get_string_value(config, 	"PUERTO_MI_RAM_HQ");
	retardoCicloCPU			= config_get_int_value(config, 		"RETARDO_CICLO_CPU");
	duracionSabotaje	 	= config_get_int_value(config, 	"DURACION_SABOTAJE");

	pthread_t hiloSabotaje;
	pthread_create(&hiloSabotaje, NULL, conectarseAMongo, NULL);
	pthread_detach(hiloSabotaje);
	inicializarColas();
	sem_init(&conexionMongoLiberada,0,0);
	sem_init(&planificadorLiberado,0,0);

	remove("../logDiscordiador.txt");

	char* leido 	= NULL;
	while(!flagExitDiscordiador){
		leido 				= readline("--->");
		t_list* tokens 		= list_create();
		char** arrTokens 	= string_split(leido, " ");
		for(int i = 0; arrTokens[i] != NULL; i++){
			list_add(tokens, arrTokens[i]);
		}
		int comando    = comandoDiscordiador(list_remove(tokens,0));
		int tid;
		char* strTid;
		char* token;
		char* log = NULL;
		t_list* tokensAux = NULL;
		pthread_t threadPlanificacion;
		switch(comando){
			case INICIAR_PATOTA:
				tokensAux = list_create();
				while(tokens->elements_count != 0){
					char* token = malloc(strlen(list_get(tokens, 0)) + 1);
					strcpy(token, list_remove(tokens, 0));
					list_add(tokensAux, token);
				}
				pthread_create(&threadPlanificacion, NULL, iniciarPatota, tokensAux);
				pthread_detach(threadPlanificacion);
				break;
			case LISTAR_TRIPULANTES:
				pthread_create(&threadPlanificacion, NULL, listarTripulantes, NULL);
				pthread_detach(threadPlanificacion);
				break;
			case EXPULSAR_TRIPULANTE:
				if(tokens->elements_count != 0){
					token = malloc(strlen(list_get(tokens, 0)) + 1);
					strcpy(token, list_remove(tokens, 0));
				} else {
					errorComandoDiscordiador();
					break;
				}
				pthread_create(&threadPlanificacion, NULL, expulsarTripulanteHilo, token);
				pthread_detach(threadPlanificacion);
				break;
			case INICIAR_PLANIFICACION:
				pthread_create(&threadPlanificacion, NULL, reanudarPlanificacion, NULL);
				pthread_detach(threadPlanificacion);
				break;
			case PAUSAR_PLANIFICACION:
				pthread_create(&threadPlanificacion, NULL, pausarPlanificacion, NULL);
				pthread_detach(threadPlanificacion);
				break;
			case OBTENER_BITACORA:
				if(tokens->elements_count != 0){
					token = malloc(strlen(list_get(tokens, 0)) + 1);
					strcpy(token, list_remove(tokens, 0));
				} else {
					errorComandoDiscordiador();
					break;
				}
				pthread_create(&threadPlanificacion, NULL, obtenerBitacora, token);
				pthread_detach(threadPlanificacion);
				break;
			case EXIT:
				flagExitDiscordiador = 1;
				terminarDiscordiador();
				terminarIMongo();
				terminarMiRam();
				esperarHilosDiscordiador();
				log = string_new();
				string_append(&log, "Se cierra correctamente el modulo Discordiador.");
				logearArchivoDiscordiador(log);
				break;
			case -1:
				errorComandoDiscordiador();
				break;
			default:
				log = string_new();
				string_append(&log, "Error default en Discordiador.");
				logearArchivoDiscordiador(log);
		}
		liberarTokens(tokens, arrTokens);
		free(leido);
	}
	sleep(1);
	config_destroy(config);

	sem_destroy(&conexionMongoLiberada);
	sem_destroy(&planificadorLiberado);
	sem_destroy(&sabotajeResuelto);
	sem_destroy(&procesadoresLibres);
	sem_destroy(&listoParaResolverSabotaje);
	sem_destroy(&tareaCambiada);
	sem_destroy(&semaforoBloqueados);
	sem_destroy(&semaforoBloqueado);
	sem_destroy(&semaforoPlanificador);
	sem_destroy(&estoyReady);

	list_destroy(listaReady);
	list_destroy(listaExec);
	list_destroy(listaTripulantes);
	queue_destroy(colaSabotaje);

	pthread_mutex_destroy(&mutexListaReady);
	pthread_mutex_destroy(&mutexListaExec);
	pthread_mutex_destroy(&mutexPlanificacion);
	pthread_mutex_destroy(&mutexListaTripulantes);
	pthread_mutex_destroy(&mutexArchivoLogDiscordiador);
	pthread_mutex_destroy(&mutexContadorPid);
	pthread_mutex_destroy(&mutexContadorTid);

	return EXIT_SUCCESS;
}
