/*
 * discordiadorUtils.c
 *
 *  Created on: 30 may. 2021
 *      Author: utnso
 */


#include "discordiadorUtils.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * DISCORDIADOR
 */

int comandoDiscordiador(char* comando){
	if(strcmp(comando, "INICIAR_PATOTA") == 0){
		return INICIAR_PATOTA;
	}
	if(strcmp(comando, "LISTAR_TRIPULANTES") == 0){
		return LISTAR_TRIPULANTES;
	}
	if(strcmp(comando, "EXPULSAR_TRIPULANTE") == 0){
		return EXPULSAR_TRIPULANTE;
	}
	if(strcmp(comando, "INICIAR_PLANIFICACION") == 0){
		return INICIAR_PLANIFICACION;
	}
	if(strcmp(comando, "PAUSAR_PLANIFICACION") == 0){
		return PAUSAR_PLANIFICACION;
	}
	if(strcmp(comando, "OBTENER_BITACORA") == 0){
		return OBTENER_BITACORA;
	}
	if(strcmp(comando, "SABOTAJE") == 0){
		return SABOTAJE;
	}
	if(strcmp(comando, "EXIT") == 0){
		return EXIT;
	}
	return -1;
}

void liberarTokens(t_list* tokens, char** arrTokens){
	while(tokens->elements_count != 0){
		list_remove(tokens, 0);
	}
	if(arrTokens != NULL){
		for(int i = 0; arrTokens[i] != NULL; i++){
			free(arrTokens[i]);
			arrTokens[i] = NULL;
		}
	}
	list_destroy(tokens);
	free(arrTokens);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * LISTAR TRIPULANTEs
 */

void solicitarTripulantes(int socket){
	void * magic        = malloc(sizeof(int));
	int desplazamiento  = 0;
	int codigoOperacion = LISTAR_TRIPULANTES;
	//CODIGO DE OPERACION
	agregar_a_serializacion(magic, &desplazamiento, &(codigoOperacion), sizeof(int));
	send(socket, magic, sizeof(int), 0);
	free(magic);
}

t_list* recibirListaTripulantes(int socket){
	t_list* tripulantes  = list_create();
	int* cantTripulantes = (int*)deserializar(socket, sizeof(int), "size Buffer");
	for(int i = 0; i < *cantTripulantes; i++){
		int* tid     = (int*) deserializar(socket, sizeof(int), "tid Tripulante");
		int* pid     = (int*) deserializar(socket, sizeof(int), "pid Tripulante");
		char* estado = (char*)deserializar(socket, sizeof(char), "estado Tripulante");

		struct tripulante* tripulante = malloc(sizeof(struct tripulante));
		tripulante->tid    = *tid;
		tripulante->pid    = *pid;
		tripulante->estado = *estado;
		free(tid);
		free(pid);
		free(estado);

		list_add(tripulantes, tripulante);
	}
	free(cantTripulantes);
	close(socket);
	return tripulantes;
}

void imprimirTripulantes(t_list* tripulantes){
	printf("--------------------------------------------------------------------------------\n");
	printf("Estado de la Nave: ");imprimirFecha(NULL);
	while(tripulantes->elements_count != 0){
		struct tripulante* tripulante = list_remove(tripulantes, 0);
		printf("Tripulante: %d\tPatota: %d\tStatus: %s\n", tripulante->tid, tripulante->pid, calcularEstado(tripulante->estado));
		free(tripulante);
	}
	list_destroy(tripulantes);
	printf("--------------------------------------------------------------------------------\n");
}

char* calcularEstado(char estado){
	switch(estado){
	case 'N':
		return "NEW";
		break;
	case 'R':
		return "READY";
		break;
	case 'E':
		return "EXEC";
		break;
	case 'B':
		return "BLOCK I/0";
		break;
	case 'X':
		return "EXIT";
		break;
	default:
		return "NULL";
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * INICIO DE PATOTA
 */


void errorComandoDiscordiador(){
	char* log = string_new();
	string_append(&log, "Error en el comando Discordiador");
	logearArchivoDiscordiador(log);
}

t_list* llenarTareas(char* nombreArchivoTareas){
	char* tarea;
	t_list* listaTareas 	= list_create();
//	char* rutaArchivoTareas = malloc(strlen("/home/utnso/tp-2021-1c-ITec-SO/Discordiador/tareas/") + strlen(nombreArchivoTareas) + 1);
//	strcpy(rutaArchivoTareas,"/home/utnso/tp-2021-1c-ITec-SO/Discordiador/tareas/");
//	strcat(rutaArchivoTareas,nombreArchivoTareas);
	FILE* archivoDeTareas 	= fopen(nombreArchivoTareas, "r");
//	free(rutaArchivoTareas);
	if(archivoDeTareas == NULL) return NULL;
	int seek 				= 0;
	char caracter			= 0;

	while(caracter != EOF){
		int contador 	= 1;
		caracter 		= fgetc(archivoDeTareas);
		while(caracter != '\n' && caracter != EOF){
			contador++;
			caracter 	= fgetc(archivoDeTareas);
		}
		tarea = malloc(contador);
		fseek(archivoDeTareas, seek, SEEK_SET);
		fgets(tarea, contador, archivoDeTareas);
		list_add(listaTareas, tarea);
		fseek(archivoDeTareas,1,SEEK_CUR);
		seek += contador;
	}
	fclose(archivoDeTareas);
	return listaTareas;
}


int tamanioInicioDePatota(struct inicioDePatota* idp){
	int tamanioTareas = tamanioListaTareas(idp->listaTareas);

	int tamanioPosiciones = 0;
	for(int i = 0; i < idp->posiciones->elements_count; i++){
		char* posicion = list_get(idp->posiciones, i);
		tamanioPosiciones += strlen(posicion) + 1 + sizeof(int);
	}
	return tamanioTareas + sizeof(int) * 3 + tamanioPosiciones;
}

void* serializarInicioDePatota(struct inicioDePatota* idp, int bytes) {
	void * magic 		= malloc(bytes);
	int desplazamiento 	= 0;

	t_list* tareas = idp->listaTareas;

	//PID
	agregar_a_serializacion(magic, &desplazamiento, &(idp->pid), sizeof(int));

	agregar_a_serializacion(magic, &desplazamiento, &(idp->cantTripulantes), sizeof(int));

	agregar_a_serializacion(magic, &desplazamiento, &(tareas->elements_count), sizeof(int));

 	for(int i = 0; i < tareas->elements_count; i++){
		char* tarea = list_get(tareas, i);
		int tamanioTarea = strlen(tarea);
		agregar_a_serializacion(magic, &desplazamiento, &tamanioTarea, sizeof(int));
		agregar_a_serializacion(magic, &desplazamiento, tarea, strlen(tarea) + 1);
		free(tarea);
	}

	t_list* posiciones = idp->posiciones;
 	agregar_a_serializacion(magic, &desplazamiento, &(posiciones->elements_count), sizeof(int));

 	for(int i = 0; i < posiciones->elements_count; i++){
		char *posicion 			= list_get(posiciones, i);
		int longitudPosicion 	= strlen(posicion);
		agregar_a_serializacion(magic, &desplazamiento, &longitudPosicion, sizeof(int));
		agregar_a_serializacion(magic, &desplazamiento, posicion, strlen(posicion) + 1);
	}

 	agregar_a_serializacion(magic, &desplazamiento, &(idp->primerTid), sizeof(int));

	return magic;
}

int enviarInicioDePatota(int socket_cliente, struct inicioDePatota* idp) {
	t_paquete* paquete 	= malloc(sizeof(t_paquete));
	int tamanioIdp 		= tamanioInicioDePatota(idp);

	paquete->codigo_operacion 	= INICIAR_PATOTA;
	paquete->buffer           	= malloc(sizeof(t_buffer));
	paquete->buffer->size     	= tamanioIdp + (sizeof(int) * 2);
	paquete->buffer->stream   	= malloc(paquete->buffer->size);
	void* serializarIdp 		= serializarInicioDePatota(idp, paquete->buffer->size);
	memcpy(paquete->buffer->stream, serializarIdp, paquete->buffer->size);
	free(serializarIdp);

	int bytes 		= paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);
	int resultado = -1;
	recv(socket_cliente,&resultado,sizeof(int),MSG_WAITALL);
	free(a_enviar);
	eliminar_paquete(paquete);
	close(socket_cliente);
	return resultado;
}

void errorIniciarPatota(){
	char* log = string_new();
	string_append(&log, "MEMORIA LLENA: No se puede iniciar Patota");
	logearArchivoDiscordiador(log);
}

void logearArchivoDiscordiador(char* log){
	char* horaActual 	= temporal_get_string_time("%d/%m/%y %H:%M:%S");
	pthread_mutex_lock(&mutexArchivoLogDiscordiador);
	FILE* archivoLog = fopen("../logDiscordiador.txt", "a+");
	fprintf(archivoLog, "%s:\t%s\n", horaActual, log);
	fclose(archivoLog);
	pthread_mutex_unlock(&mutexArchivoLogDiscordiador);
	free(horaActual);
	free(log);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * INICIO DE TRIPULANTE
 */

int tamanioInicioDeTripulante(struct inicioDeTripulante* idt){
	return sizeof(int) * 4;
}

void* serializarInicioDeTripulante(t_paquete* paquete, int bytes) {
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	agregar_a_serializacion(magic, &desplazamiento, &(paquete->codigo_operacion), sizeof(int));

	agregar_a_serializacion(magic, &desplazamiento, &(paquete->buffer->size), sizeof(int));

	struct inicioDeTripulante* idt = (struct inicioDeTripulante*)paquete->buffer->stream;

	agregar_a_serializacion(magic, &desplazamiento, &(idt->posX), sizeof(int));

	agregar_a_serializacion(magic, &desplazamiento, &(idt->posY), sizeof(int));

	agregar_a_serializacion(magic, &desplazamiento, &(idt->pid), sizeof(int));

	agregar_a_serializacion(magic, &desplazamiento, &(idt->tid), sizeof(int));

	return magic;
}

void enviarInicioDeTripulante(int socket_cliente, struct inicioDeTripulante* idt){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	int tamanioIdt     = tamanioInicioDeTripulante(idt);

	paquete->codigo_operacion = INICIAR_TRIPULANTE;
	paquete->buffer           = malloc(sizeof(t_buffer));
	paquete->buffer->size     = tamanioIdt;
	paquete->buffer->stream   = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, idt, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int); // COD OPERACION, SIZE

	void* a_enviar = serializarInicioDeTripulante(paquete, bytes);
	send(socket_cliente, a_enviar, bytes, 0);
	int* punteroResul = (int*)deserializar(socket_cliente, sizeof(int), "Resultado Iniciar Tripulante");
	free(punteroResul);

	free(a_enviar);
	eliminar_paquete(paquete);
}



void* iniciarTripulante(void* strTid){

	int tid = strtol((char*)strTid, NULL, 10);
	free(strTid);
	informarAMongo(tid, INICIAR_BITACORA);

	struct tripulantePlanificacion* tripulante 	= malloc(sizeof(struct tripulantePlanificacion));
	tripulante->tid 							= tid;
	tripulante->estado 							= 'N';
	tripulante->quantum 						= 0;
	tripulante->tiempoDeTarea					= 0;
	tripulante->proximaTarea					= malloc(sizeof(struct Tarea));
	tripulante->proximaTarea->nombre			= malloc(0);
	tripulante->solicitoES						= 0;
	tripulante->informacionES					= NULL;
	pthread_mutex_lock(&mutexListaTripulantes);
	list_add(listaTripulantes,tripulante);
	pthread_mutex_unlock(&mutexListaTripulantes);

	sem_t semaforoTripulante;
	sem_init(&semaforoTripulante, 0, 0);
	tripulante->semaforo = semaforoTripulante;
	char* log;

	while(tripulante->proximaTarea != NULL){
		if(tripulante->estado == 'N'){
			log = string_new();
			string_append_with_format(&log, "El tripulante %d entra a NEW", tripulante->tid);
			logearArchivoDiscordiador(log);
			int ficheroSocket       	= conectarAServidor(puerto_ram, ip_ram);
			solicitarProximaTarea(ficheroSocket, tid);
			struct Tarea* proximaTarea	= recibirProximaTarea(ficheroSocket);
			close(ficheroSocket);
			free(tripulante->proximaTarea->nombre);
			free(tripulante->proximaTarea);
			tripulante->proximaTarea 	= proximaTarea;
			pasarDeNewAReady(tripulante);
		} else if(tripulante->estado == 'R'){
			log = string_new();
			string_append_with_format(&log, "El tripulante %d entra a READY", tripulante->tid);
			logearArchivoDiscordiador(log);
			solicitarCambioDeEstado(tripulante);
			pthread_mutex_lock(&mutexListaReady);
			list_add(listaReady,tripulante);
			pthread_mutex_unlock(&mutexListaReady);
			if(resolviSabotaje){
				resolviSabotaje = 0;
			} else {
				sem_post(&estoyReady);
			}
			sem_wait(&tripulante->semaforo);
		} else if(tripulante->estado == 'E'){
			log = string_new();
			string_append_with_format(&log, "El tripulante %d entra a EXEC", tripulante->tid);
			logearArchivoDiscordiador(log);
			solicitarCambioDeEstado(tripulante);
			pthread_mutex_lock(&mutexListaExec);
			list_add(listaExec,tripulante);
			pthread_mutex_unlock(&mutexListaExec);
			if(valorSemaforoTripulante(tripulante) == 1 ){
				if(modoSabotaje == 1)resolverSabotaje(tripulante);
				else sem_wait(&(tripulante->semaforo));
			} else {
				ejecutar(tripulante);
				dejarDeEjecutar(tripulante, listaExec, mutexListaExec);
			}
		} else if(tripulante->estado == 'B'){
			log = string_new();
			string_append_with_format(&log, "El tripulante %d entra a BLOCK", tripulante->tid);
			logearArchivoDiscordiador(log);
			solicitarCambioDeEstado(tripulante);
			solicitoEntradaSalida(tripulante);
			desbloquearse(tripulante);
		} else if(tripulante->estado == 'X'){
			log = string_new();
			string_append_with_format(&log, "El tripulante %d entra a EXIT", tripulante->tid);
			logearArchivoDiscordiador(log);
			free(tripulante->proximaTarea->nombre);
			free(tripulante->proximaTarea);
			tripulante->proximaTarea = NULL;
			solicitarCambioDeEstado(tripulante);
			sem_destroy(&tripulante->semaforo);
			quitarTripulanteDeLista(tripulante->tid);
		}
	}
	free(tripulante);
	return 0;
}

void quitarTripulanteDeLista(int tid){
	pthread_mutex_lock(&mutexListaTripulantes);
	for(int i = 0; i < listaTripulantes->elements_count; i++){
		struct tripulantePlanificacion* tripulante = list_get(listaTripulantes,i);
		if(tripulante->tid == tid){
			list_remove(listaTripulantes,i);
		}
	}
	pthread_mutex_unlock(&mutexListaTripulantes);
}

void solicitoEntradaSalida(struct tripulantePlanificacion* tripulante){

	sem_wait(&semaforoBloqueados);

	solicitarES(tripulante->informacionES->tipoES, tripulante->informacionES->recurso, tripulante->proximaTarea->parametro);
	free(tripulante->informacionES);
	sleep(tripulante->proximaTarea->duracion * retardoCicloCPU);
	char* log = string_new();
	string_append_with_format(&log, "El tripulante %d termina la tarea %s", tripulante->tid, tripulante->proximaTarea->nombre);
	logearArchivoDiscordiador(log);
	if(planificacionPausada == 1) sem_wait(&semaforoBloqueado);
	//else if(planificacionPausada == 1) sem_wait(&tripulante->semaforo);

	sem_post(&semaforoBloqueados);

}

int verificarPlanificacionPausada(struct tripulantePlanificacion* tripulante){
	pthread_mutex_lock(&mutexPlanificacion);
	int estaPausada = planificacionPausada;
	pthread_mutex_unlock(&mutexPlanificacion);

	if(estaPausada == 1) {
		sem_wait(&(tripulante->semaforo));
		if(modoSabotaje == 1) {
			sem_post(&(tripulante->semaforo));
			return 1;
		}
	}
	return 0;
}

void desbloquearse(struct tripulantePlanificacion* tripulante){
	if(tripulante->estado == 'X') return;
	if(tripulante->tiempoDeTarea == tripulante->proximaTarea->duracion){
		pedirProximaTarea(tripulante);
	}
	if(tripulante->proximaTarea->duracion == -1){
		tripulante->estado = 'X';
	} else tripulante->estado = 'R';
}

void pedirProximaTarea(struct tripulantePlanificacion* tripulante){
	int ficheroSocketNuevo = conectarAServidor(puerto_mongo,ip_mongo);
	informarIoFTarea(ficheroSocketNuevo,tripulante,FINALIZACION_DE_TAREA);
	close(ficheroSocketNuevo);

	int ficheroSocket       	= conectarAServidor(puerto_ram, ip_ram);
	solicitarProximaTarea(ficheroSocket, tripulante->tid);
	struct Tarea* tareaActual	= recibirProximaTarea(ficheroSocket);
	close(ficheroSocket);
	free(tripulante->proximaTarea->nombre);
	free(tripulante->proximaTarea);
	tripulante->proximaTarea 	= tareaActual;
	tripulante->tiempoDeTarea 	= 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * EXPULSAR TRIPULANTE
 */

void solicitarExpulsionTripulante(int tid){
	struct tripulantePlanificacion* tripulante = obtenerTripulante(tid);
	if(tripulante != NULL){
		tripulante->estado = 'X';
		if(planificacionPausada == 1) {
			if(tripulante->estado == 'R'){
				pthread_mutex_lock(&mutexListaReady);
				for(int i = 0; i < listaReady->elements_count; i++){
					struct tripulantePlanificacion* tripulante = list_get(listaReady,i);
					if(tripulante->tid == tid){
						list_remove(listaReady,i);
					}
				}
				pthread_mutex_unlock(&mutexListaReady);
			}
			sem_post(&tripulante->semaforo);
		}
	} else {
		char* log = string_new();
		string_append_with_format(&log, "No existe el tripulante %d", tid);
		logearArchivoDiscordiador(log);
	}
}

struct tripulantePlanificacion* obtenerTripulante(int tid){
	struct tripulantePlanificacion* resultado = NULL;
	pthread_mutex_lock(&mutexListaTripulantes);
	for(int i = 0; i < listaTripulantes->elements_count; i++){
		struct tripulantePlanificacion* tripulante = list_get(listaTripulantes,i);
		if(tripulante->tid == tid) resultado = tripulante;
	}
	pthread_mutex_unlock(&mutexListaTripulantes);
	return resultado;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * REALIZAR TAREA
 */

void solicitarProximaTarea(int socket, int tid){
	void * magic        = malloc(sizeof(int) * 2);
	int desplazamiento  = 0;
	int codigoOperacion = PROXIMA_TAREA;
	//CODIGO DE OPERACION + PID
	agregar_a_serializacion(magic, &desplazamiento, &(codigoOperacion), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(tid), sizeof(int));
	send(socket, magic, sizeof(int) * 2, 0);
	free(magic);
}

int ponderarTarea(char* comando){
	if(strcmp(comando, "GENERAR_OXIGENO") == 0){
		return GENERAR_OXIGENO;
	}
	if(strcmp(comando, "CONSUMIR_OXIGENO") == 0){
		return CONSUMIR_OXIGENO;
	}
	if(strcmp(comando, "GENERAR_COMIDA") == 0){
			return GENERAR_COMIDA;
		}
	if(strcmp(comando, "CONSUMIR_COMIDA") == 0){
		return CONSUMIR_COMIDA;
	}
	if(strcmp(comando, "GENERAR_BASURA") == 0){
			return GENERAR_BASURA;
	}
	if(strcmp(comando, "DESCARTAR_BASURA") == 0){
		return DESCARTAR_BASURA;
	}
	return -1;
}

Tarea* recibirProximaTarea(int socket){
	int* longitudNombre		= (int*) deserializar(socket, sizeof(int), "Longitud del nombre");
	char* nombre 			= (char*)deserializar(socket, *longitudNombre + 1, "Nombre Tarea");
	int* parametro     		= (int*) deserializar(socket, sizeof(int), "Parametro Tarea");
	int* duracion     		= (int*) deserializar(socket, sizeof(int), "Duracion Tarea");
	int* ubicacionEnX     	= (int*) deserializar(socket, sizeof(int), "Ubicacion en X");
	int* ubicacionEnY     	= (int*) deserializar(socket, sizeof(int), "Ubicacion en Y");

	struct Tarea* proximaTarea 			= malloc(sizeof(struct Tarea));
	proximaTarea->nombre    			= nombre;
	proximaTarea->duracion    			= *duracion;
	proximaTarea->parametro 			= *parametro;
	proximaTarea->ubicacionEnX  	 	= *ubicacionEnX;
	proximaTarea->ubicacionEnY 			= *ubicacionEnY;
	free(parametro);
	free(longitudNombre);
	free(duracion);
	free(ubicacionEnX);
	free(ubicacionEnY);

	return proximaTarea;
}

int llegoAPosicion(struct tripulantePlanificacion* tripulante, int ubicacionEnX, int ubicacionEnY) {
	int ficheroNuevaConexion 	= conectarAServidor(puerto_ram, ip_ram);
	solicitarPosiciones(ficheroNuevaConexion, tripulante->tid);
	int* posX 			 	 	= (int*) deserializar(ficheroNuevaConexion, sizeof(int), "posicion X tripulante");
	int* posY 					= (int*) deserializar(ficheroNuevaConexion, sizeof(int), "posicion Y tripulante");
	close(ficheroNuevaConexion);
	int llego = (*posX == ubicacionEnX) && (*posY == ubicacionEnY);
	free(posX);
	free(posY);
	return llego;
}

void informarIoFTarea(int socket, struct tripulantePlanificacion* tripulante, int codigoOperacion){
	int tid = tripulante->tid;
	int longitudNombreTarea = strlen(tripulante->proximaTarea->nombre);
	char* nombreTarea 		= malloc(longitudNombreTarea + 1);
	strcpy(nombreTarea,tripulante->proximaTarea->nombre);
	int bytes 				= sizeof(int)*3 + longitudNombreTarea + 1;
	void* magic 			= malloc(bytes);
	int desplazamiento 		= 0;
	agregar_a_serializacion(magic,&desplazamiento,&codigoOperacion,sizeof(int));
	agregar_a_serializacion(magic,&desplazamiento,&tid,sizeof(int));
	agregar_a_serializacion(magic,&desplazamiento,&longitudNombreTarea,sizeof(int));
	agregar_a_serializacion(magic,&desplazamiento,nombreTarea,longitudNombreTarea + 1);

	send(socket,magic,bytes,0);
	int resultado = -1;
	recv(socket,&resultado,sizeof(int),MSG_WAITALL);
	free(nombreTarea);
	free(magic);
}

void solicitarES(int codigoOperacion, char* recurso, int cantidad){
	int longitudRecurso		= strlen(recurso);
	int bytes 				= sizeof(int)*3 + longitudRecurso + 1;
	void* magic 			= malloc(bytes);
	int desplazamiento 		= 0;
	agregar_a_serializacion(magic, &desplazamiento, &codigoOperacion	, sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &cantidad			, sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &longitudRecurso	, sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, recurso				, longitudRecurso + 1);

	int ficheroSocketNuevo = conectarAServidor(puerto_mongo,ip_mongo);
	send(ficheroSocketNuevo,magic,bytes,0);
	int resultado = -1;
	recv(ficheroSocketNuevo,&resultado,sizeof(int),MSG_WAITALL);
	close(ficheroSocketNuevo);
	free(magic);
}

void realizarTareaES(struct tripulantePlanificacion* tripulante, char* recurso, int codigoOperacion){
	if(tripulante->quantum == quantum && (strcmp(algoritmoPlanificacion, "RR") == 0)) return;
	tripulante->solicitoES = 1;
	tripulante->quantum++;
	sleep(retardoCicloCPU);
	tripulante->tiempoDeTarea 		= tripulante->proximaTarea->duracion;
	struct TareaES* informacionES 	= malloc(sizeof(struct TareaES));
	informacionES->recurso			= recurso;
	informacionES->tipoES			= codigoOperacion;
	tripulante->informacionES		= informacionES;
}

void realizarTarea(struct tripulantePlanificacion* tripulante){
	struct Tarea* tarea = tripulante->proximaTarea;
	while(tripulante->tiempoDeTarea < tarea->duracion) {
		if(tripulante->quantum == quantum && (strcmp(algoritmoPlanificacion, "RR") == 0)) break;

		if(verificarPlanificacionPausada(tripulante) || tripulante->estado == 'X') break;

		sleep(retardoCicloCPU);
		tripulante->quantum++;
		tripulante->tiempoDeTarea++;
	}
}

void ejecutar(struct tripulantePlanificacion* tripulante) {
	verificarPlanificacionPausada(tripulante);

	struct Tarea* proximaTarea = tripulante->proximaTarea;

	moverseAPosicion(tripulante, proximaTarea->ubicacionEnX, proximaTarea->ubicacionEnY, 0);

	if(llegoAPosicion(tripulante, proximaTarea->ubicacionEnX, proximaTarea->ubicacionEnY)){
		if(tripulante -> tiempoDeTarea == 0 && tripulante->quantum != quantum){
			int ficheroSocketNuevo = conectarAServidor(puerto_mongo,ip_mongo);
			informarIoFTarea(ficheroSocketNuevo,tripulante,COMIENZO_DE_TAREA);
			close(ficheroSocketNuevo);
			char* log = string_new();
			string_append_with_format(&log, "El tripulante %d empieza la tarea %s", tripulante->tid, tripulante->proximaTarea->nombre);
			logearArchivoDiscordiador(log);
		}
		int tarea = ponderarTarea(proximaTarea -> nombre);
		switch(tarea) {
			case GENERAR_OXIGENO:
				realizarTareaES(tripulante, "Oxigeno", GENERAR_RECURSOS);
				break;
			case CONSUMIR_OXIGENO:
				realizarTareaES(tripulante, "Oxigeno", DESCARTAR_RECURSOS);
				break;
			case GENERAR_COMIDA:
				realizarTareaES(tripulante, "Comida", GENERAR_RECURSOS);
				break;
			case CONSUMIR_COMIDA:
				realizarTareaES(tripulante, "Comida", DESCARTAR_RECURSOS);
				break;
			case GENERAR_BASURA:
				realizarTareaES(tripulante, "Basura", GENERAR_RECURSOS);
				break;
			case DESCARTAR_BASURA:
				realizarTareaES(tripulante, "Basura", DESCARTAR_RECURSOS);
				break;
			default:
				realizarTarea(tripulante);
				break;
		}
	}
	struct Tarea* tareaActual = tripulante->proximaTarea;
	if(tripulante->tiempoDeTarea == tareaActual->duracion && !tripulante->solicitoES){
		char* log = string_new();
		string_append_with_format(&log, "El tripulante %d termina la tarea %s", tripulante->tid, tareaActual->nombre);
		logearArchivoDiscordiador(log);
		pedirProximaTarea(tripulante);
		tareaActual = tripulante->proximaTarea;
		if(tareaActual->duracion != -1){
			ejecutar(tripulante);
		}
	}
}

void dejarDeEjecutar(struct tripulantePlanificacion* tripulante, t_list* lista, pthread_mutex_t mutex){

	if(tripulante->estado == 'X') {
		pthread_mutex_lock(&mutex);
		quitarDeLista(tripulante->tid, lista, "Exec");
		pthread_mutex_unlock(&mutex);
		sem_post(&procesadoresLibres);
		return;
	}

	tripulante->quantum = 0;

	pthread_mutex_lock(&mutex);
	quitarDeLista(tripulante->tid, lista, "Exec");
	pthread_mutex_unlock(&mutex);

	if(tripulante->proximaTarea->duracion == -1){
		tripulante->estado = 'X';
	} else if (tripulante->solicitoES){
		tripulante->estado 		= 'B';
		tripulante->solicitoES 	= 0;
	} else if (modoSabotaje != 1){
		tripulante->estado = 'R';
	}
	sem_post(&procesadoresLibres);

}

void quitarDeLista(int tid, t_list* lista, char* nombre){
	for(int i = 0; i < lista->elements_count; i++){
		struct tripulantePlanificacion* tripulante = list_get(lista,i);
		if(tripulante->tid == tid) {
			list_remove(lista,i);
			break;
		}
	}
}

void moverseAPosicion(struct tripulantePlanificacion* tripulante, int ubicacionX, int ubicacionY, int sabotajeActivado){
	int ficheroNuevaConexion 	= conectarAServidor(puerto_ram, ip_ram);
	solicitarPosiciones(ficheroNuevaConexion, tripulante->tid);
	int* posX 			 	 	= (int*) deserializar(ficheroNuevaConexion, sizeof(int), "posicion X tripulante");
	int* posY 					= (int*) deserializar(ficheroNuevaConexion, sizeof(int), "posicion Y tripulante");
	close(ficheroNuevaConexion);
	void sumar(int a, int b, int* resultado){
		*resultado = a + b;
	}
	void restar(int a, int b, int* resultado){
		*resultado = a - b;
	}
	void nada(int a, int b, int* resultado){
		*resultado = a;
	}

	int diferenciaX = abs(ubicacionX-*posX);

	if((ubicacionX-*posX) > 0) moverse(tripulante, *posX, *posY, diferenciaX, (void*)sumar, (void*)nada, sabotajeActivado);
	else moverse(tripulante, *posX, *posY, diferenciaX, (void*)restar, (void*)nada, sabotajeActivado);

	int diferenciaY = abs(ubicacionY-*posY);

	if((ubicacionY-*posY) > 0) moverse(tripulante, ubicacionX, *posY, diferenciaY, (void*)nada, (void*)sumar, sabotajeActivado);
	else moverse(tripulante, ubicacionX, *posY, diferenciaY, (void*)nada, (void*)restar, sabotajeActivado);
	free(posX);
	free(posY);
}

void moverse(struct tripulantePlanificacion* tripulante, int posX, int posY, int cant, void (*modificarX)(int, int, int*), void (*modificarY)(int, int, int*), int sabotajeActivado){
	int posXAnt = posX;
	int posYAnt = posY;
	for(int i = 1; i <= cant; i++){
		sleep(retardoCicloCPU);
		if(sabotajeActivado == 0){
			if(verificarPlanificacionPausada(tripulante) || tripulante->estado == 'X') break;

			if(tripulante->quantum == quantum && (strcmp(algoritmoPlanificacion, "RR") == 0)) break;
			tripulante->quantum++;
		}
		int posXAux;
		modificarX(posX, i, &posXAux);
		int posYAux;
		modificarY(posY, i, &posYAux);
		solicitarCambioPosicion(tripulante->tid, posXAux, posYAux, posXAnt, posYAnt);
		posXAnt = posXAux;
		posYAnt = posYAux;
	}
}

void solicitarPosiciones(int socket, int tid){
	void * magic        = malloc(sizeof(int) * 2);
	int desplazamiento  = 0;
	int codigoOperacion = POSICION_TRIPULANTE;
	//CODIGO DE OPERACION + PID
	agregar_a_serializacion(magic, &desplazamiento, &(codigoOperacion), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(tid), sizeof(int));
	send(socket, magic, sizeof(int) * 2, 0);
	free(magic);
}

void solicitarCambioPosicion(int tid, int posX, int posY, int posXVieja, int posYVieja){
	int bytes					  = sizeof(int) * 4;
	void* magic 				  = serializarCambioDePosicion(tid, posX, posY, bytes);
	int ficheroNuevaConexionMiRam = conectarAServidor(puerto_ram, ip_ram);
	send(ficheroNuevaConexionMiRam, magic, bytes, 0);
	int respuesta = -1;
	recv(ficheroNuevaConexionMiRam,&respuesta,sizeof(int),MSG_WAITALL);
	close(ficheroNuevaConexionMiRam);
	free(magic);

	bytes 					 	  = sizeof(int) * 6;
	int ficheroNuevaConexionMongo = conectarAServidor(puerto_mongo, ip_mongo);
	void* magic2				  = serializarMovimientoMongo(tid, posX, posY, posXVieja, posYVieja, bytes);
	send(ficheroNuevaConexionMongo, magic2, bytes, 0);
	free(magic2);
	int resultado = -1;
	recv(ficheroNuevaConexionMongo,&resultado,sizeof(int),MSG_WAITALL);
	close(ficheroNuevaConexionMongo);
}

void* serializarMovimientoMongo(int tid, int posXNueva, int posYNueva, int posXVieja, int posYVieja, int bytes){
	int codigoOperacion = MOVIMIENTO;
	void * magic        						= malloc(bytes);
	int desplazamiento  						= 0;
	//CODIGO DE OPERACION + TID + ESTADO;
	agregar_a_serializacion(magic, &desplazamiento, &(codigoOperacion), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(tid), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(posXNueva), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(posYNueva), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(posXVieja), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(posYVieja), sizeof(int));

	return magic;
}

void* serializarCambioDePosicion(int tid, int posX, int posY, int bytes){
	int codigoOperacion = CAMBIO_DE_POSICION;
	void * magic        						= malloc(bytes);
	int desplazamiento  						= 0;
	//CODIGO DE OPERACION + TID + ESTADO;
	agregar_a_serializacion(magic, &desplazamiento, &(codigoOperacion), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(tid), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(posX), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(posY), sizeof(int));

	return magic;
}


void* solicitarCambioDeEstado(void* data){
	struct tripulantePlanificacion* tripulante 	= (struct tripulantePlanificacion*)data;
	int bytes									= sizeof(int) * 2 + sizeof(char);
	void * magic        						= malloc(bytes);
	int desplazamiento  						= 0;
	int codigoOperacion 						= CAMBIO_DE_ESTADO;
	//CODIGO DE OPERACION + TID + ESTADO;
	agregar_a_serializacion(magic, &desplazamiento, &(codigoOperacion), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(tripulante->tid), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(tripulante->estado), sizeof(char));
	int ficheroNuevaConexion = conectarAServidor(puerto_ram, ip_ram);
	send(ficheroNuevaConexion, magic, bytes, 0);
	int respuesta = -1;
	recv(ficheroNuevaConexion,&respuesta,sizeof(int),MSG_WAITALL);
	close(ficheroNuevaConexion);
	free(magic);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * INICIAR PLANIFICACION
 */

void inicializarColas(){
	colaSabotaje		= queue_create();
	listaReady 			= list_create ();
	listaExec 			= list_create ();
	listaTripulantes	= list_create ();
	sem_init(&sabotajeResuelto, 0, 0);
	sem_init(&procesadoresLibres, 0, gradoMultitarea);
	sem_init(&listoParaResolverSabotaje,0,0);
	sem_init(&semaforoBloqueados,0,1);
	sem_init(&semaforoBloqueado,0,0);
	sem_init(&semaforoPlanificador,0,0);
	sem_init(&estoyReady,0,0);

	pthread_t planificadorCortoPlazo;
	pthread_create(&planificadorCortoPlazo,NULL, planificador,NULL);
	pthread_detach(planificadorCortoPlazo);
}

void* planificador(void* data){
	while(1){
		sem_wait(&estoyReady);
		sem_wait(&procesadoresLibres);
		if(flagExitDiscordiador){
			sem_post(&planificadorLiberado);
			break;
		}

		pthread_mutex_lock(&mutexPlanificacion);
		int estaPausada = planificacionPausada;
		pthread_mutex_unlock(&mutexPlanificacion);

		if(estaPausada == 1) {
			planificadorBloqueado = 1;
			sem_wait(&semaforoPlanificador);
		}

		pthread_mutex_lock(&mutexListaReady);
		struct tripulantePlanificacion* tripulante = list_remove(listaReady,0);
		pthread_mutex_unlock(&mutexListaReady);

		if(tripulante->estado != 'X') {
			tripulante->estado = 'E';
		} else {
			sem_post(&procesadoresLibres);
		}
		sem_post(&tripulante->semaforo);
	}
	return 0;
}

int valorSemaforoTripulante(struct tripulantePlanificacion* tripulante){
	int valor;
	sem_getvalue(&(tripulante->semaforo),&valor);
	return valor;
}

void pasarDeReadyAExec(struct tripulantePlanificacion* tripulante) {
	sem_wait(&procesadoresLibres);

	pthread_mutex_lock(&mutexPlanificacion);
	int estaPausada = planificacionPausada;
	pthread_mutex_unlock(&mutexPlanificacion);

	if(estaPausada == 1) {
		if(valorSemaforoTripulante(tripulante) != 1){
			if(modoSabotaje != 1){
				sem_wait(&(tripulante->semaforo));
			}
			while(modoSabotaje == 1){
				sem_wait(&(tripulante->semaforo));
				sem_wait(&procesadoresLibres);
			}
		}
	}
	pthread_mutex_lock(&mutexListaReady);
	quitarDeLista(tripulante->tid, listaReady, "Ready");
	pthread_mutex_unlock(&mutexListaReady);

	tripulante->estado 	= 'E';
}

void pasarDeNewAReady(struct tripulantePlanificacion* tripulante){

	pthread_mutex_lock(&mutexPlanificacion);
	int estaPausada = planificacionPausada;
	pthread_mutex_unlock(&mutexPlanificacion);
	if(estaPausada == 1) sem_wait(&(tripulante->semaforo));
	if(tripulante->estado == 'X') return;

	tripulante->estado = 'R';
}


////////////////////////////////////////////////////////////////////////////////////////////
/*
 * SABOTAJE
 */

void* conectarseAMongo(void* data){
	int ficheroSocketMongo 	= conectarAServidor(puerto_mongo, ip_mongo);
	int codigo 				= CONEXION_SABOTAJE;
	send(ficheroSocketMongo,&codigo,sizeof(int),0);
	while(1){
		int* longitudPosicion = malloc(sizeof(int));
		int recibido = recv(ficheroSocketMongo,longitudPosicion,sizeof(int),MSG_WAITALL);
		if(recibido != 0){
			char* posicion = malloc(*longitudPosicion);
			recv(ficheroSocketMongo,posicion,*longitudPosicion + 1,MSG_WAITALL);
			iniciarProtocoloSabotaje(posicion);
			free(posicion);
		}
		free(longitudPosicion);
		if(recibido == 0) {
			close(ficheroSocketMongo);
			break;
		}
	}
	sem_post(&conexionMongoLiberada);
	return 0;
}

void* iniciarProtocoloSabotaje(void* posicion){
	pausarPlanificacion(NULL);
	t_list* listaAuxiliar 		= list_create();
	int cantidadDeTripulantes 	= 0;
	int cantidadAux 			= 0;
	modoSabotaje 				= 1;
	sem_init(&tareaCambiada,0,0);
	struct Tarea* tareaSabotaje = malloc(sizeof(struct Tarea));
	tareaSabotaje->duracion		= duracionSabotaje;
	char** arrPosiciones		= string_split(posicion, "|");
	tareaSabotaje->ubicacionEnX	= strtol(arrPosiciones[0], NULL, 10);
	tareaSabotaje->ubicacionEnY = strtol(arrPosiciones[1], NULL, 10);
	free(arrPosiciones[0]);
	arrPosiciones[0] = NULL;
	free(arrPosiciones[1]);
	arrPosiciones[1] = NULL;
	free(arrPosiciones);

	int distancia 	= -1;
	int tripulantesEnExec = 0;

	while(listaExec->elements_count != 0){
		pthread_mutex_lock(&mutexListaExec);
		struct tripulantePlanificacion* tripulante 	= list_remove(listaExec,0);
		pthread_mutex_unlock(&mutexListaExec);

		list_add_sorted(listaAuxiliar,tripulante,tidAscendente);
		tripulantesEnExec++;
		cantidadDeTripulantes++;

	}
	while(listaReady->elements_count != 0){
		pthread_mutex_lock(&mutexListaReady);
		struct tripulantePlanificacion* tripulante 	= list_remove(listaReady,0);
		pthread_mutex_unlock(&mutexListaReady);
		list_add_sorted(listaAuxiliar,tripulante,tidAscendente);
		cantidadDeTripulantes++;
	}
	int indexMejorPosicionado;
	for(int i = 0; i < listaAuxiliar->elements_count; i++){
		struct tripulantePlanificacion* tripulante 	= list_get(listaAuxiliar,i);
		int ficheroNuevaConexion 		= conectarAServidor(puerto_ram, ip_ram);
		solicitarPosiciones(ficheroNuevaConexion, tripulante->tid);
		int* posX 			 	 	= (int*) deserializar(ficheroNuevaConexion, sizeof(int), "posicion X tripulante");
		int* posY 					= (int*) deserializar(ficheroNuevaConexion, sizeof(int), "posicion Y tripulante");
		close(ficheroNuevaConexion);
		int distanciaAux 							= abs(tareaSabotaje->ubicacionEnX-*posX)+abs(tareaSabotaje->ubicacionEnY-*posY);
		free(posX);
		free(posY);
		if(distanciaAux < distancia || distancia == -1) {
			indexMejorPosicionado 	= i;
			distancia 				= distanciaAux;
		}
	}

	struct tripulantePlanificacion* mejorPosicionado = list_remove(listaAuxiliar,indexMejorPosicionado);
	if(mejorPosicionado->estado == 'E') {
		tripulantesEnExec--;
	} else {
		mejorPosicionado->estado = 'E';
		sem_post(&(mejorPosicionado->semaforo));
		resolviSabotaje = 1;
	}
	sem_post(&(mejorPosicionado->semaforo));

	int index = 0;

	char estados[cantidadDeTripulantes];
	while(tripulantesEnExec > 0){
		struct tripulantePlanificacion* tripulante = list_get(listaAuxiliar,index);
		if(tripulante->estado == 'E'){
			tripulante = list_remove(listaAuxiliar,index);
			queue_push(colaSabotaje,tripulante);
			estados[cantidadAux] 	= 'E';

			cantidadAux++;
			tripulante->estado 		= 'B';
			solicitarCambioDeEstado(tripulante);
			tripulantesEnExec--;
		} else index++;
	}
	while(listaAuxiliar->elements_count != 0){
		struct tripulantePlanificacion* tripulante = list_remove(listaAuxiliar,0);
		queue_push(colaSabotaje,tripulante);
		estados[cantidadAux] 	= 'R';
		cantidadAux++;
		tripulante->estado 		= 'B';
		solicitarCambioDeEstado(tripulante);
	}

	sem_wait(&listoParaResolverSabotaje);
	struct Tarea* tareaAnterior = mejorPosicionado->proximaTarea;
	mejorPosicionado->proximaTarea = tareaSabotaje;
	sem_post(&tareaCambiada);
	sem_wait(&sabotajeResuelto);
	mejorPosicionado->proximaTarea = tareaAnterior;
	free(tareaSabotaje);


	cantidadAux = 0;
	modoSabotaje = 0;
	pthread_mutex_lock(&mutexPlanificacion);
	planificacionPausada = 0;
	pthread_mutex_unlock(&mutexPlanificacion);
	sem_post(&semaforoBloqueado);
	while(queue_size(colaSabotaje) != 0){
		struct tripulantePlanificacion* tripulante 	= queue_pop(colaSabotaje);
		int estado									= estados[cantidadAux];
		cantidadAux++;
		tripulante->estado 							= estado;
		solicitarCambioDeEstado(tripulante);
		if(estado == 'E'){
			pthread_mutex_lock(&mutexListaExec);
			list_add(listaExec,tripulante);
			pthread_mutex_unlock(&mutexListaExec);
			sem_post(&(tripulante->semaforo));
		} else {
			pthread_mutex_lock(&mutexListaReady);
			list_add(listaReady,tripulante);
			pthread_mutex_unlock(&mutexListaReady);
		}
	}
	sem_post(&(mejorPosicionado->semaforo));

	if(planificadorBloqueado){
		sem_post(&semaforoPlanificador);
		planificadorBloqueado = 0;
	}

	list_destroy(listaAuxiliar);
	sem_destroy(&tareaCambiada);
	return 0;
}

_Bool tidAscendente(void* tripu1, void* tripu2){
	if(((struct tripulantePlanificacion*)tripu1)->tid < ((struct tripulantePlanificacion*)tripu2)->tid) return true;
	else return false;
}

void resolverSabotaje(struct tripulantePlanificacion* tripulante){
	sem_post(&listoParaResolverSabotaje);
	sem_wait(&tareaCambiada);

	informarAMongo(tripulante->tid, CORRIDA_EN_PANICO);
	char* log = string_new();
	string_append_with_format(&log, "El tripulante %d empieza corrida en panico", tripulante->tid);
	logearArchivoDiscordiador(log);
	moverseAPosicion(tripulante, tripulante->proximaTarea->ubicacionEnX, tripulante->proximaTarea->ubicacionEnY, 1);
	informarAMongo(tripulante->tid, FSCK);
	sleep(tripulante->proximaTarea->duracion*retardoCicloCPU);
	informarAMongo(tripulante->tid, SABOTAJE_RESUELTO);
	log = string_new();
	string_append_with_format(&log, "El tripulante %d resolvio el sabotaje", tripulante->tid);
	logearArchivoDiscordiador(log);
	pthread_mutex_lock(&mutexListaExec);
	quitarDeLista(tripulante->tid, listaExec, "Exec");
	pthread_mutex_unlock(&mutexListaExec);
	sem_post(&sabotajeResuelto);

	tripulante->estado = 'R';
	sem_wait(&(tripulante->semaforo));
	sem_wait(&(tripulante->semaforo));
}

void informarAMongo(int tid, int codigoOperacion){
	int ficheroSocket       = conectarAServidor(puerto_mongo, ip_mongo);
	int bytes 				= sizeof(int)*2;
	void* magic 			= malloc(bytes);
	int desplazamiento 		= 0;
	agregar_a_serializacion(magic,&desplazamiento,&codigoOperacion,sizeof(int));
	agregar_a_serializacion(magic,&desplazamiento,&tid,sizeof(int));

	send(ficheroSocket,magic,bytes,0);
	int resultado = -1;
	recv(ficheroSocket,&resultado,sizeof(int),MSG_WAITALL);
	free(magic);
	close(ficheroSocket);
}


////////////////////////////////////////////////////////////////////////////////////////////
 /*
  * OBTENER BITACORA
  */

void recibirBitacora(int ficheroNuevaConexion, int tid){
	char* horaActual 	= temporal_get_string_time("%d/%m/%y %H:%M:%S");
	int* tamanioBitacora 	= (int*) deserializar(ficheroNuevaConexion, sizeof(int)			, "Lognitud Bitacora");
	char* log = string_new();
	if (*tamanioBitacora == 0){
		string_append_with_format(&log,"La bitacora del tripulante %d esta vacia", tid);
		logearArchivoDiscordiador(log);
	} else if(*tamanioBitacora != -1){
		char* logBitacora 		= (char*)deserializar(ficheroNuevaConexion, *tamanioBitacora + 1, "Bitacora");
		string_append_with_format(&log,"Bitacora tripulante %d: \n%s", tid, logBitacora);
		logearArchivoDiscordiador(log);
		free(logBitacora);
	} else {
		string_append_with_format(&log,"No existe el tripulante %d", tid);
		logearArchivoDiscordiador(log);
	}

	free(horaActual);
	free(tamanioBitacora);
}

////////////////////////////////////////////////////////////////////////////////////////////
 /*
  * FUNCIONES HILOS
  */

void* iniciarPatota(void* data){
	t_list* tokens 				= (t_list*)data;
	if(tokens->elements_count == 0){
		errorComandoDiscordiador();
		return 0;
	}
	char* strCantTripulantes 	= list_remove(tokens,0);
	int cantTripulantes			= strtol(strCantTripulantes, NULL, 10);
	free(strCantTripulantes);
	if(cantTripulantes == 0){
		errorComandoDiscordiador();
		return 0;
	}
	if(tokens->elements_count == 0){
		errorComandoDiscordiador();
		return 0;
	}
	char* nombreArchivoTareas 	= list_remove(tokens,0);
	t_list* lstPosiciones       = list_create();
	for(int i = 0; i < tokens->elements_count; i++){
		char* token = list_get(tokens,i);
		if(string_contains(token,"|")) list_add(lstPosiciones, token);
	}
	t_list* listaTareas         = llenarTareas(nombreArchivoTareas);
	free(nombreArchivoTareas);
	if(listaTareas == NULL){
		errorComandoDiscordiador();
		return 0;
	}
	pthread_mutex_lock(&mutexContadorPid);
	int pidActual           	= contadorPid++;
	pthread_mutex_unlock(&mutexContadorPid);

	struct inicioDePatota* idp 	= malloc(sizeof(struct inicioDePatota));
	idp->pid					= pidActual;
	idp->listaTareas			= listaTareas;
	idp->cantTripulantes		= cantTripulantes;
	idp->posiciones			 	= lstPosiciones;
	pthread_mutex_lock(&mutexContadorTid);
	idp->primerTid				= contadorTid;
	contadorTid				   += cantTripulantes;
	pthread_mutex_unlock(&mutexContadorTid);
	int ficheroNuevaConexion 	= conectarAServidor(puerto_ram, ip_ram);
	int resultado				= enviarInicioDePatota(ficheroNuevaConexion, idp);

	if(resultado != -1){
		for(int i = 0; i < cantTripulantes; i++){
			pthread_t threadTripulante[cantTripulantes];
			int tid 		= idp->primerTid++;
			char* strTid 	= string_itoa(tid);
			pthread_create(&(threadTripulante[i]), NULL, iniciarTripulante, strTid);
			pthread_detach(threadTripulante[i]);
		}
	} else {
		errorIniciarPatota();
	}
	free(idp);
	while(tokens->elements_count != 0){
		char* posicion = list_remove(tokens, 0);
		free(posicion);
	}
	list_destroy(tokens);
	list_destroy(listaTareas);
	return 0;
}

void* listarTripulantes(void* data){
	int socketListarTripulantes = conectarAServidor(puerto_ram, ip_ram);
	solicitarTripulantes(socketListarTripulantes);
	t_list* tripulantes 		= recibirListaTripulantes(socketListarTripulantes);
	imprimirTripulantes(tripulantes);
	return 0;
}

void* reanudarPlanificacion(void* data){
	if(!planificacionPausada){
		char* log = string_new();
		string_append(&log,"ERROR: La planificacion ya esta iniciada.");
		logearArchivoDiscordiador(log);
	} else {
		pthread_mutex_lock(&mutexPlanificacion);
		planificacionPausada = 0;
		pthread_mutex_unlock(&mutexPlanificacion);
		for(int i = 0; i < listaTripulantes->elements_count; i++) {
			struct tripulantePlanificacion* tripulante = list_get(listaTripulantes,i);
			if(tripulante->estado != 'R' && tripulante->estado != 'B') sem_post(&(tripulante->semaforo));
		}

		if(planificadorBloqueado) sem_post(&semaforoPlanificador);
		sem_post(&semaforoBloqueado);
	}
	return 0;
}

void* pausarPlanificacion(void* data){
	if(planificacionPausada){
		char* log = string_new();
		string_append(&log,"ERROR: La planificacion ya esta pausada.");
		logearArchivoDiscordiador(log);
	} else {
		pthread_mutex_lock(&mutexPlanificacion);
		planificacionPausada = 1;
		pthread_mutex_unlock(&mutexPlanificacion);
	}
	return 0;
}

void* obtenerBitacora(void* data){
	int tid 			= strtol((char*)data, NULL, 10);
	int bytes			= sizeof(int)* 2 ;
	void * magic        = malloc(bytes);
	int desplazamiento  = 0;
	int codigoOperacion = ENVIAR_BITACORA;
	agregar_a_serializacion(magic, &desplazamiento, &(codigoOperacion), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(tid), sizeof(int));
	int ficheroNuevaConexion = conectarAServidor(puerto_mongo, ip_mongo);
	send(ficheroNuevaConexion, magic, bytes, 0);
	free(magic);
	recibirBitacora(ficheroNuevaConexion, tid);
	close(ficheroNuevaConexion);
	free(data);
	return 0;
}

void* expulsarTripulanteHilo(void* data){
	int tid = strtol((char*)data, NULL, 10);
	solicitarExpulsionTripulante(tid);
	free(data);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////
 /*
  * EXIT
  */

void terminarDiscordiador(){
	sem_post(&estoyReady);
}

void terminarMiRam(){
	int bytes									= sizeof(int);
	void * magic        						= malloc(bytes);
	int desplazamiento  						= 0;
	int codigoOperacion 						= EXIT;
	agregar_a_serializacion(magic, &desplazamiento, &(codigoOperacion), sizeof(int));
	int ficheroNuevaConexion = conectarAServidor(puerto_ram, ip_ram);
	send(ficheroNuevaConexion, magic, bytes, 0);
	free(magic);
	close(ficheroNuevaConexion);
}

void terminarIMongo(){
	int bytes			= sizeof(int);
	void * magic        = malloc(bytes);
	int desplazamiento  = 0;
	int codigoOperacion = EXIT;
	agregar_a_serializacion(magic, &desplazamiento, &(codigoOperacion), sizeof(int));
	int ficheroNuevaConexion = conectarAServidor(puerto_mongo, ip_mongo);
	send(ficheroNuevaConexion, magic, bytes, 0);
	free(magic);
	close(ficheroNuevaConexion);
}

void esperarHilosDiscordiador(){
	sem_wait(&conexionMongoLiberada);
	sem_wait(&planificadorLiberado);
}
