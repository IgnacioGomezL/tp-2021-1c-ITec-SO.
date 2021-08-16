/*
 * miRamUtils.c
 *
 *  Created on: 30 may. 2021
 *      Author: utnso
 */

#include "miRamUtils.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * RECIBIR DE DISCORDIADOR
 */



struct inicioDePatota* recibirInicioDePatota(int socket_cliente){
	int* size           	= (int*)deserializar(socket_cliente, sizeof(int), "size Buffer");
	int* pid             	= (int*)deserializar(socket_cliente, sizeof(int), "PID");
	int* cantTripulantes 	= (int*)deserializar(socket_cliente, sizeof(int), "Cantidad de Tripulantes");
	int* cantidadTareas  	= (int*)deserializar(socket_cliente, sizeof(int), "Cantidad de Tareas");

	t_list* tareas 	= list_create();

	for(int i = 0; i < *cantidadTareas; i++){
		int* longitudTarea 					= (int*)deserializar(socket_cliente, sizeof(int), "Longitud Tarea");
		char* tarea 						= (char*)deserializar(socket_cliente, *longitudTarea + 1, "Tarea");

		list_add(tareas, tarea);
		free(longitudTarea);
	}

	int* cantidadPosiciones  = (int*)deserializar(socket_cliente, sizeof(int), "Cantidad Posiciones");

	t_list* posiciones = list_create();
	for(int i = 0; i < *cantidadPosiciones; i++){
		int* longitudPosicion	= (int*)deserializar(socket_cliente, sizeof(int), "Longitud Posicion");
		char* posicion			= (char*)deserializar(socket_cliente, *longitudPosicion + 1, "Posicion");
		list_add(posiciones, posicion);
		free(longitudPosicion);
	}

	int* primerTid	= (int*)deserializar(socket_cliente, sizeof(int), "Primer TID");

	struct inicioDePatota* idp 	= malloc(sizeof(struct inicioDePatota));
	idp->listaTareas     		= tareas;
	idp->pid             		= *pid;
	idp->cantTripulantes 		= *cantTripulantes;
	idp->posiciones				= posiciones;
	idp->primerTid				= *primerTid;

	free(pid);
	free(cantTripulantes);
	free(size);
	free(cantidadTareas);
	free(primerTid);
	free(cantidadPosiciones);

	return idp;
}

struct inicioDeTripulante* recibirInicioDeTripulante(int socket_cliente){
	int* size	= (int*)deserializar(socket_cliente, sizeof(int), "size Buffer");
	int* posX	= (int*)deserializar(socket_cliente, sizeof(int), "POS X");
	int* posY	= (int*)deserializar(socket_cliente, sizeof(int), "POS Y");
	int* pid    = (int*)deserializar(socket_cliente, sizeof(int), "PID");
	int* tid    = (int*)deserializar(socket_cliente, sizeof(int), "TID");
	struct inicioDeTripulante* idt = malloc(sizeof(struct inicioDeTripulante));

	idt->posX = *posX;
	idt->posY = *posY;
	idt->pid  = *pid;
	idt->tid  = *tid;

	free(posX);
	free(posY);
	free(pid);
	free(tid);
	free(size);

	return idt;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * PATOTA
 */

int generarEstructurasPatota(struct inicioDePatota* idp){

	int resultado = -1;
	//pthread_mutex_lock(&mutexPatota);
	if(strcmp(esquemaMemoria,"SEGMENTACION") == 0){
		resultado = generarEstructurasPatotaSegmentacion(idp);
	} else if(strcmp(esquemaMemoria, "PAGINACION") == 0){
		resultado = generarEstructurasPatotaPaginacion(idp);
	}
	//pthread_mutex_unlock(&mutexPatota);
	if(resultado != -1){
		for(int i = 0; i < idp->cantTripulantes; i++) {
			int posX = 0;
			int posY = 0;
			if(idp->posiciones->elements_count != 0){
				char* posicion		 = list_remove(idp->posiciones, 0);
				char** arrPosiciones = string_split(posicion, "|");
				posX = strtol(arrPosiciones[0], NULL, 10);
				posY = strtol(arrPosiciones[1], NULL, 10);
				free(arrPosiciones[0]);
				arrPosiciones[0] = NULL;
				free(arrPosiciones[1]);
				arrPosiciones[1] = NULL;
				free(arrPosiciones);
				free(posicion);
			}
			generarEstructurasTripulante(posX, posY, idp->primerTid + i, idp->pid);
		}
	}
	imprimirTabla("Se agrega una patota");
	liberarIdp(idp);
	return resultado;
}

void liberarIdp(struct inicioDePatota* idp){
	for(int i = 0; i < idp->listaTareas->elements_count; i++){
		char* tarea = list_get(idp->listaTareas, i);
		free(tarea);
	}
	list_destroy(idp->listaTareas);
	list_destroy(idp->posiciones);
	free(idp);
}

int tamanioEnMemoriaPatota(struct inicioDePatota* idp) {
	return tamanioListaTareas(idp->listaTareas) + TAMANIO_PCB + TAMANIO_TCB * idp->cantTripulantes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * TRIPULANTE
 */

int generarEstructurasTripulante(int posX, int posY, int tid, int pid){
	//pthread_mutex_lock(&mutexTripulante);
	if(strcmp(esquemaMemoria,"SEGMENTACION") == 0){
		generarEstructurasTripulanteSegmentacion(posX, posY, tid, pid);
	} else if(strcmp(esquemaMemoria, "PAGINACION") == 0){
		generarEstructurasTripulantePaginacion(posX, posY, tid, pid);
	}
	//pthread_mutex_unlock(&mutexTripulante);

	pthread_mutex_lock(&mutexCantidadTripulantes);
	cantidadDeTripulantes++;
	pthread_mutex_unlock(&mutexCantidadTripulantes);

	if(mostrarMapa){
		inicializarTripulante(tid, posX, posY);
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * MEMORIA
 */

void* reservarMemoriaRam(int tamanio){
	return malloc(tamanio);
}

void inicializarTabla(){
	if(strcmp(esquemaMemoria,"SEGMENTACION") == 0){
		inicializarTablaSegmentos();
	} else if (strcmp(esquemaMemoria,"PAGINACION") == 0){
		inicializarTablaPaginas();
	}
}

void liberarTabla(){
	if(strcmp(esquemaMemoria,"SEGMENTACION") == 0){
		liberarTablaSegmentos();
	} else if (strcmp(esquemaMemoria,"PAGINACION") == 0){
		liberarTablaPaginas();
	}
}

void* imprimirTabla(void* informacion){
	mkdir("Dumps", 0777);
	char* horaActual 	= temporal_get_string_time("%H:%M:%S:%MS");
	char* diaActual 	= temporal_get_string_time("%d/%m/%y");
	char* nombreDump  	= string_new();
	string_append_with_format(&nombreDump, "Dumps/Dump_%s.txt", horaActual);

	FILE* dump;
	if(informacion != NULL){
		dump = fopen("../logMiRamHQ.txt", "a+");
	} else {
		dump = fopen(nombreDump, "a+");
	}
	free(nombreDump);
	fprintf(dump, "-------------------------------------------------------------------------------------------------------------\n");
	fprintf(dump, "Dump: %s %s\n", diaActual, horaActual);
	free(horaActual);
	free(diaActual);
	if(strcmp(esquemaMemoria,"SEGMENTACION") == 0){
		imprimirTablaSegmentos(dump);
	} else if (strcmp(esquemaMemoria,"PAGINACION") == 0){
		imprimirTablaPaginas(dump,listaDeFrames,0);
	}
	fprintf(dump, "------------------------------------------------------------------------------------------------------------\n");
	fclose(dump);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * SEGMENTACION
 */

void inicializarTablaSegmentos(){
	tablasPatotas 				= list_create();
	listaDeSegmentos			= list_create();
	struct segmento* segmento 	= malloc(sizeof(struct segmento));
	segmento->idSegmento 		= -1;
	segmento->inicio			= 0;
	segmento->tamanio			= tamanioMemoria;
	segmento->tipoDeDato		= 'V';
	list_add(listaDeSegmentos, segmento);

}

void liberarTablaSegmentos(){
	while(listaDeSegmentos->elements_count != 0){
		struct segmento* segmento = list_remove(listaDeSegmentos, 0);
		free(segmento);
	}
	list_destroy(listaDeSegmentos);
}


struct tablaSegmentosPatota* buscarTablaSegmentos(int pid){
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaSegmentosPatota* tablaPatota = list_get(tablasPatotas, i);
		if(tablaPatota->pid == pid){
			return tablaPatota;
		}
	}
	return NULL;
}

void borrarSegmentoTCB(int tid){
	int inicio = 0;
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaSegmentosPatota* tablaPatota = list_get(tablasPatotas, i);
		waitSemaforoSegmentosTCB(tablaPatota);
		t_list* segmentosTCB                     = tablaPatota->segmentosTCB;
		for(int j = 0; j < segmentosTCB->elements_count; j++){
			struct segmento* segmentoTCB = list_get(segmentosTCB, j);
			inicio                       = segmentoTCB->inicio;
			void* data                   = obtenerDeMemoria(&(inicio), segmentoTCB->tamanio, 0);
			struct TCB* tcb              = deserializarTCB(data);
			free(data);
			if(tcb->TID == tid){
				list_remove(segmentosTCB, j);
				borrarSegmentoEnListaSegmentos(segmentoTCB);
				//free(segmentoTCB);
			}
			free(tcb);
		}
		postSemaforoSegmentosTCB(tablaPatota);
		if(segmentosTCB->elements_count == 0){
			list_destroy(segmentosTCB);
			borrarSegmentoEnListaSegmentos(tablaPatota->pcb);
			borrarSegmentoEnListaSegmentos(tablaPatota->tareas);
			//free(tablaPatota->pcb);
			//free(tablaPatota->tareas);
			pthread_mutex_lock(&mutexListaTablasPatotas);
			list_remove(tablasPatotas, i);
			pthread_mutex_unlock(&mutexListaTablasPatotas);
			sem_destroy(&tablaPatota->semaforoSegmentosTCB);
			free(tablaPatota);
		}
	}
}

struct segmento* buscarSegmentoTCB(int tid){
	int inicio 					= 0;
	struct segmento* resultado 	= NULL;
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaSegmentosPatota* tablaPatota = list_get(tablasPatotas, i);
		t_list* segmentosTCB                     = tablaPatota->segmentosTCB;
		for(int j = 0; j < segmentosTCB->elements_count; j++){
			struct segmento* segmentoTCB = list_get(segmentosTCB, j);
			inicio                       = segmentoTCB->inicio;
			void* data                   = obtenerDeMemoria(&(inicio), segmentoTCB->tamanio, 0);
			struct TCB* tcb              = deserializarTCB(data);
			free(data);
			if(tcb->TID == tid){
				resultado = segmentoTCB;
				free(tcb);
				break;
			}
			free(tcb);
		}
		if(resultado != NULL) break;
	}
	return resultado;
}


void borrarSegmentoEnListaSegmentos(struct segmento* segmento){
	pthread_mutex_lock(&mutexListaSegmentos);
	for(int i = 0; i < listaDeSegmentos->elements_count; i++){
		struct segmento* seg = list_get(listaDeSegmentos,i);
		if(segmento==seg){
			seg->idSegmento = -1;
			seg->tipoDeDato = 'V';
		}
	}
	pthread_mutex_unlock(&mutexListaSegmentos);
}

void imprimirTablaSegmentos(FILE* archivo){
	int inicio = 0;
	for(int i = 0; i < listaDeSegmentos->elements_count; i++){
		for(int j = 0; i < listaDeSegmentos->elements_count; j++){
			struct segmento* segmento 	= list_get(listaDeSegmentos, j);
			if(segmento->inicio == inicio){
				int pid 					= perteneceAPatota(segmento);
				imprimirSegmento(segmento, pid, archivo);
				inicio += segmento->tamanio;
				break;
			}
		}
	}
}

int perteneceAPatota(struct segmento* segmento){
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaSegmentosPatota* tablaActual = list_get(tablasPatotas, i);
		for(int j = 0; j < tablaActual->segmentosTCB->elements_count; j++){
			struct segmento* tcb = list_get(tablaActual->segmentosTCB, j);
			if(tcb == segmento) return tablaActual->pid;
		}
		if(segmento == tablaActual->tareas || segmento == tablaActual->pcb){
			return tablaActual->pid;
		}
	}
	return -1;
}

void imprimirSegmento(struct segmento* seg, int numProceso, FILE* archivo){
	fprintf(archivo, "Proceso: %d\tTipo: %c\t\tSegmento: %d\t Inicio: 0x%x\tTam: %db\n", numProceso, seg->tipoDeDato, seg->idSegmento, seg->inicio, seg->tamanio);
}
int calcularInicioEnMemoriaSegmentacion(int tamanioBloque,int* index, int modificaEspacioVacio){
	int resultado = -1;
	pthread_mutex_lock(&mutexListaSegmentos);
	int cantidadSegmentos = listaDeSegmentos->elements_count;
	pthread_mutex_unlock(&mutexListaSegmentos);
	if(cantidadSegmentos == 0 && (tamanioMemoria >= tamanioBloque)) return 0;
	if (strcmp(criterioSeleccion, "FF") == 0) {
		struct segmento* segmento = NULL;
		pthread_mutex_lock(&mutexListaSegmentos);
		for(int i = 0; i < listaDeSegmentos->elements_count; i++){
			 segmento = list_get(listaDeSegmentos,i);
			 if(segmento->tipoDeDato == 'V' && segmento->tamanio >= tamanioBloque){
				 resultado 				 = segmento->inicio;
				 if(modificaEspacioVacio){
					 segmento->inicio 		+= tamanioBloque;
					 segmento->tamanio 		-= tamanioBloque;
					 if(segmento->tamanio == 0){
						 list_remove(listaDeSegmentos, i--);
						 free(segmento);
					 }
				 }
				 break;
			 }
		}
		pthread_mutex_unlock(&mutexListaSegmentos);
		return resultado;
	} if (strcmp(criterioSeleccion, "BF") == 0) {
		int inicio 			= 0;
		int indexMejorSegmento = -1;
		struct segmento* mejorSegmento 	= NULL;
		pthread_mutex_lock(&mutexListaSegmentos);
		t_list* segmentosVacios = list_create();
		for(int i = 0; i < listaDeSegmentos->elements_count; i++){
			struct segmento* segmento = list_get(listaDeSegmentos,i);
			if(segmento->tipoDeDato == 'V') list_add(segmentosVacios, segmento);
		}
		for(int i = 0; i < segmentosVacios->elements_count; i++){
			struct segmento* segmentoVacio = list_get(listaDeSegmentos,i);
			if((mejorSegmento == NULL || segmentoVacio->tamanio < mejorSegmento->tamanio) && segmentoVacio->tamanio > tamanioBloque){
				mejorSegmento 		= segmentoVacio;
				indexMejorSegmento 	= i;
			}
		}
		if(mejorSegmento != NULL){
			 resultado 				 = mejorSegmento->inicio;
			 if(modificaEspacioVacio){
				 mejorSegmento->inicio 	+= tamanioBloque;
				 mejorSegmento->tamanio -= tamanioBloque;
				 if(mejorSegmento->tamanio == 0){
					 list_remove(listaDeSegmentos, indexMejorSegmento);
					 free(mejorSegmento);
				 }
			 }
		}
		pthread_mutex_unlock(&mutexListaSegmentos);
		return resultado;
	}
	return -1;
}

int puedeEntrar(int tareas, int cantidadTripulantes){
	pthread_mutex_lock(&mutexListaSegmentos);
	int cantidadDeSegmentos = listaDeSegmentos->elements_count;
	pthread_mutex_unlock(&mutexListaSegmentos);
	if(cantidadDeSegmentos == 1 && (tamanioMemoria >= tareas + TAMANIO_PCB + (TAMANIO_TCB * cantidadTripulantes))) return 0;
	pthread_mutex_lock(&mutexListaSegmentos);
	int tamanios[2 + cantidadTripulantes];
	tamanios[0] = tareas;
	tamanios[1] = TAMANIO_PCB;
	for(int i = 0; i < cantidadTripulantes; i++){
		tamanios[2+i] = TAMANIO_TCB;
	}
	int resultado = -2 - cantidadTripulantes;
	int contadorSegmentosLibres = 0;
	for(int i = 0; i < cantidadDeSegmentos; i++){
		struct segmento* segmento = list_get(listaDeSegmentos,i);
		if(segmento->tipoDeDato == 'V') contadorSegmentosLibres++;
	}
	int espaciosLibres[contadorSegmentosLibres];
	int contador 	= 0;
	int k 			= 0;
	while(k < cantidadDeSegmentos){
		struct segmento* segmento = list_get(listaDeSegmentos,k);
		if(segmento->tipoDeDato == 'V'){
			espaciosLibres[contador] = segmento->tamanio;
			contador++;
		}
		k++;
	}
	int j = 0;
	int i = 0;
	while(i < contadorSegmentosLibres){
		int espacio = espaciosLibres[i];
		if(espacio >= tamanios[j]){
			int espacio2 		= espacio - tamanios[j];
			espaciosLibres[i] 	= espacio2;
			resultado++;
			j++;
			if(j >= 2 + cantidadTripulantes){
				break;
			}
			i = 0;
		} else {
			i++;
		}
	}
	pthread_mutex_unlock(&mutexListaSegmentos);
	return resultado;
}

int generarEstructurasPatotaSegmentacion(struct inicioDePatota* idp){
	t_list* listaTareas 		= idp->listaTareas;
	int pid             		= idp->pid;
	char* aInsertarTareas    	= serializarTareasAux(listaTareas);
	int tamanioTareas        	= strlen(aInsertarTareas) + 1;
	pthread_mutex_lock(&mutexGenerarEstructuras);
	if((tamanioMemoria < tamanioTareas + TAMANIO_PCB + (TAMANIO_TCB * idp->cantTripulantes))){
		pthread_mutex_unlock(&mutexGenerarEstructuras);
		return -1;
	} else if ((puedeEntrar(tamanioTareas,idp->cantTripulantes) < 0)){
		compactar();
		int tamanioTotal = tamanioTareas + TAMANIO_PCB + (TAMANIO_TCB * idp->cantTripulantes);
		int index;
		if(calcularInicioEnMemoriaSegmentacion(tamanioTotal,&index, 0) == -1){
			pthread_mutex_unlock(&mutexGenerarEstructuras);
			return -1;
		}
	}
	int indexPCB = -1;
	int desplazamientoPCB                  = calcularInicioEnMemoriaSegmentacion(TAMANIO_PCB, &indexPCB, 1);
	struct PCB* processControlBlock        = malloc(sizeof(struct PCB));
	processControlBlock->PID               = pid;
	processControlBlock->direccPrimerTarea = -1; //OBTENER LA PRIMER TAREA
	void* aInsertarPCB                     = serializarPCB(processControlBlock);

	struct segmento* segmentoPCB  = malloc(sizeof(struct segmento));
	segmentoPCB->idSegmento       = 0;
	segmentoPCB->inicio           = desplazamientoPCB;
	segmentoPCB->tamanio          = TAMANIO_PCB;
	segmentoPCB->tipoDeDato       = 'P';
	pthread_mutex_lock(&mutexListaSegmentos);
	if(indexPCB == -1){
		list_add(listaDeSegmentos,segmentoPCB);
	}else list_add_in_index(listaDeSegmentos,indexPCB,segmentoPCB);
	pthread_mutex_unlock(&mutexListaSegmentos);

	insertarAMemoria(aInsertarPCB, &desplazamientoPCB, 0, TAMANIO_PCB, 'P');

	//INSERTAR LISTA DE TAREAS EN RAM
	int indexTareas 			= -1;
	int desplazamientoTareas 	= calcularInicioEnMemoriaSegmentacion(tamanioTareas,&indexTareas, 1);

	struct segmento* segmentoTareas  = malloc(sizeof(struct segmento));
	segmentoTareas->idSegmento       = 1;
	segmentoTareas->inicio           = desplazamientoTareas;
	segmentoTareas->tamanio          = tamanioTareas;
	segmentoTareas->tipoDeDato       = 'X';
	pthread_mutex_lock(&mutexListaSegmentos);
	if(indexTareas == -1){
		list_add(listaDeSegmentos,segmentoTareas);
	} else list_add_in_index(listaDeSegmentos,indexTareas,segmentoTareas);
	pthread_mutex_unlock(&mutexListaSegmentos);
	insertarAMemoria(aInsertarTareas, &desplazamientoTareas, 0, tamanioTareas, 'X');

	crearTablaSegmentosPatota(pid, segmentoPCB,segmentoTareas);
	processControlBlock->direccPrimerTarea = segmentoTareas->inicio;
	actualizarPCB(processControlBlock);
	free(processControlBlock);

	pthread_mutex_unlock(&mutexGenerarEstructuras);

	return 0;
}

void generarEstructurasTripulanteSegmentacion(int posX, int posY, int tid, int pid){
	struct tablaSegmentosPatota* tablaPatota = buscarTablaSegmentos(pid);

	struct TCB* threadControlBlock         = malloc(sizeof(struct TCB));
	threadControlBlock->TID                = tid;
	threadControlBlock->Estado             = 'N';
	threadControlBlock->PosicionX          = posX;
	threadControlBlock->PosicionY          = posY;
	threadControlBlock->ProximaInstruccion = 0;
	threadControlBlock->DireccionPCB       = tablaPatota->pcb->inicio;
	void* aInsertarTCB                     = serializarTCB(threadControlBlock);
	int indexTCB 						   = -1;
	pthread_mutex_lock(&mutexGenerarEstructuras);
	int desplazamientoTCB                  = calcularInicioEnMemoriaSegmentacion(TAMANIO_TCB,&indexTCB, 1);

	agregarATablaSegmentosPatota(tid, desplazamientoTCB, tablaPatota, indexTCB);
	insertarAMemoria(aInsertarTCB, &desplazamientoTCB, 0, TAMANIO_TCB, 'T');
	pthread_mutex_unlock(&mutexGenerarEstructuras);
	free(threadControlBlock);
}

void crearTablaSegmentosPatota(int pid, struct segmento* segmentoPCB, struct segmento* segmentoTareas){

	struct tablaSegmentosPatota* tablaPatota = malloc(sizeof(struct tablaSegmentosPatota));
	tablaPatota->pid          = pid;
	tablaPatota->pcb          = segmentoPCB;
	tablaPatota->tareas       = segmentoTareas;
	tablaPatota->segmentosTCB = list_create();
	sem_t semaforoTablaPatota;
	sem_init(&semaforoTablaPatota,0,1);
	tablaPatota->semaforoSegmentosTCB = semaforoTablaPatota;

	//REGION_CRITICA
	pthread_mutex_lock(&mutexListaTablasPatotas);
	list_add(tablasPatotas, tablaPatota);
	pthread_mutex_unlock(&mutexListaTablasPatotas);
	//REGION_CRITICA
}

void agregarATablaSegmentosPatota(int tid, int desplazamiento, struct tablaSegmentosPatota* tablaPatota, int index){
	sem_wait(&(tablaPatota->semaforoSegmentosTCB));
	t_list* segmentosTCB          = tablaPatota->segmentosTCB;
	struct segmento* segmentoTCB  = malloc(sizeof(struct segmento));
	segmentoTCB->idSegmento       = 2 + segmentosTCB->elements_count;
	segmentoTCB->inicio           = desplazamiento;
	segmentoTCB->tamanio          = TAMANIO_TCB;
	segmentoTCB->tipoDeDato       = 'T';

	list_add(segmentosTCB, segmentoTCB);
	sem_post(&(tablaPatota->semaforoSegmentosTCB));

	pthread_mutex_lock(&mutexListaSegmentos);
	if(index == -1){
		list_add(listaDeSegmentos,segmentoTCB);
	} else list_add_in_index( listaDeSegmentos, index, segmentoTCB);
	pthread_mutex_unlock(&mutexListaSegmentos);
}

void compactar(){
	pthread_mutex_lock(&mutexListaSegmentos);
	for(int i = 0; i < listaDeSegmentos->elements_count; i++){
		struct segmento* segmento = list_get(listaDeSegmentos, i);
		if(segmento->tipoDeDato == 'V'){
			list_remove(listaDeSegmentos, i);
			free(segmento);
			i--;
		}
	}
	int inicio = 0;
	for(int i = 0; i < listaDeSegmentos->elements_count; i++){
		struct segmento* segmento = list_get(listaDeSegmentos, i);
		if(segmento->inicio > inicio){
			moverSegmento(segmento, inicio);
		}
		inicio += segmento->tamanio;
	}
	struct segmento* segmentoVacio 	= malloc(sizeof(struct segmento));
	segmentoVacio->idSegmento 		= -1;
	if(listaDeSegmentos->elements_count == 0){
		segmentoVacio->inicio 			= 0;
	} else {
		struct segmento* ultimoSegmento = list_get(listaDeSegmentos, listaDeSegmentos->elements_count - 1);
		segmentoVacio->inicio 			= ultimoSegmento->inicio + ultimoSegmento->tamanio;
	}
	segmentoVacio->tamanio 			= tamanioMemoria - segmentoVacio->inicio;
	segmentoVacio->tipoDeDato 		= 'V';
	list_add(listaDeSegmentos, segmentoVacio);

	pthread_mutex_unlock(&mutexListaSegmentos);
}

void moverSegmento(struct segmento* segmento, int nuevoInicio){
	int inicio 			= segmento->inicio;
	void* aMover 		= obtenerDeMemoria(&(inicio), segmento->tamanio, 0);
	segmento->inicio 	= nuevoInicio;
	int nuevoInicioAux	= nuevoInicio;
	insertarAMemoria(aMover, &nuevoInicioAux, 0, segmento->tamanio, segmento->tipoDeDato);
	if(segmento->tipoDeDato == 'P') avisarATripulantesSegmentacion(segmento);
	else if(segmento->tipoDeDato == 'X'){
		avisarAPCBSegmentacion(segmento);
	}
}

void avisarAPCBSegmentacion(struct segmento* segmento){
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaSegmentosPatota* tablaPatota = list_get(tablasPatotas,i);
		if(tablaPatota->tareas == segmento){
			struct PCB* pcb 		= buscarPCB(tablaPatota->pid);
			pcb->direccPrimerTarea 	= segmento->inicio;
			actualizarPCB(pcb);
			free(pcb);
			pcb = buscarPCB(tablaPatota->pid);
		}
	}
}

void avisarATripulantesSegmentacion( struct segmento* segmento ){
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaSegmentosPatota* tablaPatota = list_get(tablasPatotas,i);
		if(tablaPatota->pcb == segmento){
		sem_wait(&(tablaPatota->semaforoSegmentosTCB));
		for(int j = 0; j < tablaPatota->segmentosTCB->elements_count; j++){
				struct segmento* segmentoTCB 	= list_get(tablaPatota->segmentosTCB,j);
				int desplazamiento 				= segmentoTCB->inicio;
				void* data 						= obtenerDeMemoria(&desplazamiento,segmentoTCB->tamanio,0);
				struct TCB* tcb 				= deserializarTCB(data);
				free(data);
				tcb->DireccionPCB 				= segmento->inicio;
				actualizarTCB(tcb,'K',-1,-1,-1);
				free(tcb);
		}
		sem_post(&(tablaPatota->semaforoSegmentosTCB));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * PAGINACION
 */

void inicializarTablaPaginas(){
	tablasPatotas = list_create();

	listaDeFrames = list_create();
	for(int i = 0; i < tamanioMemoria/tamanioPagina; i++){
		struct pagina* pagina 	= malloc(sizeof(struct pagina));
		pagina->numeroFrame   	= i;
		pagina->numeroPagina  	= -1;
		pagina->infoFrame     	= list_create();
		pagina->instante		= -1;
		pagina->clock			= -1;
		pagina->presencia		= 0;
		pagina->bitDeUso		= 0;
		list_add(listaDeFrames, pagina);
	}
}

void liberarTablaPaginas(){
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaPaginasPatota* tablaPatota = list_remove(tablasPatotas, i);
		for(int j = 0; j < tablaPatota->paginas->elements_count; j++){
			struct pagina* pagina = list_remove(tablaPatota->paginas, j);
			for(int k = 0; k < pagina->infoFrame->elements_count; k++){
				struct infoFrame* info = list_remove(pagina->infoFrame, k);
				free(info);
			}
			list_destroy(pagina->infoFrame);
			sem_destroy((&pagina->refrescarPagina));
			sem_destroy((&pagina->semaforoInfoFrame));
			free(pagina);
		}
		list_destroy(tablaPatota->paginas);
		free(tablaPatota);
	}
	list_destroy(tablasPatotas);
}

int estaLibre(struct pagina* pagina){
	return pagina->infoFrame->elements_count == 0;
}

int cantidadDeFramesLibres(){
	int contador = 0;
	for(int i = 0; i < listaDeFrames->elements_count; i++){
		pthread_mutex_lock(&mutexListaFrames);
		struct pagina* pagina = list_get(listaDeFrames, i);
		if(estaLibre(pagina)){
			contador++;
		}
		pthread_mutex_unlock(&mutexListaFrames);
	}
	return contador;
}

int calcularInicoEnMemoriaPaginacion(int tamanio){
	if((cantidadDeFramesLibres() * tamanioPagina) + tamanioDisponibleEnSwap() > tamanio){
		return proximoFrameLibre();
	} else {
		return -1;
	}
}

int proximoFrameLibre(){
	int numeroFrameLibre = -1;
	for(int i = 0; i < listaDeFrames->elements_count; i++){
		pthread_mutex_lock(&mutexListaFrames);
		struct pagina* pagina = list_get(listaDeFrames, i);
		if(estaLibre(pagina) && numeroFrameLibre == -1){
			numeroFrameLibre = pagina->numeroFrame;
		}
		pthread_mutex_unlock(&mutexListaFrames);
	}
	if(numeroFrameLibre != -1) {
		return numeroFrameLibre;
	}
	else{
		int frameLibre = liberarFrame();
		return frameLibre;
	}
}

struct tablaPaginasPatota* crearTablaPaginasPatota(int pid, int numeroFrame){
	struct tablaPaginasPatota* tablaPatota 	= malloc(sizeof(struct tablaPaginasPatota));
	tablaPatota->pid                       	= pid;
	tablaPatota->paginas                   	= list_create();
	sem_t semaforoTablaPatota;
	sem_init(&semaforoTablaPatota,0,1);
	tablaPatota->semaforoPaginas 			= semaforoTablaPatota;

	pthread_mutex_lock(&mutexListaFrames);
	struct pagina* pagina 		= list_get(listaDeFrames, numeroFrame);
	pagina->numeroPagina  		= 0;
	pagina->presencia			= 1;
	sem_t semaforoInfoFrame;
	sem_init(&semaforoInfoFrame,0,1);
	pagina->semaforoInfoFrame	= semaforoInfoFrame;
	sem_t semRefrescarPagina;
	sem_init(&semRefrescarPagina,0,1);
	pagina->refrescarPagina	= semRefrescarPagina;
	refrescarPagina(pagina);
	pthread_mutex_unlock(&mutexListaFrames);

	sem_wait(&(tablaPatota->semaforoPaginas));
	list_add(tablaPatota->paginas, pagina);
	sem_post(&(tablaPatota->semaforoPaginas));

	pthread_mutex_lock(&mutexListaTablasPatotas);
	list_add(tablasPatotas, tablaPatota);
	pthread_mutex_unlock(&mutexListaTablasPatotas);

	return tablaPatota;
}

int generarEstructurasPatotaPaginacion(struct inicioDePatota* idp){
	t_list* listaTareas = idp->listaTareas;
	int pid             = idp->pid;

	int tamanioTotal          = tamanioEnMemoriaPatota(idp);
	int primerFrameDisponible;
	pthread_mutex_lock(&mutexGenerarEstructuras);
	if((primerFrameDisponible = calcularInicoEnMemoriaPaginacion(tamanioTotal)) == -1){
		pthread_mutex_unlock(&mutexGenerarEstructuras);
		return -1;
	}

	int desplazamientoPCB = primerFrameDisponible * tamanioPagina;

	struct PCB* processControlBlock        = malloc(sizeof(struct PCB));
	processControlBlock->PID               = pid;
	processControlBlock->direccPrimerTarea = -1; //OBTENER LA PRIMER TAREA

	crearTablaPaginasPatota(pid, primerFrameDisponible);

	void* aInsertarPCB       = serializarPCB(processControlBlock);
	insertarAMemoriaConPaginacion(pid, TAMANIO_PCB, desplazamientoPCB, primerFrameDisponible, aInsertarPCB, 'P', 0, -1);


	for(int i = 0; i < listaTareas->elements_count; i++){
		struct pagina* pagina    	= ultimaPagina(pid);
		int desplazamientoTarea 	= ((pagina->numeroFrame+1) * tamanioPagina) - tamanioDisponible(pagina->numeroFrame);
		char* aInsertarTarea 	 	= serializarTareaAux(listaTareas, i);
		int tamanioAInsertar     	= strlen(aInsertarTarea) + 1;
		insertarAMemoriaConPaginacion(pid, tamanioAInsertar, desplazamientoTarea, pagina->numeroFrame, aInsertarTarea, 'X', 0, i);
		if(i == 0){
			processControlBlock->direccPrimerTarea = desplazamientoTarea;
			actualizarPCB(processControlBlock);
		}
	}
	pthread_mutex_unlock(&mutexGenerarEstructuras);


	free(processControlBlock);
	return 0;
}

struct PCB* buscarPCB(int pid){
	struct TCB* tcb = NULL;
	if(strcmp(esquemaMemoria, "SEGMENTACION") == 0){
		tcb = buscarPCBEnTablaSegmentos(pid);
	} else {
		tcb = buscarPCBEnTablaPaginas(pid);
	}
	//pthread_mutex_unlock(&mutexBuscar);
	return tcb;
}

struct PCB* buscarPCBEnTablaSegmentos(int pid){
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaSegmentosPatota* tablaPatota = list_get(tablasPatotas, i);
		if(tablaPatota->pid == pid){
			struct segmento* segmentoPCB = tablaPatota->pcb;
			int inicio                   = segmentoPCB->inicio;
			void* data                   = obtenerDeMemoria(&(inicio), segmentoPCB->tamanio, 0);
			struct PCB* pcb              = deserializarPCB(data);
			free(data);
			return pcb;
		}
	}
	return NULL;
}

struct PCB* buscarPCBEnTablaPaginas(int pid){
	int desplazamiento = 0;
	void* pcbMagic = malloc(TAMANIO_PCB);
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaPaginasPatota* tablaPatota = list_get(tablasPatotas,i);
		if(tablaPatota->pid == pid){
			struct pagina* pag 		= list_get(tablaPatota->paginas,0);
			if(pag->presencia == 0) traerAMemoria(pag);
			else refrescarPagina(pag);
			struct infoFrame* info 	= list_get(pag->infoFrame,0);
			int inicio = info->inicio;
			pcbMagic 				= obtenerDeMemoria(&inicio,info->tamanio,0);
		}
	}
	struct PCB* pcb          = deserializarPCB(pcbMagic);
	free(pcbMagic);
	return pcb;
}

void actualizarPCB(struct PCB* pcb){
	if(strcmp(esquemaMemoria,"SEGMENTACION") == 0){
		actualizarPCBSegmentacion(pcb);
	} else if(strcmp(esquemaMemoria, "PAGINACION") == 0){
		actualizarPCBPaginacion(pcb);
	}
}

void actualizarPCBPaginacion(struct PCB* pcb){
	void* magic = serializarPCB(pcb);
	void* magicAux = malloc(TAMANIO_PCB);
	memcpy(magicAux, magic, TAMANIO_PCB);
	int desplazamiento = 0;
	struct tablaPaginasPatota* tablaPatota 	= buscarTablaPaginas(pcb->PID);
	t_list* paginas                        	= tablaPatota->paginas;
	for(int i = 0; i < paginas->elements_count; i++){
		struct pagina* pagina = list_get(paginas, i);
		for(int j = 0; j < pagina->infoFrame->elements_count; j++){
			struct infoFrame* info = list_get(pagina->infoFrame, j);
			if(info->tipoDeDato == 'P'){
				if(pagina->presencia == 0) traerAMemoria(pagina);
				else {
					sem_wait(&(pagina->refrescarPagina));
					refrescarPagina(pagina);
					sem_post(&(pagina->refrescarPagina));
				}
				int inicio = pagina->numeroFrame * tamanioPagina + info->inicio;
				insertarAMemoria(magic,&inicio,info->tamanio,info->tamanio,'P');
				desplazamiento += info->tamanio;
				if(TAMANIO_PCB - desplazamiento > 0){
					magic = malloc(TAMANIO_PCB - desplazamiento);
					memcpy(magic, magicAux + desplazamiento, TAMANIO_PCB - desplazamiento);
				}
			}
		}
	}
	free(magicAux);
}

void actualizarPCBSegmentacion(struct PCB* pcb){
	struct segmento* seg 	= (struct segmento*)buscarSegmentoPCB(pcb->PID);
	void* magic 			= serializarPCB(pcb);
	int inicio 				= seg->inicio;
	insertarAMemoria(magic,&inicio,0,seg->tamanio,'P');
}

struct segmento* buscarSegmentoPCB(int pid){
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaSegmentosPatota* tablaPatota = list_get(tablasPatotas,i);
		if(tablaPatota->pid == pid){
			return tablaPatota->pcb;
		}
	}
	return NULL;
}

struct pagina* ultimaPagina(int pid){
	struct tablaPaginasPatota* tablaPatota 	= buscarTablaPaginas(pid);
	int cantPaginas                        	= tablaPatota->paginas->elements_count;

	sem_wait(&(tablaPatota->semaforoPaginas));
	struct pagina* ultimaPag 				= list_get(tablaPatota->paginas, cantPaginas-1);
	sem_post(&(tablaPatota->semaforoPaginas));

	int contador 							= tamanioPagina;
	for(int i = 0; i < ultimaPag->infoFrame->elements_count; i++){
		struct infoFrame* infoFrame = list_get(ultimaPag->infoFrame, i);
		contador -= infoFrame->tamanio;
	}
	if(contador == 0){
		int numeroFrame = proximoFrameLibre();
		agregarPagina(pid, numeroFrame);
		ultimaPag 		= list_get(tablaPatota->paginas, cantPaginas);
	}
	return ultimaPag;
}

void generarEstructurasTripulantePaginacion(int posX, int posY, int tid, int pid){
	struct TCB* threadControlBlock         	= malloc(sizeof(struct TCB));
	threadControlBlock->TID                	= tid;
	threadControlBlock->Estado             	= 'N';
	threadControlBlock->PosicionX          	= posX;
	threadControlBlock->PosicionY          	= posY;
	threadControlBlock->ProximaInstruccion 	= 0;
	struct tablaPaginasPatota* tablaPatota 	= buscarTablaPaginas(pid);
	struct pagina* primerPagina            	= list_get(tablaPatota->paginas,0);
	if(primerPagina->presencia == 0){
		threadControlBlock->DireccionPCB       	= -1;
	} else {
		threadControlBlock->DireccionPCB       	= primerPagina->numeroFrame * tamanioPagina;
	}

	pthread_mutex_lock(&mutexGenerarEstructuras);
	struct pagina* pagina = ultimaPagina(pid);
	if(pagina->presencia == 0) traerAMemoria(pagina);
	else {
		sem_wait(&(pagina->refrescarPagina));
		refrescarPagina(pagina);
		sem_post(&(pagina->refrescarPagina));
	}
	int desplazamientoTCB = ((pagina->numeroFrame+1) * tamanioPagina) - tamanioDisponible(pagina->numeroFrame);
	void* aInsertarTCB       = serializarTCB(threadControlBlock);
	insertarAMemoriaConPaginacion(pid, TAMANIO_TCB, desplazamientoTCB, pagina->numeroFrame, aInsertarTCB, 'T', tid, -1);
	pthread_mutex_unlock(&mutexGenerarEstructuras);
	free(threadControlBlock);
}

void agregarPagina(int pid, int numeroFrame){
	struct tablaPaginasPatota* tablaPatota = buscarTablaPaginas(pid);
	pthread_mutex_lock(&mutexListaFrames);
	struct pagina* nuevaPagina 		= list_get(listaDeFrames, numeroFrame);
	nuevaPagina->numeroPagina  		= tablaPatota->paginas->elements_count;
	refrescarPagina(nuevaPagina);
	nuevaPagina->presencia			= 1;
	sem_t semaforoTablaPatota;
	sem_init(&semaforoTablaPatota,0,1);
	sem_t refrescarPagina;
	sem_init(&refrescarPagina,0,1);
	nuevaPagina->refrescarPagina	= refrescarPagina;
	nuevaPagina->semaforoInfoFrame 	= semaforoTablaPatota;
	pthread_mutex_unlock(&mutexListaFrames);

	sem_wait(&(tablaPatota->semaforoPaginas));
	list_add(tablaPatota->paginas, nuevaPagina);
	sem_post(&(tablaPatota->semaforoPaginas));
}

int tamanioDisponible(int numeroFrame) {
	if(numeroFrame == -1){
		return 0;
	}
	int contador = tamanioPagina;
	struct pagina* pagina 	= list_get(listaDeFrames, numeroFrame);
	for(int i = 0; i < pagina->infoFrame->elements_count; i++){
		struct infoFrame* infoFrame = list_get(pagina->infoFrame, i);
		contador -= infoFrame->tamanio;
	}
	return contador;
}


void imprimirTablaPaginas(FILE* archivo, t_list* listaAImprimir, int esSwap){
	for(int i = 0; i < listaAImprimir->elements_count; i++){
		int pid = 0;
		pthread_mutex_lock(&mutexListaFrames);
		struct pagina* pagina 	= list_get(listaAImprimir,i);
		if(pagina->numeroPagina != -1){
			pid               = paginaPid(pagina);
		}
		if(estaLibre(pagina) && !esSwap){
			imprimirMarcoVacio(i, archivo);
		} else {
			imprimirMarcoOcupado(pagina, pid, archivo);
		}
		pthread_mutex_unlock(&mutexListaFrames);
	}
}

void imprimirMarcoOcupado(struct pagina* pagina, int pid, FILE* archivo){
	fprintf(archivo, "Marco: %d\tEstado: Ocupado\tProceso: %d\tPagina: %d\tLibre: %db\tInstante: %d\tClock: %d\tInfoMarco: [",pagina->numeroFrame, pid, pagina->numeroPagina, tamanioDisponible(pagina->numeroFrame), pagina->instante, pagina->clock);
	for(int i = 0; i < pagina->infoFrame->elements_count - 1; i++){
		struct infoFrame* info = list_get(pagina->infoFrame, i);
		fprintf(archivo, "%c: %db, ", info->tipoDeDato, info->tamanio);
	}
	struct infoFrame* info = list_get(pagina->infoFrame, pagina->infoFrame->elements_count - 1);
	fprintf(archivo, "%c: %db", info->tipoDeDato, info->tamanio);
	fprintf(archivo, "]\n");
}

void imprimirMarcoVacio(int numeroFrame, FILE* archivo){
	int tamanio = 0;
	if(numeroFrame != -1) tamanio = tamanioDisponible(numeroFrame);
	fprintf(archivo, "Marco: %d\tEstado: Libre\tProceso: -\tPagina: -\tLibre: %db\tInstante: -\tClock: -\tInfoMarco: -\n", numeroFrame, tamanio);
}

struct tablaPaginasPatota* buscarTablaPaginas(int pid){
	struct tablaPaginasPatota* resultado = NULL;
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaPaginasPatota* tablaPatota = (struct tablaPaginasPatota*)list_get(tablasPatotas, i);
		if(tablaPatota->pid == pid){
			resultado = tablaPatota;
		}
	}
	return resultado;
}

int paginaPid(struct pagina* pagina){
	int resultado = 0;
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaPaginasPatota* tablaActual = list_get(tablasPatotas, i);
		for(int j = 0; j < tablaActual->paginas->elements_count; j++){
			struct pagina* paginaAux = list_get(tablaActual->paginas, j);
			if(paginaAux == pagina){
				resultado = tablaActual->pid;
			}
		}
	}
	return resultado;
}

void borrarInfoFrame(int tid){
	struct TCB* tcb                        = buscarTCBEnTablaPaginas(tid);
	int inicioPCB                          = tcb->DireccionPCB;
	void* data                             = obtenerDeMemoria(&inicioPCB, TAMANIO_PCB, 0);
	struct PCB* pcb                        = deserializarPCB(data);
	free(data);
	struct tablaPaginasPatota* tablaPatota = buscarTablaPaginas(pcb->PID);

	for(int i = 0; i < tablaPatota->paginas->elements_count; i++){
		struct pagina* pagina = list_get(tablaPatota->paginas, i);
		for(int j = 0; j < pagina->infoFrame->elements_count; j++){
			struct infoFrame* info = list_get(pagina->infoFrame, j);
			if(info->tid == tid){
				struct infoFrame* info = list_remove(pagina->infoFrame, j--);
				free(info);
				if(estaLibre(pagina)){
					pagina->numeroPagina = -1;
					sem_wait(&(tablaPatota->semaforoPaginas));
					list_remove(tablaPatota->paginas, i--);
					//sem_destroy(&(pagina->refrescarPagina));
					//sem_destroy(&(pagina->semaforoInfoFrame));
					//free(pagina);
					sem_post(&(tablaPatota->semaforoPaginas));
				}
			}
		}
	}
	if(!hayMasTripulantes(tablaPatota)){
		int cantidadPaginas = tablaPatota->paginas->elements_count;
		for(int i = 0; i < cantidadPaginas; i++){
			sem_wait(&(tablaPatota->semaforoPaginas));
			struct pagina* pagina = list_remove(tablaPatota->paginas, 0);
			sem_post(&(tablaPatota->semaforoPaginas));
			int cantidadInfoFrames = pagina->infoFrame->elements_count;
			for(int j = 0; j < cantidadInfoFrames; j++){
				struct infoFrame* info = list_remove(pagina->infoFrame, 0);
				free(info);
			}
			//pagina->numeroPagina = -1;
			//sem_destroy(&(pagina->refrescarPagina));
			//sem_destroy(&(pagina->semaforoInfoFrame));
			//free(pagina);
		}
		list_destroy(tablaPatota->paginas);
		for(int i = 0; i < tablasPatotas->elements_count; i++){
			struct tablaPaginasPatota* tablaPatota = list_get(tablasPatotas, i);
			if(tablaPatota->pid == pcb->PID){
				pthread_mutex_lock(&mutexListaTablasPatotas);
				list_remove(tablasPatotas, i);
				pthread_mutex_unlock(&mutexListaTablasPatotas);
			}
		}
		sem_destroy(&(tablaPatota->semaforoPaginas));
		free(tablaPatota);
	}
	free(pcb);
	free(tcb);
}

int hayMasTripulantes(struct tablaPaginasPatota* tablaPatota){
	for(int i = 0; i < tablaPatota->paginas->elements_count; i++){
		struct pagina* pagina = list_get(tablaPatota->paginas, i);
		for(int j = 0; j < pagina->infoFrame->elements_count; j++){
			struct infoFrame* info = list_get(pagina->infoFrame, j);
			if(info->tipoDeDato == 'T') return 1;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * MEMORIA VIRTUAL
 */

void inicializarSwap(){
	FILE* swapdump = fopen("swap.txt", "w");
	if(swapdump != NULL) fclose(swapdump);
	FILE* swap = fopen(pathSwap, "w");
	truncate(pathSwap,tamanioSwap);
	fclose(swap);
	listaPaginasSwap = list_create();
}

void liberarSwap(){
	while(listaPaginasSwap->elements_count != 0){
		struct pagina* pagina = list_remove(listaPaginasSwap, 0);
		for(int k = 0; k < pagina->infoFrame->elements_count; k++){
			struct infoFrame* info = list_remove(pagina->infoFrame, k);
			free(info);
		}
		list_destroy(pagina->infoFrame);
		sem_destroy((&pagina->refrescarPagina));
		sem_destroy((&pagina->semaforoInfoFrame));
		free(pagina);
	}
	list_destroy(listaPaginasSwap);
}

void* eliminarPaginaSwap(int indice){
	void* datosPagina 	= malloc(tamanioPagina);
	FILE* swap 			= fopen(pathSwap, "r+");
	fseek(swap, indice * tamanioPagina, SEEK_SET);
	fread(datosPagina, tamanioPagina, 1, swap);
	pthread_mutex_lock(&mutexListaPaginasSwap);
	int paginasRestantes = listaPaginasSwap->elements_count - indice;
	pthread_mutex_unlock(&mutexListaPaginasSwap);
	if(paginasRestantes > 0){
		fseek(swap, (indice+1) * tamanioPagina, SEEK_SET);
		void* datosPaginasRestantes = malloc(paginasRestantes * tamanioPagina);
		fread(datosPaginasRestantes, paginasRestantes * tamanioPagina, 1, swap);
		fseek(swap, indice*tamanioPagina, SEEK_SET);
		fwrite(datosPaginasRestantes, paginasRestantes * tamanioPagina, 1, swap);
		free(datosPaginasRestantes);
	}
	fclose(swap);
	return datosPagina;
}

void traerAMemoria(struct pagina* paginaNecesitada){
	int numeroFrame 	= reemplazarPaginaPor(paginaNecesitada);

	int pid 			= tienePCB(paginaNecesitada);
	//int primerTarea 	= tienePrimerTarea(paginaNecesitada);
	//if(primerTarea) avisarAPCBPaginacion(primerTarea,calcularInicioTarea(paginaNecesitada));
	if(pid) avisarATripulantesPaginacion(pid, numeroFrame * tamanioPagina);

}

int calcularInicioTarea(struct pagina* pagina){
	for(int i = 0; i < pagina->infoFrame->elements_count; i++){
		struct infoFrame* info = list_get(pagina->infoFrame,i);
		if(info->tipoDeDato == 'X') return (info->inicio + (pagina->numeroFrame* tamanioPagina));
	}
	return -1;
}

void refrescarPagina(struct pagina* pagina){
	pagina->clock		= 1;
	pagina->instante 	= clock();
}

void agregarPaginaASwap(struct pagina* pagina){
	int desplazamiento 	= pagina->numeroFrame * tamanioPagina;
	void* datosPagina 	= obtenerDeMemoria(&desplazamiento, tamanioPagina, 0);
	pagina->numeroFrame = -1;
	FILE* swap 			= fopen(pathSwap, "r+");

	pthread_mutex_lock(&mutexListaPaginasSwap);
	fseek(swap, listaPaginasSwap->elements_count * tamanioPagina, SEEK_SET);
	pthread_mutex_unlock(&mutexListaPaginasSwap);

	fwrite(datosPagina, tamanioPagina, 1, swap);
	fclose(swap);
	free(datosPagina);

	pthread_mutex_lock(&mutexListaPaginasSwap);
	list_add(listaPaginasSwap,pagina);
	pthread_mutex_unlock(&mutexListaPaginasSwap);
}
int liberarFrame(){
	return reemplazarPaginaPor(NULL);
}

int reemplazarPaginaPor(struct pagina* nuevaPagina){
	pthread_mutex_lock(&mutexSwap);
	struct pagina* paginaAReemplazar = NULL;
	if(strcmp(algoritmoReemplazo, "LRU") == 0){
		paginaAReemplazar = reemplazarPaginaLRU();
	}
	if(strcmp(algoritmoReemplazo, "CLOCK") == 0){
		paginaAReemplazar = reemplazarPaginaClock();
	}
	int numeroFrame 				= paginaAReemplazar->numeroFrame;

	int indice = -1;
	paginaAReemplazar->presencia	= 0;
	int pid 			= tienePCB(paginaAReemplazar);
	int primerTarea 	= tienePrimerTarea(paginaAReemplazar);
	agregarPaginaASwap(paginaAReemplazar);
	if(nuevaPagina != NULL){
		for(int i = 0; i < listaPaginasSwap->elements_count; i++){
			struct pagina* pagina = list_get(listaPaginasSwap, i);
			if(pagina == nuevaPagina){
				pthread_mutex_lock(&mutexListaPaginasSwap);
				nuevaPagina = list_remove(listaPaginasSwap, i);
				pthread_mutex_unlock(&mutexListaPaginasSwap);
				indice = i;
				break;
			}
		}
		void* datosPagina 	= eliminarPaginaSwap(indice);
		pthread_mutex_lock(&mutexBaseMemoria);
		memcpy(baseMemoria + (numeroFrame*tamanioPagina), datosPagina, tamanioPagina);
		pthread_mutex_unlock(&mutexBaseMemoria);
		free(datosPagina);
		sem_wait(&(nuevaPagina->refrescarPagina));
		refrescarPagina(nuevaPagina);
		sem_post(&(nuevaPagina->refrescarPagina));
		nuevaPagina->numeroFrame		= numeroFrame;
		nuevaPagina->presencia			= 1;
		pthread_mutex_lock(&mutexListaFrames);
		list_replace(listaDeFrames,numeroFrame,nuevaPagina);
		pthread_mutex_unlock(&mutexListaFrames);
	} else {
		struct pagina* nuevaPaginaVacia = malloc(sizeof(struct pagina));
		nuevaPaginaVacia->numeroFrame	= numeroFrame;
		nuevaPaginaVacia->presencia		= 1;
		nuevaPaginaVacia->infoFrame		= list_create();
		nuevaPaginaVacia->numeroPagina 	= -1;
		nuevaPaginaVacia->clock			= -1;
		nuevaPaginaVacia->instante		= -1;
		nuevaPaginaVacia->bitDeUso      = 1;
		pthread_mutex_lock(&mutexListaFrames);
		list_replace(listaDeFrames, numeroFrame, nuevaPaginaVacia);
		pthread_mutex_unlock(&mutexListaFrames);
	}
	imprimirTabla("Cambio de Pagina");
	pthread_mutex_unlock(&mutexSwap);

	//if(primerTarea) avisarAPCBPaginacion(primerTarea, -1);
	if(pid) {
		avisarATripulantesPaginacion(pid, -1);
		//if(nuevaPaginaVacia->numeroFrame != numeroFrame);
	}
	struct pagina* paginaADevolver = list_get(listaDeFrames,numeroFrame);
	paginaADevolver->bitDeUso = 0;
	return numeroFrame;
}

void avisarAPCBPaginacion(int pid, int direcTarea){
	struct PCB* pcb = buscarPCB(pid);
	pcb->direccPrimerTarea = direcTarea;
	actualizarPCB(pcb);
	free(pcb);
}

int tieneTripulantes(int pid){
	struct tablaPaginasPatota* tablaPatotas = buscarTablaPaginas(pid);
	for(int i = 0; i < tablaPatotas->paginas->elements_count; i++){
		struct pagina* pag = list_get(tablaPatotas->paginas,i);
		for(int j = 0; j < pag->infoFrame->elements_count; j++){
			struct infoFrame* info = list_get(pag->infoFrame,j);
			if(info->tipoDeDato == 'T') return 1;
		}
	}
	return 0;
}

void avisarATripulantesPaginacion(int pid, int direccionPCBNueva){
	t_list* listaTCB 	= obtenerTCBsPaginacion(pid);
	for( int i = 0; i < listaTCB->elements_count; i++ ){
		struct TCB* tcb = list_get(listaTCB,i);
		tcb->DireccionPCB = direccionPCBNueva;
		actualizarTCBPaginacion(tcb, pid, 'K', -1, -1, -1);
	}
	while(listaTCB->elements_count != 0){
		struct TCB* tcb = list_remove(listaTCB, 0);
		free(tcb);
	}
	list_destroy(listaTCB);
}

int tienePrimerTarea(struct pagina* paginaAVerificar){
	int salir = 0;
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaPaginasPatota* tablaPatota = list_get(tablasPatotas,i);
		for(int j = 0; j < tablaPatota->paginas->elements_count; j++){
			struct pagina* pag = list_get(tablaPatota->paginas,j);
			for(int k = 0; k < pag->infoFrame->elements_count; k++){
				struct infoFrame* info = list_get(pag->infoFrame,k);
				if(info->tipoDeDato == 'X'){
					if(pag == paginaAVerificar){
						return tablaPatota->pid;
					}else {
						salir = 1;
						break;
					}
				}
			}
			if(salir){
				salir = 0;
				break;
			}
		}
	}
	return 0;
}

int tienePCB(struct pagina* paginaAVerificar){
	if(paginaAVerificar->infoFrame->elements_count == 0) return 0;
	struct infoFrame* info = list_get(paginaAVerificar->infoFrame,0);
	if(info->tipoDeDato == 'P'){
		int inicio 		= paginaAVerificar->numeroFrame * tamanioPagina;
		int* ptrTiene 	= (int*)obtenerDeMemoria(&inicio,sizeof(int), 0);
		int tiene 		= *ptrTiene;
		free(ptrTiene);
		return tiene;
	}
	return 0;
}

struct pagina* reemplazarPaginaLRU(){
	struct pagina* paginaAReemplazar 	= NULL;
	pthread_mutex_lock(&mutexListaFrames);
	while(paginaAReemplazar == NULL){
		for(int i = 0; i < listaDeFrames->elements_count; i++){
			struct pagina* pagina = list_get(listaDeFrames,i);
			if((paginaAReemplazar == NULL || pagina->instante < paginaAReemplazar->instante) && pagina->bitDeUso == 0){
				paginaAReemplazar = pagina;
			}
		}
	}
	pthread_mutex_unlock(&mutexListaFrames);
	return paginaAReemplazar;
}

struct pagina* reemplazarPaginaClock(){
	struct pagina* paginaAReemplazar = NULL;
	pthread_mutex_lock(&mutexListaFrames);
	while(paginaAReemplazar == NULL){
		struct pagina* pagina = list_get(listaDeFrames,punteroClock);
		if(pagina->clock == 0 && pagina->bitDeUso == 0){
			paginaAReemplazar 	= pagina;
		} else {
			pagina->clock 		= 0;
		}
		if(punteroClock+1 == listaDeFrames->elements_count){
			punteroClock = 0;
		} else {
			punteroClock++;
		}
	}
	pthread_mutex_unlock(&mutexListaFrames);
	return paginaAReemplazar;
}

int tamanioDisponibleEnSwap(){
	pthread_mutex_lock(&mutexListaPaginasSwap);
	int tamanioOcupadoEnSwap = listaPaginasSwap->elements_count * tamanioPagina;
	pthread_mutex_unlock(&mutexListaPaginasSwap);
	return tamanioSwap - tamanioOcupadoEnSwap;
}

struct pagina* traerAMemoriaPCB(struct TCB* tcb){
	struct pagina* pagPCB = obtenerPaginaPIDDeTID(tcb->TID);
	traerAMemoria(pagPCB);
	return pagPCB;
}

void imprimirSwap(){
	FILE* swap = fopen("logMiRamHQ.txt", "a+");
	fprintf(swap,"INSTANTE: %d\n",clock());
	fprintf(swap, "-------------------------------------------------------------------------------------------------------------\n");
	fprintf(swap, "Dump: ");imprimirFecha(swap);
	imprimirTablaPaginas(swap,listaPaginasSwap, 1);
	fprintf(swap, "------------------------------------------------------------------------------------------------------------\n");
	fclose(swap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * INSERTAR Y TRAER DE MEMORIA
 */

void* serializarPCB(struct PCB* pcb){
	void * magic       = malloc(TAMANIO_PCB);
	int desplazamiento = 0;
	//PID
	agregar_a_serializacion(magic, &desplazamiento, &(pcb->PID), sizeof(int));

	//DIRECC PRIMER TAREA
	agregar_a_serializacion(magic, &desplazamiento, &(pcb->direccPrimerTarea), sizeof(int));

	return magic;
}


void* serializarTCB(struct TCB* tcb){
	void * magic       = malloc(TAMANIO_TCB);
	int desplazamiento = 0;
	//PID
	agregar_a_serializacion(magic, &desplazamiento, &(tcb->TID), sizeof(uint32_t));

	//ESTADO
	agregar_a_serializacion(magic, &desplazamiento, &(tcb->Estado), sizeof(char));

	//POSICION X
	agregar_a_serializacion(magic, &desplazamiento, &(tcb->PosicionX), sizeof(uint32_t));

	//POSICION Y
	agregar_a_serializacion(magic, &desplazamiento, &(tcb->PosicionY), sizeof(uint32_t));

	//PROXIMA INSTRUCCION
	agregar_a_serializacion(magic, &desplazamiento, &(tcb->ProximaInstruccion), sizeof(uint32_t));

	//DIRECC PCB
	agregar_a_serializacion(magic, &desplazamiento, &(tcb->DireccionPCB), sizeof(uint32_t));

	return magic;
}

void* serializarTarea(struct Tarea* proximaTarea, int bytes) {
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	int longitudNombreTarea = strlen(proximaTarea->nombre);

	agregar_a_serializacion(magic, &desplazamiento, &(longitudNombreTarea), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, proximaTarea->nombre, longitudNombreTarea + 1);
	agregar_a_serializacion(magic, &desplazamiento, &(proximaTarea->parametro), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(proximaTarea->duracion), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(proximaTarea->ubicacionEnX), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(proximaTarea->ubicacionEnY), sizeof(int));

	return magic;
}

void* serializarTareas(t_list* listaTareas, int tamanioTareas){
	void* magic = malloc(tamanioTareas);
	int desplazamiento = 0;
	agregar_a_serializacion(magic, &desplazamiento, &(listaTareas->elements_count), sizeof(int));
	for(int i = 0; i < listaTareas->elements_count; i++){
		struct Tarea *tarea    = list_get(listaTareas, i);
		int tamTarea           = tamanioTarea(tarea);
		void* tareaSerializada = serializarTarea(tarea, tamTarea);
		memcpy(magic + desplazamiento, tareaSerializada, tamTarea);
		free(tareaSerializada);
		desplazamiento += tamTarea;
	}
	return magic;
}

void* serializarTareaAux(t_list* listaTareas, int index){
	char* magic = string_new();
	char* tarea = list_get(listaTareas, index);
	string_append(&magic, tarea);
	return magic;
}

void* serializarTareasAux(t_list* listaTareas){
	char* magic = string_new();
	for(int i = 0; i < listaTareas -> elements_count; i++){
		char* tarea 	= list_get(listaTareas, i);
		string_append(&magic, tarea);
		if(i+1 != listaTareas->elements_count){
			string_append(&magic, "|");
		}
	}
	return magic;
}

void* insertarAMemoria(void* aInsertar, int* desplazamiento, int tamanioDisponible, int tamanioTotal, char tipoDeDato){
	int tamanioAInsertar;
	if(tamanioTotal > tamanioDisponible && tamanioDisponible != 0){
		tamanioAInsertar = tamanioDisponible;
	} else {
		tamanioAInsertar = tamanioTotal;
	}
	int tamanioRestante = tamanioTotal - tamanioAInsertar;
	//REGION CRITICA
	pthread_mutex_lock(&mutexBaseMemoria);
	memcpy(baseMemoria + *desplazamiento, aInsertar, tamanioAInsertar);
	pthread_mutex_unlock(&mutexBaseMemoria);
	//REGION CRITICA

	*desplazamiento += tamanioAInsertar;
	void* aInsertarAux;

	if(tamanioRestante > 0){
		aInsertarAux  = malloc(tamanioRestante);
		memcpy(aInsertarAux, aInsertar + tamanioAInsertar, tamanioRestante);
		free(aInsertar);
		return aInsertarAux;
	}
	else {
		free(aInsertar);
		return NULL;
	}
}

void imprimirTablaPatota(int pid, char* str){
	printf("%s\n", str);
	struct tablaPaginasPatota* tablaPatotas = buscarTablaPaginas(pid);
	for(int i = 0; i < tablaPatotas->paginas->elements_count; i++){
		struct pagina* pag = list_get(tablaPatotas->paginas,i);
		printf("PAGINA: %d, NUMERO FRAME: %d\n", pag->numeroPagina,pag->numeroFrame);
		for(int j = 0; j < pag->infoFrame->elements_count; j++){
			struct infoFrame* info = list_get(pag->infoFrame,j);
			printf("\t TIPO DE DATO: %c, TAMANIO: %d\n", info->tipoDeDato,info->tamanio);
		}
	}
	printf("\n\n");
}

void insertarAMemoriaConPaginacion(int pid, int tamanioTotal, int desplazamiento, int frame, void* aInsertar, char tipoDeDato, int tid, int numeroTarea){
	int tamDisponible = tamanioDisponible(frame);
	if(tamDisponible >= tamanioTotal){
		struct infoFrame* info   	= malloc(sizeof(struct infoFrame));
		info->inicio             	= desplazamiento % tamanioPagina;
		insertarAMemoria(aInsertar, &desplazamiento, tamDisponible, tamanioTotal, tipoDeDato);
		struct pagina* ultimaPag 	= ultimaPagina(pid);
		info->tamanio            	= tamanioTotal;
		info->tipoDeDato         	= tipoDeDato;
		info->tid				 	= tid;
		info->numeroTarea			= numeroTarea;
		sem_wait(&(ultimaPag->semaforoInfoFrame));
		list_add(ultimaPag->infoFrame, info);
		sem_post(&(ultimaPag->semaforoInfoFrame));
	} else {
		void* aInsertarAux;
		int tamanioRestante                 = tamanioTotal - tamDisponible;
		if(tamDisponible != 0){
			aInsertarAux = insertarAMemoria(aInsertar, &desplazamiento, tamDisponible, tamanioTotal, tipoDeDato);
		} else {
			aInsertarAux = malloc(tamanioRestante);
			memcpy(aInsertarAux, aInsertar, tamanioRestante);
			free(aInsertar);
		}
		struct infoFrame* info              = malloc(sizeof(struct infoFrame));
		info->inicio                        = (tamanioPagina*(frame+1) - tamDisponible) % tamanioPagina;
		info->tamanio                       = tamDisponible;
		info->tipoDeDato                    = tipoDeDato;
		info->tid				 			= tid;
		info->numeroTarea				 	= numeroTarea;
		struct pagina* ultimaPag            = ultimaPagina(pid);
		sem_wait(&(ultimaPag->semaforoInfoFrame));
		list_add(ultimaPag->infoFrame, info);
		sem_post(&(ultimaPag->semaforoInfoFrame));
		frame                               = proximoFrameLibre();
		agregarPagina(pid, frame);
		insertarAMemoriaConPaginacion(pid, tamanioRestante, frame * tamanioPagina, frame, aInsertarAux, tipoDeDato, tid, numeroTarea);
	}
}


void* obtenerDeMemoria(int* desplazamiento, int tamanio, int esString){
	void* data = NULL;
	if(esString){
		data = malloc(tamanio + 1);
	} else {
		data = malloc(tamanio);
	}
	//REGION CRITICA
	pthread_mutex_lock(&mutexBaseMemoria);
	memcpy(data, baseMemoria + *desplazamiento, tamanio);
	pthread_mutex_unlock(&mutexBaseMemoria);
	//REGION CRITICA
	*desplazamiento += tamanio;
	return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * LISTAR_TRIPULANTES
 */

void enviarTripulantes(int socket){

	t_list* tripulantes = obtenerTripulantes();
	int bufferSize      = tamanioTripulantes(tripulantes);

	int bytes = bufferSize + sizeof(int);

	void* a_enviar = serializarListaTripulantes(tripulantes, bytes); // Para la cantidad de tripulantes

	send(socket, a_enviar, bytes, 0);
	close(socket);
	free(a_enviar);
}

void* serializarListaTripulantes(t_list* tripulantes, int bytes) {
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	//CANTIDAD TRIPULANTES
	agregar_a_serializacion(magic, &desplazamiento, &(tripulantes->elements_count), sizeof(int));

	for(int i = 0; i < tripulantes->elements_count; i ++){
		struct tripulante* tripulante = list_get(tripulantes, i);
		agregar_a_serializacion(magic, &desplazamiento, &(tripulante->tid), sizeof(int));
		agregar_a_serializacion(magic, &desplazamiento, &(tripulante->pid), sizeof(int));
		agregar_a_serializacion(magic, &desplazamiento, &(tripulante->estado), sizeof(char));
	}

	while(tripulantes->elements_count != 0){
		struct tripulante* tripulante = list_remove(tripulantes, 0);
		free(tripulante);
	}
	list_destroy(tripulantes);

	return magic;
}

int tamanioTripulantes(t_list* tripulantes){
	return (sizeof(int)*2 + sizeof(char)) * tripulantes->elements_count;
}

t_list* obtenerTripulantes(){
	t_list* tripulantes;
	if(strcmp(esquemaMemoria, "SEGMENTACION") == 0){
		tripulantes = obtenerTripulantesSegmentacion();
	} else if(strcmp(esquemaMemoria, "PAGINACION") == 0){
		tripulantes = obtenerTripulantesPaginacion();
	}
	return tripulantes;
}

t_list* obtenerTripulantesSegmentacion(){
	t_list* tripulantes = list_create();
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaSegmentosPatota* tablaPatota = list_get(tablasPatotas, i);
		waitSemaforoSegmentosTCB(tablaPatota);
		for(int j = 0; j < tablaPatota->segmentosTCB->elements_count; j++){
			struct segmento* segmentoTCB = list_get(tablaPatota->segmentosTCB, j);
			int inicio                   = segmentoTCB->inicio;
			void* data                   = obtenerDeMemoria(&(inicio), segmentoTCB->tamanio, 0);
			struct TCB* tcb              = deserializarTCB(data);
			free(data);
			struct tripulante* tripu     = malloc(sizeof(struct tripulante));
			tripu->estado                = tcb->Estado;
			tripu->pid                   = tablaPatota->pid;
			tripu->tid                   = tcb->TID;
			list_add(tripulantes, tripu);
			free(tcb);
		}
		postSemaforoSegmentosTCB(tablaPatota);
	}
	return tripulantes;
}

struct PCB* obtenerPCBDeTCB(struct TCB* tcb){
	if(tcb->DireccionPCB == -1){
		struct pagina* pagPCB = obtenerPaginaPIDDeTID(tcb->TID);
		traerAMemoria(pagPCB);
		tcb->DireccionPCB = pagPCB->numeroFrame * tamanioPagina;
	}
	int inicioPCB 			 = tcb->DireccionPCB;
	void* data               = obtenerDeMemoria(&inicioPCB, TAMANIO_PCB, 0);
	struct PCB* pcb          = deserializarPCB(data);
	free(data);
	return pcb;
}

struct pagina* obtenerPaginaPIDDeTID(int tid){
	struct pagina* resultado = NULL;
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaPaginasPatota* tablaPatota = list_get(tablasPatotas,i);
		for(int j = 0; j < tablaPatota->paginas->elements_count ; j++){
			struct pagina* pag = list_get(tablaPatota->paginas,j);
			for(int k = 0; k < pag->infoFrame->elements_count ; k++){
				struct infoFrame* info = list_get(pag->infoFrame,k);
				if(info->tipoDeDato == 'T'){
					if(info->tid == tid){
						resultado = list_get(tablaPatota->paginas,0);
					}
				}
			}
		}
	}
	return resultado;
}


int obtenerPIDDeListaConTID(t_list* tripulantes,int tid){
	for(int i = 0; i < tripulantes->elements_count; i++){
		struct tripulante* tripu = list_get(tripulantes,i);
		if(tripu->tid == tid) return tripu->pid;
	}
	return -1;
}

t_list* obtenerTCBsPaginacion(int pid){
	t_list* listaTCBs 		= list_create();
	void* tripulantes 		= malloc(TAMANIO_TCB*cantidadDeTripulantes);
	int desplazamiento 		= 0;
	int desplazamientoAux 	= 0;

	//REGION CRITICA TABLA PAGINAS
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaPaginasPatota* tablaPatota = list_get(tablasPatotas,i);
	//REGION CRITICA TABLA PAGINAS
		if(pid == 0 || pid == tablaPatota->pid){
			for(int j = 0; j < tablaPatota->paginas->elements_count ; j++){
				struct pagina* pag = list_get(tablaPatota->paginas,j);
				for(int k = 0; k < pag->infoFrame->elements_count ; k++){
					struct infoFrame* info = list_get(pag->infoFrame,k);
					if(info->tipoDeDato == 'T' && desplazamiento < TAMANIO_TCB*cantidadDeTripulantes){
						if(pag->presencia == 0 ) traerAMemoria(pag);
						else {
							sem_wait(&(pag->refrescarPagina));
							refrescarPagina(pag);
							sem_post(&(pag->refrescarPagina));
						}
						int inicio 			= pag->numeroFrame * tamanioPagina + info->inicio;
						void* tripulante 	= obtenerDeMemoria(&inicio, info->tamanio, 0);
						memcpy(tripulantes+desplazamiento,tripulante,info->tamanio);
						desplazamiento += info->tamanio;
						free(tripulante);
					}
				}
			}
		}

	}
	int cantidadTripulantesAux = desplazamiento / TAMANIO_TCB;
	for(int i = 0; i < cantidadTripulantesAux ; i++) {
		void* tripulante 		 = malloc(TAMANIO_TCB);
		memcpy(tripulante, tripulantes + desplazamientoAux, TAMANIO_TCB);
		desplazamientoAux       += TAMANIO_TCB;
		struct TCB* tcb          = deserializarTCB(tripulante);
		list_add(listaTCBs, tcb);
		free(tripulante);
	}
	free(tripulantes);
	return listaTCBs;
}

t_list* obtenerTripulantesPaginacion(){
	t_list* listaTCBs 			= obtenerTCBsPaginacion(0);
	t_list* listaTripulantes 	= transformarTCBsATripulante(listaTCBs);
	list_destroy(listaTCBs);
	return listaTripulantes;
}

t_list* transformarTCBsATripulante(t_list* listaTCBs){
	t_list* listaTripulantes = list_create();
	int anteriorTienePCB = 0;
	while(listaTCBs->elements_count != 0){
		struct TCB* tcb 		 = list_remove(listaTCBs,0);
		struct pagina* paginaConPCB = obtenerPaginaPIDDeTID(tcb->TID);
		if(paginaConPCB->presencia == 0 && !anteriorTienePCB){
			struct pagina* paginaPCB 	= traerAMemoriaPCB(tcb);
			int tid = tcb->TID;
			free(tcb);
			tcb 						= buscarTCB(tid);
			anteriorTienePCB 			= 1;
		} else {
			int tid = tcb->TID;
			free(tcb);
			tcb 							= buscarTCBEnTablaPaginas(tid);
			struct pagina* paginaConPCBAux 	= obtenerPaginaPIDDeTID(tcb->TID);
			if(paginaConPCBAux->presencia == 0){
				struct pagina* paginaPCB 	= traerAMemoriaPCB(tcb);
				tid = tcb->TID;
				free(tcb);
				tcb 						= buscarTCBEnTablaPaginas(tid);
			}
		}
		struct tripulante* tripu = malloc(sizeof(struct tripulante));
		tripu->estado            = tcb->Estado;
		struct PCB* pcb 		 = obtenerPCBDeTCB(tcb);
		tripu->pid               = pcb->PID;
		tripu->tid               = tcb->TID;
		list_add(listaTripulantes,tripu);
		free(pcb);
		free(tcb);
	}
	return listaTripulantes;
}


struct PCB* pcbDe(struct TCB* tcb){
	struct pagina* paginaConPCB = obtenerPaginaPIDDeTID(tcb->TID);
	if(paginaConPCB->presencia == 0){
		struct pagina* paginaPCB 	= traerAMemoriaPCB(tcb);
		tcb 						= buscarTCB(tcb->TID);
	}
	int inicioPCB 	= tcb->DireccionPCB;
	free(tcb);
	void* data      = obtenerDeMemoria(&inicioPCB, TAMANIO_PCB, 0);
	struct PCB* pcb = deserializarPCB(data);
	return pcb;
}

int seBuscaPid(int pidABuscar, int pidTabla){
	if(pidABuscar == 0) return 1;
	else return pidABuscar == pidTabla;
}

struct TCB* deserializarTCB(void* magic){
	int desplazamiento 		= 0;

	struct TCB* tcb 		= malloc(sizeof(struct TCB));

    int* punteroTid 		= malloc(sizeof(int));
    memcpy(punteroTid, magic + desplazamiento, sizeof(int));
    desplazamiento         += sizeof(int);
    tcb->TID				= *(punteroTid);
    free(punteroTid);

    char* punteroEstado 	= malloc(sizeof(char));
    memcpy(punteroEstado, magic + desplazamiento, sizeof(char));
    desplazamiento         += sizeof(char);
    tcb->Estado				= *(punteroEstado);
    free(punteroEstado);

    int* punteroPosX 		= malloc(sizeof(int));
    memcpy(punteroPosX, magic + desplazamiento, sizeof(int));
    desplazamiento         += sizeof(int);
    tcb->PosicionX			= *(punteroPosX);
    free(punteroPosX);

    int* punteroPosY 		= malloc(sizeof(int));
    memcpy(punteroPosY, magic + desplazamiento, sizeof(int));
    desplazamiento         += sizeof(int);
    tcb->PosicionY			= *(punteroPosY);
    free(punteroPosY);

    int* punteroProxima	= malloc(sizeof(int));
    memcpy(punteroProxima, magic + desplazamiento, sizeof(int));
    desplazamiento         += sizeof(int);
    tcb->ProximaInstruccion	= *(punteroProxima);
    free(punteroProxima);

    int* punteroDireccPCB	= malloc(sizeof(int));
    memcpy(punteroDireccPCB, magic + desplazamiento, sizeof(int));
    desplazamiento         += sizeof(int);
    tcb->DireccionPCB		= *(punteroDireccPCB);
    free(punteroDireccPCB);

	return tcb;
}

struct PCB* deserializarPCB(void* magic){
    int desplazamiento         = 0;

    struct PCB* pcb         = malloc(sizeof(struct PCB));

    int* punteroPid 		= malloc(sizeof(int));
    memcpy(punteroPid, magic + desplazamiento, sizeof(int));
    desplazamiento         += sizeof(int);
    pcb->PID                = *(punteroPid);
    free(punteroPid);

    int* punteroDirecc 		= malloc(sizeof(int));
    memcpy(punteroDirecc, magic + desplazamiento, sizeof(int));
    desplazamiento         += sizeof(int);
    pcb->direccPrimerTarea  = *(punteroDirecc);
    free(punteroDirecc);

    return pcb;
}

struct Tarea* obtenerTarea(int pid, int numeroTarea){
	struct Tarea* tarea = NULL;
	if(strcmp(esquemaMemoria, "SEGMENTACION") == 0){
		tarea = obtenerTareaSegmentacion(pid, numeroTarea);
	} else {
		tarea = obtenerTareaPaginacion(pid, numeroTarea);
	}
	return tarea;
}

struct Tarea* obtenerTareaPaginacion(int pid, int numeroTarea){
	void* tarea = malloc(0);
	int tamanio = 0, desplazamiento = 0;
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaPaginasPatota* tablaPatota = list_get(tablasPatotas,i);
		if(tablaPatota->pid == pid){
			for(int j = 0; j < tablaPatota->paginas->elements_count ; j++){
				struct pagina* pag = list_get(tablaPatota->paginas,j);
				for(int k = 0; k < pag->infoFrame->elements_count ; k++){
					struct infoFrame* info = list_get(pag->infoFrame,k);
					if(info->tipoDeDato == 'X' && info->numeroTarea == numeroTarea){
						if(pag->presencia == 0) traerAMemoria(pag);
						else {
							sem_wait(&(pag->refrescarPagina));
							refrescarPagina(pag);
							sem_post(&(pag->refrescarPagina));
						}
						int inicio 			= pag->numeroFrame * tamanioPagina + info->inicio;
						tamanio    		   += info->tamanio;
						tarea 				= realloc( tarea, tamanio );
						void* tareaAux 		= obtenerDeMemoria( &inicio, info->tamanio, 0);
						memcpy( tarea + desplazamiento, tareaAux, info->tamanio);
						desplazamiento += info->tamanio;
						free(tareaAux);
					}
				}
			}
		}
	}
	struct Tarea* tareaResultado = NULL;
	if(tamanio != 0){
		tareaResultado = deserializarTareaAux(tarea);
	}
	free(tarea);
	return tareaResultado;
}

struct Tarea*  obtenerTareaSegmentacion(int pid, int numeroTarea){
	struct tablaSegmentosPatota* tablaPatota 	= buscarTablaSegmentos(pid);
	struct segmento* segmentoTareas          	= tablaPatota->tareas;
	int inicioTareas                         	= segmentoTareas->inicio;
	char* tareas                             	= (char*)obtenerDeMemoria(&inicioTareas, segmentoTareas->tamanio, 1);
	t_list* listaTareas							= deserializarTareasAux(tareas);
	free(tareas);
	struct Tarea* tarea 						= NULL;
	if((numeroTarea != listaTareas->elements_count)){
		 tarea = list_remove(listaTareas, numeroTarea);
	}
	while(listaTareas->elements_count != 0){
		struct Tarea* tareaAux = list_remove(listaTareas, 0);
		free(tareaAux->nombre);
		free(tareaAux);
	}
	list_destroy(listaTareas);
	return tarea;
}

t_list* deserializarTareasAux(char* tareas){
	t_list* listaTareas = list_create();
	char** arrTareas 	= string_split(tareas, "|");
	for(int i = 0; arrTareas[i] != NULL; i++){
		struct Tarea* tarea = deserializarTareaAux(arrTareas[i]);
		list_add(listaTareas, tarea);
	}
	for(int i = 0; arrTareas[i] != NULL; i++){
		free(arrTareas[i]);
		arrTareas[i] = NULL;
	}
	free(arrTareas);
	return listaTareas;
}

struct Tarea* deserializarTareaAux(char* tareas){
	struct Tarea* tarea = malloc(sizeof(struct Tarea));
	if(string_contains(tareas," ")){
		char** arrNombre 			= string_split(tareas, " ");
		char** arrParametros 		= string_split(arrNombre[1], ";");

		tarea->nombre				= malloc(strlen(arrNombre[0])+1);
		strcpy(tarea->nombre, arrNombre[0]);
		tarea->ubicacionEnX 		= strtol(arrParametros[1],NULL,10);
		tarea->ubicacionEnY 		= strtol(arrParametros[2],NULL,10);
		tarea->duracion 			= strtol(arrParametros[3],NULL,10);

		tarea->parametro 			= strtol(arrParametros[0], NULL, 10);

		free(arrParametros[0]);
		arrParametros[0] = NULL;
		free(arrParametros[1]);
		arrParametros[1] = NULL;
		free(arrParametros[2]);
		arrParametros[2] = NULL;
		free(arrParametros[3]);
		arrParametros[3] = NULL;
		free(arrParametros);
		free(arrNombre[0]);
		arrNombre[0]	= NULL;
		free(arrNombre[1]);
		arrNombre[1]	= NULL;
		free(arrNombre);
	} else{
		char** arrTarea 			= string_split(tareas, ";");
		tarea->nombre				= malloc(strlen(arrTarea[0])+1);
		strcpy(tarea->nombre, arrTarea[0]);
		tarea->ubicacionEnX 		= strtol(arrTarea[1],NULL,10);
		tarea->ubicacionEnY 		= strtol(arrTarea[2],NULL,10);
		tarea->duracion 			= strtol(arrTarea[3],NULL,10);
		tarea->parametro 			= 0;

		free(arrTarea[0]);
		arrTarea[0] = NULL;
		free(arrTarea[1]);
		arrTarea[1] = NULL;
		free(arrTarea[2]);
		arrTarea[2] = NULL;
		free(arrTarea[3]);
		arrTarea[3] = NULL;
		free(arrTarea);
	}
	return tarea;
}

struct Tarea* deserializarTareas(void* tareas, int* desplazamiento){
	struct Tarea* tarea 		= malloc(sizeof(struct Tarea));

	int longitudNombre		 	= *(int*)(tareas+*desplazamiento);
	*desplazamiento 			+= sizeof(int);

	char* nombre 				= malloc(longitudNombre+1);
	memcpy(nombre, tareas + *desplazamiento, longitudNombre+1);

	tarea->nombre                = nombre;
	*desplazamiento 			+= longitudNombre+1;

	tarea->parametro 			= *(int*)(tareas+*desplazamiento);
	*desplazamiento 			+= sizeof(int);

	tarea->duracion 			= *(int*)(tareas+*desplazamiento);
	*desplazamiento 			+= sizeof(int);

	tarea->ubicacionEnX 		= *(int*)(tareas+*desplazamiento);
	*desplazamiento 			+= sizeof(int);

	tarea->ubicacionEnY 		= *(int*)(tareas+*desplazamiento);
	*desplazamiento 			+= sizeof(int);


	return tarea;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * LISTAR_TRIPULANTES
 */

int recibirExpulsionDeTripulante(int socket){
	int* size   = (int*)deserializar(socket, sizeof(int), "size Buffer");
	int* tidAux = (int*)deserializar(socket, sizeof(int), "tid");
	int tid     = *tidAux;
	free(size);
	free(tidAux);
	return tid;
}

void expulsarTripulante(int tid){
	pthread_mutex_lock(&mutexExpulsar);
	if(strcmp(esquemaMemoria, "SEGMENTACION") == 0){
		borrarSegmentoTCB(tid);
	} else {
		borrarInfoFrame(tid);
	}
	pthread_mutex_unlock(&mutexExpulsar);

	pthread_mutex_lock(&mutexCantidadTripulantes);
	cantidadDeTripulantes--;
	pthread_mutex_unlock(&mutexCantidadTripulantes);


	if(mostrarMapa){
		quitarDeMapa(tid);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * ENVIAR PROXIMA TAREA
 */

int recibirPedidoDeProximaTarea(int socket){
	int* ptrTid = (int*)deserializar(socket, sizeof(int), "PID");
	int tid		= *ptrTid;
	free(ptrTid);
	return tid;
}

void enviarProximaTarea(int socket, int tid){
	pthread_mutex_lock(&mutexActualizarTripulante);
	struct TCB* tcb = buscarTCB(tid);
	if(strcmp(esquemaMemoria, "PAGINACION") == 0){
		struct pagina* paginaConPCB = obtenerPaginaPIDDeTID(tid);
		if(paginaConPCB->presencia == 0){
			struct pagina* paginaPCB 	= traerAMemoriaPCB(tcb);
			int tid 					= tcb->TID;
			free(tcb);
			tcb 						= buscarTCB(tid);
		}
	}
	int inicioPCB       = tcb->DireccionPCB;
	void* data          = obtenerDeMemoria(&inicioPCB, TAMANIO_PCB, 0);
	struct PCB* pcb     = deserializarPCB(data);
	free(data);
	int proximaTarea    = tcb->ProximaInstruccion++;
	actualizarTCB(tcb, 'K', -1, -1, 1);
	pthread_mutex_unlock(&mutexActualizarTripulante);
	free(tcb);
	pthread_mutex_lock(&mutexActualizarTripulante);
	struct Tarea* tarea = obtenerTarea(pcb->PID, proximaTarea);
	pthread_mutex_unlock(&mutexActualizarTripulante);
	free(pcb);
	int ultimaTarea	= 0;
	if(tarea == NULL){
		tarea = malloc(sizeof(struct Tarea));
		tarea->nombre 				= "";
		tarea->ubicacionEnX 		= -1;
		tarea->ubicacionEnY 		= -1;
		tarea->parametro			= -1;
		tarea->duracion 			= -1;
		ultimaTarea 				= 1;
	}
	int bytes 			= tamanioTarea(tarea);
	void* a_enviar      = serializarTarea(tarea, bytes);
	if(!ultimaTarea) free(tarea->nombre);
	free(tarea);
	send(socket, a_enviar, bytes, 0);
	free(a_enviar);
}

struct TCB* buscarTCB(int tid){
	//pthread_mutex_lock(&mutexBuscar);
	struct TCB* tcb = NULL;
	if(strcmp(esquemaMemoria, "SEGMENTACION") == 0){
		tcb = buscarTCBEnTablaSegmentos(tid);
	} else {
		tcb = buscarTCBEnTablaPaginas(tid);
	}
	//pthread_mutex_unlock(&mutexBuscar);
	return tcb;
}

void waitSemaforoSegmentosTCB(struct tablaSegmentosPatota* tablaSegmentos){
	sem_wait(&(tablaSegmentos->semaforoSegmentosTCB));
}
void postSemaforoSegmentosTCB(struct tablaSegmentosPatota* tablaSegmentos){
	sem_post(&(tablaSegmentos->semaforoSegmentosTCB));
}


struct TCB* buscarTCBEnTablaSegmentos(int tid){
	struct TCB* resultado = NULL;
	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaSegmentosPatota* tablaPatota = list_get(tablasPatotas, i);
		waitSemaforoSegmentosTCB(tablaPatota);
		for(int j = 0; j < tablaPatota->segmentosTCB->elements_count; j++){
			struct segmento* segmentoTCB = list_get(tablaPatota->segmentosTCB, j);
			int inicio                   = segmentoTCB->inicio;
			void* data                   = obtenerDeMemoria(&(inicio), segmentoTCB->tamanio, 0);
			struct TCB* tcb              = deserializarTCB(data);
			free(data);
			if(tcb->TID == tid){
				resultado = tcb;
				break;
			} else {
				free(tcb);
			}
		}
		postSemaforoSegmentosTCB(tablaPatota);
		if(resultado != NULL) break;
	}
	return resultado;
}

struct TCB* buscarTCBEnTablaPaginas(int tid){
	int desplazamiento = 0;
	void* tcbMagic = malloc(TAMANIO_TCB);

	for(int i = 0; i < tablasPatotas->elements_count; i++){
		struct tablaPaginasPatota* tablaPatota = list_get(tablasPatotas,i);
		for(int j = 0; j < tablaPatota->paginas->elements_count ; j++){
			struct pagina* pag = list_get(tablaPatota->paginas,j);
			for(int k = 0; k < pag->infoFrame->elements_count ; k++){
				struct infoFrame* info = list_get(pag->infoFrame,k);
				if(info->tipoDeDato == 'T'){
					if(tid == info->tid){
						if(pag->presencia == 0) traerAMemoria(pag);
						int inicio 			= pag->numeroFrame * tamanioPagina + info->inicio;
						void* porcionTCB 	= obtenerDeMemoria(&inicio, info->tamanio, 0);
						memcpy(tcbMagic + desplazamiento,porcionTCB,info->tamanio);
						desplazamiento += info->tamanio;
						free(porcionTCB);
					}
				}
			}
		}
	}
	struct TCB* tcb          = deserializarTCB(tcbMagic);
	free(tcbMagic);
	return tcb;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * MAPA
 */

void inicializarMapa(){
	relacionesID 	= list_create();
	nivel 			= nivel_crear("AMONG OS");
	nivel_gui_inicializar();
	int cols, rows;
	nivel_gui_get_area_nivel(&cols, &rows);
	nivel_gui_dibujar(nivel);
}

void liberarMapa(){
	int tamanio = relacionesID->elements_count;
	for(int i = 0; i < tamanio; i++){
		struct relacionID* relacion = list_get(relacionesID,i);
		list_remove(relacionesID, i);
		free(relacion);
	}
	list_destroy(relacionesID);
	nivel_destruir(nivel);
}

void inicializarTripulante(int tid, int posX, int posY){
	struct relacionID* relacion = malloc(sizeof(struct relacionID));
	relacion->idMapa 			= idMapa++;
	relacion->tid    			= tid;

	pthread_mutex_lock(&mutexRelacionesID);
	list_add(relacionesID, relacion);
	pthread_mutex_unlock(&mutexRelacionesID);

	dibujarTripulante(posX, posY, relacion);
}

void dibujarTripulante(int posX, int posY, struct relacionID* relacion){
	pthread_mutex_lock(&mutexMapa);
	int err = personaje_crear(nivel, relacion->idMapa, posX, posY);
	ASSERT_CREATE(nivel, relacion->idMapa, err);
	nivel_gui_dibujar(nivel);
	pthread_mutex_unlock(&mutexMapa);
}

void actualizarTripulante(int tid, int posX, int posY){
	pthread_mutex_lock(&mutexRelacionesID);
	for(int i = 0; i < relacionesID->elements_count; i++){
		struct relacionID* relacion = list_get(relacionesID, i);
		if(tid == relacion->tid){
			pthread_mutex_lock(&mutexMapa);
			int err = item_mover(nivel, relacion->idMapa, posX, posY);
			ASSERT_CREATE(nivel, relacion->idMapa, err);
			nivel_gui_dibujar(nivel);
			pthread_mutex_unlock(&mutexMapa);
		}
	}
	pthread_mutex_unlock(&mutexRelacionesID);
}

void quitarDeMapa(int tid){
	pthread_mutex_lock(&mutexRelacionesID);
	for(int i = 0; i < relacionesID->elements_count; i++){
		struct relacionID* relacion = list_get(relacionesID, i);
		if(tid == relacion->tid){
			pthread_mutex_lock(&mutexMapa);
			int err = item_borrar(nivel, relacion->idMapa);
			ASSERT_CREATE(nivel, relacion->idMapa, err);
			nivel_gui_dibujar(nivel);
			pthread_mutex_unlock(&mutexMapa);
			list_remove(relacionesID, i);
			free(relacion);
		}
	}
	pthread_mutex_unlock(&mutexRelacionesID);
}

/*void agregarRecurso(char* nombreTarea, int posX, int posY, int cantidad){
	char idRecurso = recursoSegunTarea(nombreTarea);
}

char recursoSegunTarea(char* nombreTarea){
	if(strcmp(nombreTarea, "GENERAR_OXIGENO") == 0 || strcmp(nombreTarea, "CONSUMIR_OXIGENO") == 0) return 'o';
	if(strcmp(nombreTarea, "GENERAR_COMIDA") == 0 || strcmp(nombreTarea, "CONSUMIR_COMIDA") == 0) return 'c';
	if(strcmp(nombreTarea, "GENERAR_BASURA") == 0 || strcmp(nombreTarea, "CONSUMIR_BASURA") == 0) return 'b';
}
int caja_crear(NIVEL* nivel, char id, int x, int y, int srcs);*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * CAMBIO DE ESTADO
 */

void actualizarEstado(int socket){
	int* tid		= (int*)deserializar(socket, sizeof(int), "tid");
	char* estado 	= (char*)deserializar(socket, sizeof(char), "estado");
	send(socket,tid,sizeof(int),0);
	pthread_mutex_lock(&mutexActualizarTripulante);
	struct TCB* tcb = buscarTCB(*tid);
	tcb->Estado     = *estado;

	tcb = actualizarTCB(tcb, *estado, -1, -1, -1);
	if(*estado == 'X') expulsarTripulante(*tid);
	pthread_mutex_unlock(&mutexActualizarTripulante);

	free(tcb);
	free(tid);
	free(estado);
}

struct TCB* actualizarTCB(struct TCB* tcb, char estado, int posX, int posY, int proximaInstruccion){
	//pthread_mutex_lock(&mutexActualizacion);
	if(strcmp(esquemaMemoria,"SEGMENTACION") == 0){
		struct segmento* seg 	= (struct segmento*)buscarSegmentoTCB(tcb->TID);
		void* magic 			= serializarTCB(tcb);
		int inicio 				= seg->inicio;
		insertarAMemoria(magic,&inicio,0,seg->tamanio,'T');
		return tcb;
	} else {
		return actualizarTCBPaginacion(tcb, -1, estado, posX, posY, proximaInstruccion);
	}
	//pthread_mutex_unlock(&mutexActualizacion);
}


struct TCB* actualizarTCBPaginacion(struct TCB* tcb, int pid, char estado, int posX, int posY, int proximaInstruccion){
	struct pagina* paginaConPCB = obtenerPaginaPIDDeTID(tcb->TID);
	if(paginaConPCB->presencia == 0 && pid == -1){
		struct pagina* paginaPCB 	= traerAMemoriaPCB(tcb);
		int tid 					= tcb->TID;
		free(tcb);
		tcb 						= buscarTCB(tid);
		if(estado 				!= 'K') tcb->Estado 	= estado;
		if(posX 				!= -1) 	tcb->PosicionX 	= posX;
		if(posY 				!= -1) 	tcb->PosicionY 	= posY;
		if(proximaInstruccion 	== 1) 	tcb->ProximaInstruccion++;
	}

	void* magic 								= serializarTCB(tcb);
	if(pid == -1){
		int inicioPCB                          	= tcb->DireccionPCB;
		void* data                              = obtenerDeMemoria(&inicioPCB, TAMANIO_PCB, 0);
		struct PCB* pcb                         = deserializarPCB(data);
		free(data);
		pid = pcb->PID;
		free(pcb);
	}
	struct tablaPaginasPatota* tablaPatota 	= buscarTablaPaginas(pid);
	t_list* paginas                        	= tablaPatota->paginas;

	void* magicAux = malloc(TAMANIO_TCB);
	memcpy(magicAux, magic, TAMANIO_TCB);
	int desplazamiento = 0;

	for(int i = 0; i < paginas->elements_count; i++){
		struct pagina* pagina = list_get(paginas, i);
		for(int j = 0; j < pagina->infoFrame->elements_count; j++){
			struct infoFrame* info = list_get(pagina->infoFrame, j);
			if(info->tid == tcb->TID){
				if(pagina->presencia == 0) traerAMemoria(pagina);
				else {
					sem_wait(&(pagina->refrescarPagina));
					refrescarPagina(pagina);
					sem_post(&(pagina->refrescarPagina));
				}
				int inicio = pagina->numeroFrame * tamanioPagina + info->inicio;
				insertarAMemoria(magic,&inicio,info->tamanio,info->tamanio,'T');
				desplazamiento += info->tamanio;
				if(TAMANIO_TCB - desplazamiento > 0){
					magic = malloc(TAMANIO_TCB - desplazamiento);
					memcpy(magic, magicAux + desplazamiento, TAMANIO_TCB - desplazamiento);
				}
			}
		}
	}
	free(magicAux);
	return tcb;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * DEVOLVER POSICION
 */

void enviarPosiciones(int ficheroSocket){
	int* tid			= (int*)deserializar(ficheroSocket, sizeof(int), "tid");
	pthread_mutex_lock(&mutexActualizarTripulante);
	struct TCB* tcb 	= buscarTCB(*tid);
	pthread_mutex_unlock(&mutexActualizarTripulante);

	void* magic 		= malloc(sizeof(int) * 2);
	int desplazamiento 	= 0;

	agregar_a_serializacion(magic, &desplazamiento, &(tcb->PosicionX), sizeof(int));
	agregar_a_serializacion(magic, &desplazamiento, &(tcb->PosicionY), sizeof(int));

	send(ficheroSocket, magic, sizeof(int) * 2, 0);

	free(tcb);
	free(magic);
	free(tid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * ACTUALIZAR POSICION
 */

void actualizarPosicion(int ficheroSocket){
	int* tid			= (int*)deserializar(ficheroSocket, sizeof(int), "tid");
	int* posX 			= (int*)deserializar(ficheroSocket, sizeof(int), "poscion X");
	int* posY 			= (int*)deserializar(ficheroSocket, sizeof(int), "poscion Y");

	pthread_mutex_lock(&mutexActualizarTripulante);
	struct TCB* tcb = buscarTCB(*tid);
	tcb->PosicionX 	= *posX;
	tcb->PosicionY 	= *posY;
	actualizarTCB(tcb, 'K', *posX, *posY, -1);
	pthread_mutex_unlock(&mutexActualizarTripulante);

	if(mostrarMapa){
		actualizarTripulante(tcb->TID, tcb->PosicionX, tcb->PosicionY);
	}
	send(ficheroSocket,tid,sizeof(int),0);
	free(tcb);
	free(tid);
	free(posX);
	free(posY);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * FUNCIONES HILOS
 */

void* iniciarPatotaMiRam(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	struct inicioDePatota*idp   = recibirInicioDePatota(ficheroSocketNuevo);
	int resul                   = generarEstructurasPatota(idp);
	void * a_enviar             = malloc(sizeof(int));
	int desplazamiento          = 0;
	agregar_a_serializacion(a_enviar, &desplazamiento, &resul, sizeof(int));
	send(ficheroSocketNuevo, a_enviar, sizeof(int), 0);
	free(a_enviar);
	close(ficheroSocketNuevo);
	free(data);
	return NULL;
};


void* listarTripulantesMiRam(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	enviarTripulantes(ficheroSocketNuevo);
	free(data);
	return 0;
}

void* expulsar(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	int tid                          = recibirExpulsionDeTripulante(ficheroSocketNuevo);
	expulsarTripulante(tid);
	free(data);
	return 0;
}

void* proximaTarea(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	int tid = recibirPedidoDeProximaTarea(ficheroSocketNuevo);
	enviarProximaTarea(ficheroSocketNuevo, tid);
	free(data);
	return NULL;
}

void* cambiarEstado(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	actualizarEstado(ficheroSocketNuevo);
	close(ficheroSocketNuevo);
	free(data);
	return NULL;
}

void* enviarPosicionTripulante(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	enviarPosiciones(ficheroSocketNuevo);
	close(ficheroSocketNuevo);
	free(data);
	return NULL;
}

void* cambiarPosicion(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	actualizarPosicion(ficheroSocketNuevo);
	close(ficheroSocketNuevo);
	free(data);
	return NULL;
}

void logearArchivoMiRam(char* log){
	char* horaActual 	= temporal_get_string_time("%d/%m/%y %H:%M:%S");
	pthread_mutex_lock(&mutexArchivoLogMiRam);
	FILE* archivoLog = fopen("../logMiRamHQ.txt", "a+");
	fprintf(archivoLog, "%s:\t%s\n", horaActual, log);
	fclose(archivoLog);
	pthread_mutex_unlock(&mutexArchivoLogMiRam);
	free(horaActual);
	free(log);
}
