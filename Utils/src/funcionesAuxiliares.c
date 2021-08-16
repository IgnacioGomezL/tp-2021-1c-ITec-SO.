/*
 * FuncionesAuxiliares.c
 *
 *  Created on: 26 abr. 2021
 *      Author: utnso
 */


#include "funcionesAuxiliares.h"

#define BACKLOG 10
#define MAXDATASIZE 100


pthread_mutex_t mutexServidor			= PTHREAD_MUTEX_INITIALIZER;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * NO SE USAN MAS
 */

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * SOCKETS
 */

int crearSocket() {

	int yes = 1;

	int ficheroSocketOriginal = socket(AF_INET, SOCK_STREAM, 0);
	comprobarErrores(ficheroSocketOriginal, "Socket");

	int respuestaSetSocket = setsockopt(ficheroSocketOriginal, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	comprobarErrores(respuestaSetSocket, "Set Socket Options");

	return ficheroSocketOriginal;
}

struct sockaddr_in setDireccion(char* puerto, char* ip) {
	struct sockaddr_in direcc;

	direcc.sin_family = AF_INET;
	direcc.sin_port = htons(atoi(puerto));
	direcc.sin_addr.s_addr = inet_addr(ip);
	memset(&(direcc.sin_zero), '\0', 8);

	return direcc;
}

void iniciarEscucha(int ficheroSocketOriginal, struct sockaddr_in direccPersonal) {
	int retornoBind = bind(ficheroSocketOriginal, (struct sockaddr *)&direccPersonal, sizeof(struct sockaddr));
	comprobarErrores(retornoBind, "Bind");

	int retornoListen = listen(ficheroSocketOriginal, BACKLOG);
	comprobarErrores(retornoListen, "Listen");
}

int aceptarConexion(int ficheroSocketOriginal, struct sockaddr_in direccDestino) {
	socklen_t tamanioDireccion = sizeof(struct sockaddr_in);
	int ficheroSocketNuevo = accept(ficheroSocketOriginal, (struct sockaddr *)&direccDestino, &tamanioDireccion);
	comprobarErrores(ficheroSocketNuevo, "Accept");

	return ficheroSocketNuevo;
}

void conectar(int ficheroSocket, struct sockaddr_in direccDestino) {
	int respuesta = connect(ficheroSocket, (struct sockaddr *)&direccDestino, sizeof(struct sockaddr));
	comprobarErrores(respuesta, "Connect");
}


int conectarAServidor(char* puerto, char* ip) {
	int socket = crearSocket();
	struct sockaddr_in direcc = setDireccion(puerto, ip);
	conectar(socket, direcc);

	return socket;
}

int recibir_operacion(int socket_cliente) {
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) != 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * GENERALES
 */

void comprobarErrores(int respuestaFuncion, char* llamada) {
	if(respuestaFuncion == -1) {
		perror(llamada);
		exit(1);
	}
}

char* mensajeCompleto(char* quienHabla,char* mensaje){
	char* aux = malloc(strlen(quienHabla)+ strlen(mensaje) + 1);
	strcpy(aux,quienHabla);
	strcat(aux,mensaje);
	return aux;
}

t_list* strsplit(char* leido){
	t_list* tokens 	= list_create();
	int i 			= 0;
	while(leido[i] != '\0'){
		int j = 0;
		while(leido[j + i] != ' ' && leido[j + i] != '\0'){
			j++;
		}
		char* token = string_substring(leido, i, j);
		if(string_ends_with(token, ",")){
			token = string_substring(token, 0, strlen(token)-1);
		}
		list_add(tokens, token);
		i += j;
		if(leido[i] != '\0'){
			i++;
		}
	}
	return tokens;
}

void imprimirFecha(FILE* archivo){
	time_t tiempo     = time(0);
	struct tm *tlocal = localtime(&tiempo);
	char output[128];
	strftime(output,128,"%d/%m/%y %H:%M:%S",tlocal);
	if(archivo == NULL){
		printf("%s\n",output);
	} else {
		fprintf(archivo, "%s\n",output);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * SERIALIZACION
 */

void* serializar_paquete(t_paquete* paquete, int bytes) {
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void* deserializar(int socket, int tamanio, char* info){
	void* data = malloc(tamanio);
	int resul  = recv(socket, data, tamanio, MSG_WAITALL);
	char* msj  = mensajeCompleto("Deserializar ",info);
	comprobarErrores(resul, msj);
	free(msj);
	return data;
}

void agregar_a_serializacion(void* magic, int* desplazamiento, void* data, int tamanio){
	memcpy(magic + (*desplazamiento), data, tamanio);
	(*desplazamiento) += tamanio;
}

int tamanioTarea(struct Tarea* tarea){
	int tamanio = 0;
	tamanio += strlen(tarea->nombre) + 1;
	tamanio += sizeof(int) * 5;
	return tamanio;
}

int tamanioListaTareas(t_list* tareas){
	int tamanio = 0;
	for(int i = 0; i < tareas->elements_count; i++){
		char* tareaActual = list_get(tareas,i);
		tamanio += strlen(tareaActual) + 1;
		tamanio += sizeof(int);
	}
	return tamanio;
}

int getValorSemaforo(sem_t semaforo){
	int valor;
	sem_getvalue(&semaforo, &valor);
	return valor;
}
