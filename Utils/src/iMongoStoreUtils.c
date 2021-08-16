/*
 * iMongoStoreUtils.c
 *
 *  Created on: 1 jul. 2021
 *      Author: utnso
 */

#include "iMongoStoreUtils.h"
extern int errno ;
int errnum;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * GENERAL
 */

void verificarErrores(int error){
	if (error == -1) {
		errnum = errno;
		fprintf(stderr, "Value of errno: %d\n", errno);
		perror("Error printed by perror");
		fprintf(stderr, "Error opening file: %s\n", strerror( errnum ));
	}
}

char* rutaPuntoMontaje(int longitudNombreArchivo){
	char* pathArchivo 			= malloc(strlen(puntoMontaje) + longitudNombreArchivo + 1);
	strcpy(pathArchivo, puntoMontaje);
	return pathArchivo;
}

int ultimoBloque(FILE* archivo){
	t_list* bloques 	= obtenerBloquesFile(archivo);
	int bloque 			= obtenerIntLista(bloques, bloques->elements_count-1, 0);
	liberarListaBloques(bloques);
	return bloque;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * SUPERBLOCK
 */



void borrarFileSystemAnterior(){
	char* pathBitacoras = rutaBitacoras(0);
	t_list* bitacoras 	= obtenerNombresArchivos(pathBitacoras);
	for(int i = 0; i < bitacoras->elements_count; i++){
		char* pathBitacora = string_duplicate(pathBitacoras);
		string_append(&pathBitacora,list_get(bitacoras,i));
		remove(pathBitacora);
		free(pathBitacora);
	}
	while(bitacoras->elements_count != 0){
		char* bitacora = list_remove(bitacoras,0);
		free(bitacora);
	}
	list_destroy(bitacoras);
	remove(pathBitacoras);
	free(pathBitacoras);
	char* pathFiles 	=  ruteFiles(0);
	t_list* files = obtenerNombresArchivos(pathFiles);
	for(int i = 0; i < files->elements_count; i++){
		char* pathFile = string_duplicate(pathFiles);
		string_append(&pathFile,list_get(files,i));
		remove(pathFile);
		free(pathFile);
	}
	while(files->elements_count != 0){
		char* file = list_remove(files,0);
		free(file);
	}
	list_destroy(files);
	remove(pathFiles);
	free(pathFiles);
	char* pathSuperBloque = rutaPuntoMontaje(strlen("/SuperBloque.ims"));
	strcat(pathSuperBloque, "/SuperBloque.ims");
	remove(pathSuperBloque);
	free(pathSuperBloque);
	char* pathBlocks 	= rutaPuntoMontaje(strlen("/Blocks.ims"));
	strcat(pathBlocks, "/Blocks.ims");
	remove(pathBlocks);
	free(pathBlocks);
}

void inicializarFileSystem(char* borrarFS){
	puntoMontaje    	= config_get_string_value(config,"PUNTO_MONTAJE");

	char* pathSuperBloque = rutaPuntoMontaje(strlen("/SuperBloque.ims"));
	strcat(pathSuperBloque, "/SuperBloque.ims");
	uint32_t blockSize 		= config_get_int_value(config,"BLOCK_SIZE");
	uint32_t blockCount 	= config_get_int_value(config,"BLOCK_COUNT");

	if(strcmp(borrarFS, "S") == 0){
		borrarFileSystemAnterior();
	}

	FILE* superBloque 			= fopen(pathSuperBloque,"r");
	if (superBloque == NULL){
		superBloque 			= fopen(pathSuperBloque,"w");

		char* strBlockSize 		= string_new();
		string_append_with_format(&strBlockSize, "%s%d\n", "BLOCKSIZE=", blockSize);
		fwrite(strBlockSize,strlen(strBlockSize),1,superBloque);
		free(strBlockSize);

		char* strBlockCount 	= string_new();
		string_append_with_format(&strBlockCount, "%s%d\n", "BLOCK_COUNT=", blockCount);
		fwrite(strBlockCount,strlen(strBlockCount),1,superBloque);
		free(strBlockCount);

		char* bitmapInicial = string_repeat('0', blockCount);
		char* strBitmap		= string_new();
		string_append_with_format(&strBitmap, "%s%s\n", "BITMAP=", bitmapInicial);
		fwrite(strBitmap,strlen(strBitmap),1,superBloque);
		free(strBitmap);
		free(bitmapInicial);

		char* pathFiles 	= rutaPuntoMontaje(strlen("/Files"));
		strcat(pathFiles, "/Files");
		mkdir(pathFiles, 0777);
		free(pathFiles);

		char* pathBitacoras = rutaPuntoMontaje(strlen("/Files/Bitacoras"));
		strcat(pathBitacoras, "/Files/Bitacoras");
		mkdir(pathBitacoras, 0777);
		free(pathBitacoras);
	}

	char* pathBlocks 	= rutaPuntoMontaje(strlen("/Blocks.ims"));
	strcat(pathBlocks, "/Blocks.ims");
	int fileDescriptor 	= open(pathBlocks, O_CREAT | O_RDWR, 0777);
	ftruncate(fileDescriptor, blockSize*blockCount);
	mapeadoBlocks 		= mmap(NULL, blockSize*blockCount, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fileDescriptor, 0);
	close(fileDescriptor);

	copiaBlocks 		= malloc(blockSize*blockCount);
	memcpy(copiaBlocks,mapeadoBlocks,blockSize*blockCount);
	free(pathBlocks);

	fclose(superBloque);
	free(pathSuperBloque);
	listaMutexBitacoras = list_create();
}

int proximoBloqueLibre(){
	int resultado 		= -1;

	int cantidadBloques = obtenerCantidadBloquesSuperbloque();
	pthread_mutex_lock(&mutexBitmap);
	char* punteroABit 	= obtenerBitmap();

	for(int i = 0; i < cantidadBloques; i++){
		if(punteroABit[i] == '0' && resultado == -1){
			resultado = i;
		}
	}

	if(resultado != -1) actualizarBitmap(resultado, '1');
	pthread_mutex_unlock(&mutexBitmap);
	free(punteroABit);

	return resultado;
}

void imprimirBitmap(){
	int cantidadBloques = obtenerCantidadBloquesSuperbloque();
	FILE* superBloque 	= abrirSuperBloque();
	pthread_mutex_lock(&mutexBitmap);
	char* punteroABit 	= obtenerBitmap();
	pthread_mutex_unlock(&mutexBitmap);
	fclose(superBloque);

	for(int i = 0; i < cantidadBloques; i++){
		printf("%c",punteroABit[i]);
	}
	free(punteroABit);
	printf("\n");
}

int obtenerCantidadBloquesSuperbloque(){
	FILE* superBloque 				= abrirSuperBloque();
	char* punteroCantidadBloques 	= obtenerDeAux(superBloque,1,0);
	int cantidadBloques 			= strtol(punteroCantidadBloques, NULL, 10);
	free(punteroCantidadBloques);
	fclose(superBloque);
	return cantidadBloques;
}

int obtenerBlockSize(){
	FILE* superBloque 		= abrirSuperBloque();
	char* punteroBlockSize 	= obtenerDeAux(superBloque,0,0);
	int blockSize 			= strtol(punteroBlockSize, NULL, 10);
	free(punteroBlockSize);
	fclose(superBloque);
	return blockSize;
}


char* obtenerBitmap(){
	FILE* superBloque 		= abrirSuperBloque();
	char* punteroBitmap 	= obtenerDeAux(superBloque,2,0);
	fclose(superBloque);
	return punteroBitmap;
}


void actualizarBitmap(int index, char valor){

	char* bitmap 		= obtenerBitmap();
	bitmap[index] 	= valor;
	char* strBitmap 	= string_new();
	string_append_with_format(&strBitmap, "%s%s\n", "BITMAP=", bitmap);
	free(bitmap);

	FILE* superBloque 	= abrirSuperBloque();
	actualizarLineaArchivo(superBloque, 3, strBitmap, 3);
	fclose(superBloque);
}

FILE* abrirSuperBloque(){
	char* pathSuperBloque 	= rutaPuntoMontaje(strlen("/SuperBloque.ims"));
	strcat(pathSuperBloque, "/SuperBloque.ims");
	FILE* superBloque 		= fopen(pathSuperBloque,"r+");
	free(pathSuperBloque);
	return superBloque;
}

int bloquesLibres(){
	int bloquesLibres 	= 0;
	int cantidadBloques = obtenerCantidadBloquesSuperbloque();
	pthread_mutex_lock(&mutexBitmap);
	char* punteroABit 	= obtenerBitmap();
	for(int i = 0; i < cantidadBloques; i++){
		if(punteroABit[i] == '0'){
			bloquesLibres++;
		}
	}
	pthread_mutex_unlock(&mutexBitmap);
	free(punteroABit);

	return bloquesLibres;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * BLOCKS
 */
int abrirBlock(){
	char* pathBloque 	= rutaPuntoMontaje(strlen("/Blocks.ims"));
	strcat(pathBloque, "/Blocks.ims");
	int fd 				= open(pathBloque, O_CREAT|O_RDWR, 0777);
	free(pathBloque);
	return fd;
}

void escribirBlocks(char caracter, int cantidad, int posicionAEscribir){
	char* cadenaAEscribir = string_repeat(caracter, cantidad);
	pthread_mutex_lock(&mutexCopiaBloques);
	memcpy(copiaBlocks+posicionAEscribir,cadenaAEscribir,cantidad);
	pthread_mutex_unlock(&mutexCopiaBloques);
	free(cadenaAEscribir);
}

void imprimirBlocks(){
	int cantidadBloques = obtenerCantidadBloquesSuperbloque();
	int tamanioBloque 	= obtenerBlockSize();
	int fd 				= abrirBlock();
	char* bloques 		= malloc(tamanioBloque * cantidadBloques);
	lseek(fd, 0 , SEEK_SET);
	int resultadoRead = read(fd, bloques, tamanioBloque * cantidadBloques);
	verificarErrores(resultadoRead);
	close(fd);
	printf("----- BLOQUES: -----\n");
	for(int i = 0; i < cantidadBloques; i++){
		for(int j = 0; j < tamanioBloque; j++){
			if(bloques[(i * tamanioBloque) + j] == '\0'){
				printf("X");
			} else {
				printf("%c", bloques[(i * tamanioBloque) + j]);
			}
		}
		printf("\n");
	}
	printf("\n");
	free(bloques);
}

void escribirCaracteres(FILE* archivo, char* pathArchivo, int cantidadAEscribir, int bloque, int size, char caracterLlenado){
	int blockSize 			= obtenerBlockSize();
	int restoBloque 		= (size % blockSize);
	int posicionAEscribir 	= restoBloque + (bloque * blockSize);
	int siguienteBloque;
	if(cantidadAEscribir <= (blockSize - restoBloque)){
		escribirBlocks( caracterLlenado, cantidadAEscribir, posicionAEscribir);
	} else {
		int cantidadRestante = cantidadAEscribir - (blockSize - restoBloque);
		escribirBlocks( caracterLlenado, cantidadAEscribir - cantidadRestante, posicionAEscribir);
		if(caracterLlenado == '\0'){
			siguienteBloque = siguienteBloqueFile(archivo,bloque);
		} else siguienteBloque = agregarBloqueAFile(archivo, pathArchivo);
		if(siguienteBloque != -1) escribirCaracteres(archivo, pathArchivo, cantidadRestante, siguienteBloque, size + (cantidadAEscribir - cantidadRestante), caracterLlenado);
	}
}

void escribirLog(FILE* bitacora, char* log, int size, int bloque){
	int blockSize 			= obtenerBlockSize();
	int restoBloque 		= (size % blockSize);
	int posicionAEscribir 	= restoBloque + (bloque * blockSize);

	int cantidadAEscribir = strlen(log);



	if(cantidadAEscribir <= (blockSize - restoBloque)){
		pthread_mutex_lock(&mutexCopiaBloques);
		memcpy(copiaBlocks+posicionAEscribir,log,cantidadAEscribir);
		pthread_mutex_unlock(&mutexCopiaBloques);
		free(log);
	} else {
		int cantidadRestante 	= cantidadAEscribir - (blockSize - restoBloque);
		char* logRestante 		= malloc(cantidadRestante + 1);
		memcpy(logRestante, log + (cantidadAEscribir - cantidadRestante), cantidadRestante + 1);
		pthread_mutex_lock(&mutexCopiaBloques);
		memcpy(copiaBlocks+posicionAEscribir,log,cantidadAEscribir - cantidadRestante);
		pthread_mutex_unlock(&mutexCopiaBloques);
		int siguienteBloque 	= agregarBloqueABitacora(bitacora);
		free(log);
		if(siguienteBloque != -1) escribirLog(bitacora, logRestante, size + (blockSize - restoBloque), siguienteBloque);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * FILES
 */


char* ruteFiles(int bytes){
	char* pathFiles = rutaPuntoMontaje(7 + bytes);
	strcat(pathFiles, "/Files/");
	return pathFiles;
}

char* rutaArchivo(char* nombreArchivo){
	char* pathFile = ruteFiles(strlen(nombreArchivo) + 4);
	strcat(pathFile, nombreArchivo);
	strcat(pathFile, ".ims");
	return pathFile;
}

int agregarCaracteresA(char* nombreArchivo, int cantidad, int pathCompleto){
	int resultado 		= -1;
	char* pathArchivo 	= (pathCompleto) ? nombreArchivo : rutaArchivo(nombreArchivo);

	FILE* archivo = fopen(pathArchivo,"r+") ;
	if (archivo == NULL){
		char caracterLlenado 	= caracterArchivo(nombreArchivo);
		archivo 				= crearArchivoFile(pathArchivo, caracterLlenado);
	}

	char caracterLlenado 	= obtenerCaracterLlenadoFile(archivo);

	int size 				= obtenerSizeFile(archivo);

	int blockCount			= obtenerCantidadDeBloquesFile(archivo);

	int tamanioBloque 		= obtenerBlockSize();

	int bloque;
	if(blockCount == 0 || (size % tamanioBloque == 0 )){
		bloque = agregarBloqueAFile(archivo, pathArchivo);
	} else {
		bloque = ultimoBloque(archivo);
	}

	if(bloque != -1){
		escribirCaracteres(archivo, pathArchivo, cantidad, bloque, size, caracterLlenado);
		actualizarFileSize(archivo, pathArchivo, cantidad, 1);
		resultado = 0;
	}
	free(pathArchivo);
	fclose(archivo);

	return resultado;
}

int borrarCaracteresA(char* nombreArchivo, int cantidad){

    char* pathArchivo = rutaArchivo(nombreArchivo);
    int resultado, cantidadABorrar;

    FILE* archivo = fopen(pathArchivo,"r+") ;
    if (archivo == NULL){
        free(pathArchivo);
        return -1;
    }

    int sizeViejo         = obtenerSizeFile(archivo);
    int cantidadTotal     = sizeViejo;

    if (cantidad > cantidadTotal) {
        cantidadABorrar = cantidadTotal;
        resultado         = 1;
    } else if(cantidad == 0){
        cantidadABorrar = sizeViejo;
        resultado         = 0;
    } else {
        cantidadABorrar = cantidad;
        resultado         = 0;
    }

    actualizarFileSize(archivo, pathArchivo, cantidadABorrar*-1, 1);
    int sizeNuevo             = obtenerSizeFile(archivo);
    int primerBloque         = primerBloqueAEscribir(archivo);

    escribirCaracteres(archivo, pathArchivo, cantidadABorrar,primerBloque,sizeNuevo,'\0');
    borrarBloquesFile(archivo, pathArchivo);
    free(pathArchivo);
    fclose(archivo);

    return resultado;
}

void borrarBloquesFile(FILE* archivo, char* pathArchivo){
	int tamanioFile					= obtenerSizeFile(archivo);
	int cantidadBloques 			= obtenerCantidadDeBloquesFile(archivo);
	int tamanioBloque				= obtenerBlockSize();
	int nuevaCantidadBloques 		= floor(tamanioFile/tamanioBloque);
	if(tamanioFile % tamanioBloque) nuevaCantidadBloques++;
	int cantidadDeBloquesABorrar 	= cantidadBloques-nuevaCantidadBloques;
	if(cantidadDeBloquesABorrar > 0){
		eliminarBloquesFile(archivo, pathArchivo, cantidadDeBloquesABorrar);
	}
}

void eliminarBloquesFile(FILE* archivo, char* pathArchivo, int bloquesABorrar){
    t_list* bloques            = obtenerBloquesFile(archivo);
    for( int i = 0; i < bloquesABorrar; i++){
        int bloqueEliminado         = obtenerIntLista(bloques, bloques->elements_count-1, 1);
        pthread_mutex_lock(&mutexBitmap);
        actualizarBitmap(bloqueEliminado, '0');
        pthread_mutex_unlock(&mutexBitmap);
    }
    int cantidadBloques = bloques->elements_count;
    actualizarBloquesFile(archivo, bloques, pathArchivo);
    if(cantidadBloques == 0){
        remove(pathArchivo);
    }
}


int primerBloqueAEscribir(FILE* archivo){
	int tamanioFile		= obtenerSizeFile(archivo);
	t_list* bloques		= obtenerBloquesFile(archivo);
	int tamanioBloque	= obtenerBlockSize();
	int posicionBloque 	= floor(tamanioFile/tamanioBloque);
	int bloque			= obtenerIntLista(bloques, posicionBloque,0);
	liberarListaBloques(bloques);
	return bloque;
}

void liberarListaBloques(t_list* bloques){
	while(bloques->elements_count != 0){
		char* bloque = list_remove(bloques, 0);
		free(bloque);
	}
	list_destroy(bloques);
}

char obtenerCaracterLlenadoFile(FILE* archivo){
	char* punteroCaracter 	= obtenerDeAux(archivo,0,0);
	char caracter 			= (char)punteroCaracter[0];
	free(punteroCaracter);
	return caracter;
}

int obtenerSizeFile(FILE* archivo){
	char* punteroSize 	= (char*)obtenerDeAux(archivo, 1, 0);
	int size 			= strtol(punteroSize,NULL,10);
	free(punteroSize);
	return size;
}

int obtenerCantidadDeBloquesFile(FILE* archivo){
	char* punteroCantidadBloques = obtenerDeAux(archivo,2,0);
	int cantidadBloques			= strtol(punteroCantidadBloques,NULL,10);
	free(punteroCantidadBloques);
	return cantidadBloques;
}

t_list* obtenerBloquesFile(FILE* archivo){
	char* strBloques	= (char*)obtenerDeAux(archivo, 3, 0);
	t_list* bloques 	= generarListaBloques(strBloques);
	free(strBloques);
	return bloques;
}

char* obtenerMD5(FILE* archivo){
	char* md5 = (char*)obtenerDeAux(archivo,4,0);
	return md5;
}

void* obtenerDe(FILE* archivo, int seek, int size, int cantidad){
	fseek(archivo,seek,SEEK_SET);
	void* data = malloc(size*cantidad);
	int leido = fread(data,size,cantidad,archivo);

	if(leido == 0){
		free(data);
		return NULL;
	} else {
		return data;
	}
}

void* obtenerDeAux(FILE* archivo, int linea, int flagCompleto){
	int contador 		= 0;
	int leido 			= 0;
	int inicio			= -1;
	int seek 			= desplazamientoLinea(archivo, linea);
	fseek(archivo, seek, SEEK_SET);
	char caracter 		= fgetc(archivo);
	while(caracter != '\n' && !feof(archivo)){
		if(flagCompleto){
			if(inicio == -1) inicio = contador;
			leido++;
		}
		if(caracter == '=') flagCompleto = 1;
		caracter 		= fgetc(archivo);
		contador++;
	}

	if(leido == 0){
		return NULL;
	} else {
		char* data = malloc(contador - inicio + 2);
		fseek(archivo, seek + inicio, SEEK_SET);
		fgets(data,(contador - inicio)+1, archivo);
		return data;
	}
}

FILE* crearArchivoFile(char* pathArchivo, char caracterLlenado){
	FILE* archivo = fopen(pathArchivo,"w+");

	char* caracterDeLlenado = string_new();
	string_append_with_format(&caracterDeLlenado, "%s%c\n", "CARACTER_LLENADO=",caracterLlenado);
	fwrite(caracterDeLlenado,strlen(caracterDeLlenado),1,archivo);
	free(caracterDeLlenado);

	char* size = string_new();
	string_append_with_format(&size, "%s\n", "SIZE=0");
	fwrite(size,strlen(size),1,archivo);
	free(size);

	char* blockCount = string_new();
	string_append_with_format(&blockCount, "%s\n", "BLOCK_COUNT=0");
	fwrite(blockCount,strlen(blockCount),1,archivo);
	free(blockCount);

	char* bloques = string_new();
	string_append_with_format(&bloques, "%s\n", "BLOCKS=[]");
	fwrite(bloques,strlen(bloques),1,archivo);
	free(bloques);

	char* MD5 = string_new();
	char* md5 = generarMD5(archivo);
	string_append_with_format(&MD5, "%s%s\n", "MD5_ARCHIVO=",md5);
	fwrite(MD5,strlen(MD5),1,archivo);
	free(MD5);
	free(md5);

	return archivo;
}

char* generarMD5(FILE* archivo){
	FILE* archivoAux 		= fopen("datos.txt","w+");
	int size				= obtenerSizeFile(archivo);
	int blockCount			= obtenerCantidadDeBloquesFile(archivo);
	t_list* bloques			= obtenerBloquesFile(archivo);
	fwrite(&size,sizeof(int),1,archivoAux);
	fwrite(&blockCount,sizeof(int),1,archivoAux);
	for(int i = 0; i < bloques->elements_count; i++){
		int bloque = strtol(list_get(bloques,i),NULL,10);
		fwrite(&bloque,sizeof(int),1,archivoAux);
	}
	liberarListaBloques(bloques);
	fclose(archivoAux);

	pthread_mutex_lock(&mutexMD5);
	system("md5sum datos.txt > md5.txt");
	remove("datos.txt");

	FILE* fileMD5 = fopen("md5.txt", "r");
	char caracter = fgetc(fileMD5);
	int contador = 0;
	while(caracter != ' '){
		contador++;
		caracter = fgetc(fileMD5);
	}
	char* MD5 = malloc(contador+1);
	fseek(fileMD5, 0, SEEK_SET);
	fgets(MD5, contador+1, fileMD5);
	fclose(fileMD5);
	remove("md5.txt");
	pthread_mutex_unlock(&mutexMD5);

	return MD5;
}

char caracterArchivo(char* nombreArchivo){
	if(strlen(nombreArchivo) == 7 && strcmp(nombreArchivo, "Oxigeno") == 0) return 'O';
	if(strlen(nombreArchivo) == 6 && strcmp(nombreArchivo, "Comida")  == 0) return 'C';
	if(strlen(nombreArchivo) == 6 && strcmp(nombreArchivo, "Basura")  == 0) return 'B';
	return 0;
}


int agregarBloqueAFile(FILE* archivo, char* pathArchivo){
	int bloque = proximoBloqueLibre();

	if(bloque != -1){
		t_list* bloques = obtenerBloquesFile(archivo);
		char* strBloque = string_itoa(bloque);
		list_add(bloques, strBloque);

		actualizarBloquesFile(archivo, bloques, pathArchivo);
	}

	return bloque;
}

void actualizarBlockCount(FILE* archivo, int bloquesNuevos){
	char* blockCountNuevo 		= string_itoa(bloquesNuevos);
	char* strBlockCount 		= string_new();
	string_append(&strBlockCount, "BLOCK_COUNT=");
	string_append_with_format(&strBlockCount, "%s\n", blockCountNuevo);
	free(blockCountNuevo);

	actualizarLineaArchivo(archivo,3,strBlockCount,5);
}

void actualizarBloquesFile(FILE* archivo, t_list* bloques, char* pathArchivo){
	char* strBlocks = generarMetadataBloques(bloques);
	string_append(&strBlocks, "\n");

	actualizarBlockCount(archivo,bloques->elements_count);
	liberarListaBloques(bloques);
	actualizarLineaArchivo(archivo, 4, strBlocks, 5);
	actualizarMD5(archivo,pathArchivo);
}

char* generarMetadataBloques(t_list* bloques){
	char* strBlocks 		= string_new();
	string_append(&strBlocks,"BLOCKS=[");
	for(int i = 0; i < bloques->elements_count; i++){
		char* strBloque = (char*)list_get(bloques, i);
		string_append(&strBlocks, strBloque);
		if(i+1 != bloques->elements_count){
			string_append(&strBlocks, ",");
		}
	}
	string_append_with_format(&strBlocks, "%s", "]");
	return strBlocks;
}

void actualizarFileSize(FILE* archivo, char* pathArchivo, int sizeNuevo, int sumar){

	int viejoSize 	= obtenerSizeFile(archivo);

	char* strNuevoSize 	= (sumar) ? string_itoa(viejoSize + sizeNuevo) : string_itoa(sizeNuevo);
	char* strSize 		= string_new();
	string_append(&strSize, "SIZE=");
	string_append_with_format(&strSize, "%s\n", strNuevoSize);
	free(strNuevoSize);

	actualizarLineaArchivo(archivo, 2, strSize, 5);

	actualizarMD5(archivo,pathArchivo);

}

void actualizarMD5(FILE* archivo, char* pathArchivo){
	char* MD5 		= generarMD5(archivo);
	char* md5 		= string_new();
	string_append(&md5, "MD5_ARCHIVO=");
	string_append_with_format(&md5, "%s\n", MD5);
	int stringLength	= string_length(md5);
	int seek 	 		= actualizarLineaArchivo(archivo,5,md5,5);
	int tamanioTotal 	= seek + stringLength;
	free(MD5);
	truncate(pathArchivo, tamanioTotal);
}

int siguienteBloqueFile (FILE* archivo, int bloqueActual){
	char* strBloque 	= string_itoa(bloqueActual);
	t_list* bloques 	= obtenerBloquesFile(archivo);
	int bloqueSiguiente = -1;
	for(int i = 0; i < bloques->elements_count; i++){
		char* bloque = list_get(bloques, i);
		if(strcmp(strBloque, bloque) == 0){
			char* strSiguienteBloque 	= list_get(bloques, i+1);
			bloqueSiguiente 			= strtol(strSiguienteBloque, NULL, 10);
			break;
		}
	}
	free(strBloque);
	liberarListaBloques(bloques);

	return bloqueSiguiente;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * BITACORAS
 */

char* rutaBitacoras(int bytes){
	char* pathArchivo = rutaPuntoMontaje(strlen("/Files/Bitacoras/") + bytes);
	strcat(pathArchivo, "/Files/Bitacoras/");
	return pathArchivo;
}

char* nombreBitacora(int tid){
	char* tripulante 		= "Tripulante";
	char* strTid 			= string_itoa(tid);
	char* nombreBitacora 	= malloc(strlen(tripulante) + strlen(strTid) + 1);
	strcpy(nombreBitacora, tripulante);
	strcat(nombreBitacora, strTid);
	free(strTid);
	return nombreBitacora;
}

char* rutaBitacora(int tid){
	char* nombreArchivo = nombreBitacora(tid);
	char* pathBitacora 	= rutaBitacoras(strlen(nombreArchivo) + strlen(".ims"));
	strcat(pathBitacora, nombreArchivo);
	free(nombreArchivo);
	strcat(pathBitacora, ".ims");
	return pathBitacora;
}

void crearArchivoBitacora(int tid){
	char* pathBitacora 	= rutaBitacora(tid);
	FILE* bitacora 		= fopen(pathBitacora,"r");
	if(bitacora == NULL){
		bitacora 		= fopen(pathBitacora,"w+");

		char* size = string_new();
		string_append_with_format(&size, "%s\n", "SIZE=0");
		fwrite(size,strlen(size),1,bitacora);
		free(size);

		char* bloques = string_new();
		string_append_with_format(&bloques, "%s\n", "BLOCKS=[]");
		fwrite(bloques,strlen(bloques),1,bitacora);
		free(bloques);
	}
	sem_t semaforoBitacora;
	sem_init(&semaforoBitacora, 0, 1);
	struct relacionTIDMutex* mutexTid = malloc(sizeof(struct relacionTIDMutex));
	mutexTid->mutex = semaforoBitacora;
	mutexTid->tid = tid;
	pthread_mutex_lock(&mutexListaMutex);
	list_add(listaMutexBitacoras,mutexTid);
	pthread_mutex_unlock(&mutexListaMutex);

	fclose(bitacora);
	free(pathBitacora);
}

t_list* obtenerBloquesBitacora(FILE* bitacora){
	char* strBloques 	= (char*)obtenerDeAux(bitacora, 1, 0);
	t_list* bloques 	= generarListaBloques(strBloques);
	free(strBloques);
	return bloques;
}

t_list* generarListaBloques(char* strBloques){
	t_list* bloques = list_create();
	for(int i = 1; strBloques[i-1] != ']'; i++){
		int cantidadDecimales 	= 0;
		while(strBloques[i] != ',' && strBloques[i] != ']'){
			cantidadDecimales++;
			i++;
		}
		if(cantidadDecimales){
			char* strNumeroBloque		= string_substring(strBloques, i-cantidadDecimales, cantidadDecimales);
			list_add(bloques, strNumeroBloque);
		}
	}
	return bloques;
}

int ultimoBloqueBitacora(FILE* bitacora){
	t_list* bloques = obtenerBloquesBitacora(bitacora);
	int bloque 		= -1;
	if(bloques->elements_count != 0){
		char* punteroBloque = (char*)list_remove(bloques, bloques->elements_count - 1);
		bloque 				= strtol(punteroBloque,NULL,10);
		free(punteroBloque);
	}
	liberarListaBloques(bloques);
	return bloque;
}

int obtenerSizeBitacora(FILE* bitacora){
	char* punteroSize 	= (char*)obtenerDeAux(bitacora, 0, 0);
	int size 			= strtol(punteroSize,NULL,10);
	free(punteroSize);
	return size;
}

int tamanioDisponibleBlock(int bloque){
	int tamanioBloque 	= obtenerBlockSize();
	int bytesOcupados 	= 0;
	char* strBloque 	= malloc(tamanioBloque);

	pthread_mutex_lock(&mutexCopiaBloques);
	memcpy(strBloque, copiaBlocks + bloque*tamanioBloque, tamanioBloque);
	pthread_mutex_unlock(&mutexCopiaBloques);

	for(int i = 0; i < tamanioBloque; i++){
		if(strBloque[i] != '\0') bytesOcupados++;
	}
	free(strBloque);

	return tamanioBloque - bytesOcupados;
}


int agregarBloqueABitacora(FILE* bitacora){
	int bloque = proximoBloqueLibre();

	if(bloque != -1){
		t_list* bloques = obtenerBloquesBitacora(bitacora);
		char* strBloque = string_itoa(bloque);
		list_add(bloques, strBloque);

		actualizarBloquesBitacora(bitacora, bloques);
	}

	return bloque;
}

void actualizarBloquesBitacora(FILE* bitacora, t_list* bloques){

	char* strBlocks 		= generarMetadataBloques(bloques);
	liberarListaBloques(bloques);

	actualizarLineaArchivo(bitacora, 2, strBlocks, 2);
}

int desplazamientoLinea(FILE* archivo, int numeroLinea){
	int seek = 0;
	fseek(archivo, seek, SEEK_SET);
	for(int i = 0; i < numeroLinea; i++){
		char caracter = fgetc(archivo);
		while((caracter != '\n') && (caracter != EOF)){
			seek++;
			caracter = fgetc(archivo);
		}
		if(caracter != EOF){
			seek++;
		}
	}
	return seek;
}

int actualizarLineaArchivo(FILE* archivo, int numeroLinea, char* nuevaLinea, int cantidadLineas){
	char* restante		= obtenerRestoArchivo(archivo, numeroLinea, cantidadLineas-1);
	int seek			= desplazamientoLinea(archivo, numeroLinea-1);
	fseek(archivo, seek, SEEK_SET);
	fwrite(nuevaLinea, strlen(nuevaLinea), 1, archivo);
	fwrite(restante, strlen(restante), 1, archivo);
	free(restante);
	free(nuevaLinea);
	return seek;
}

void actualizarSizeBitacora(int nuevoSize, FILE* bitacora) {
	char* strNuevoSize 	= string_itoa(nuevoSize);
	char* strSize 		= string_new();
	string_append(&strSize, "SIZE=");
	string_append_with_format(&strSize, "%s\n", strNuevoSize);
	free(strNuevoSize);

	actualizarLineaArchivo(bitacora, 1, strSize, 2);
}

char* obtenerRestoArchivo(FILE* archivo, int fila, int cantidadFilas){
	char* restante 		= string_new();
	for(int i = fila ; i <= cantidadFilas; i++){
		char* filaActual 	= (char*)obtenerDeAux(archivo, i , 1);
		string_append_with_format(&restante, "%s\n", filaActual);
		free(filaActual);
	}
	return restante;
}

struct relacionTIDMutex* obtenerMutexTid(int tid){
	pthread_mutex_lock(&mutexListaMutex);
	int tamanioLista = listaMutexBitacoras->elements_count;
	pthread_mutex_unlock(&mutexListaMutex);

	for(int i = 0; i < tamanioLista; i++){
		pthread_mutex_lock(&mutexListaMutex);
		struct relacionTIDMutex* mutexTid = list_get(listaMutexBitacoras,i);
		pthread_mutex_unlock(&mutexListaMutex);
		if(mutexTid->tid == tid) return mutexTid;
	}
	return NULL;
}
void waitSemaforoBitacora(int tid){
	struct relacionTIDMutex* mutexTid = obtenerMutexTid(tid);
	sem_wait(&(mutexTid->mutex));
}
void postSemaforoBitacora(int tid){
	struct relacionTIDMutex* mutexTid = obtenerMutexTid(tid);
	sem_post(&(mutexTid->mutex));
}
int existeTripulante(int tid){
	struct relacionTIDMutex* mutexTid = obtenerMutexTid(tid);
	return mutexTid != NULL;
}

int actualizarBitacora(int tid, char* log){
	char* pathBitacora 		= rutaBitacora(tid);
	int resultado 			= -1;

	waitSemaforoBitacora(tid);

	FILE* bitacora 			= fopen(pathBitacora,"r+");
	free(pathBitacora);
	int ultimoBloque 	= ultimoBloqueBitacora(bitacora);
	if(ultimoBloque == -1){
		ultimoBloque = agregarBloqueABitacora(bitacora);
	} else if (tamanioDisponibleBlock(ultimoBloque) == 0){
		ultimoBloque = agregarBloqueABitacora(bitacora);
	}

	if(ultimoBloque != -1){
		int size 			= obtenerSizeBitacora(bitacora);
		int nuevoSize		= size + strlen(log);

		escribirLog(bitacora, log, size, ultimoBloque);
		actualizarSizeBitacora(nuevoSize, bitacora);
		resultado = 0;
	}
	fclose(bitacora);

	postSemaforoBitacora(tid);

	return resultado;
}


void logearMovimiento(int tid, int posicionXAntes, int posicionYAntes, int posicionXDespues, int posicionYDespues){
	char* primeraParte 	= "Me muevo de ";
	char* segundaParte 	= "|";
	char* terceraParte 	= " a ";
	char* cuartaParte 	= "|";
	char* quintaParte 	= ".";
	char* posXAntes 	= string_itoa(posicionXAntes);
	char* posYAntes 	= string_itoa(posicionYAntes);
	char* posXDespues	= string_itoa(posicionXDespues);
	char* posYDespues 	= string_itoa(posicionYDespues);

	char* log = malloc(strlen(primeraParte) + strlen(segundaParte) + strlen(terceraParte) + strlen(cuartaParte) + strlen(quintaParte) + strlen(posXAntes)+ strlen(posYAntes)+ strlen(posXDespues)+ strlen(posYDespues) + 1);


	strcpy(log, primeraParte);
	strcat(log, posXAntes);
	strcat(log, segundaParte);
	strcat(log, posYAntes);
	strcat(log, terceraParte);
	strcat(log, posXDespues);
	strcat(log, cuartaParte);
	strcat(log, posYDespues);
	strcat(log, quintaParte);

	logear(tid, log);
	free(posXAntes);
	free(posYAntes);
	free(posXDespues);
	free(posYDespues);
}

void logearComienzoDeTarea(int tid, char* nombreTarea){
	char* inicio 	= "Se comienza la tarea ";

	char* log 		= malloc(strlen(inicio) + strlen(nombreTarea) + strlen(".") + 1);

	strcpy(log,inicio);
	strcat(log,nombreTarea);
	free(nombreTarea);
	strcat(log,".");
	logear(tid, log);
}
void logearFinalizacionDeTarea(int tid, char* nombreTarea){
	char* inicio 	= "Se finaliza la tarea ";

	char* log 		= malloc(strlen(inicio) + strlen(nombreTarea) + strlen(".") + 1);

	strcpy(log,inicio);
	strcat(log,nombreTarea);
	free(nombreTarea);
	strcat(log,".");

	logear(tid, log);
}
void logearCorridaEnPanico(int tid){
	char* log = malloc(strlen("Se corre en panico hacia la ubicacion del sabotaje.") + 1);
	strcpy(log, "Se corre en panico hacia la ubicacion del sabotaje.");
	logear(tid, log);
}
void logearSabotajeResuelto(int tid){
	char* log = malloc(strlen("Se resuelve el sabotaje.") + 1);
	strcpy(log, "Se resuelve el sabotaje.");
	logear(tid, log);
}

void logear(int tid, char* log){
	int resultado = actualizarBitacora(tid,log);
	if(resultado == -1){
		char* log = string_new();
		string_append(&log, "No hay suficiente espacio, no se pudo actualizar la Bitacora");
		logearArchivoIMongo(log);
	}

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * CONEXION TRIPULANTES
 */

int recibirTID(int socket){
	int* punteroTid 	= (int*)deserializar(socket, sizeof(int), "TID");
	int tid 			= *punteroTid;
	free(punteroTid);
	return tid;
}

void recibirMovimientoYLoguear(int socket, int tid){
	int* posicionXDespues 	= (int*)deserializar(socket, sizeof(int), "Posicion X Nueva");
	int* posicionYDespues 	= (int*)deserializar(socket, sizeof(int), "Posicion Y Nueva");
	int* posicionXAntes 	= (int*)deserializar(socket, sizeof(int), "Posicion X Vieja");
	int* posicionYAntes 	= (int*)deserializar(socket, sizeof(int), "Posicion Y Vieja");

	send(socket,posicionXDespues,sizeof(int),0);

	logearMovimiento(tid,*posicionXAntes,*posicionYAntes,*posicionXDespues,*posicionYDespues);
	free(posicionXDespues);
	free(posicionYDespues);
	free(posicionXAntes);
	free(posicionYAntes);
}

char* deserializarTarea(int socket){
	int* longitudNombreTarea 	= (int*)deserializar(socket, sizeof(int), "Longitud Nombre Tarea");
	char* nombreTarea			= (char*)deserializar(socket, *longitudNombreTarea + 1, "Nombre Tarea");
	free(longitudNombreTarea);
	return nombreTarea;
}

void* sincronizarBlocks(void* tiempoDeSincronizacion){
	int blockSize  = obtenerBlockSize();
	int blockCount = obtenerCantidadBloquesSuperbloque();
	sem_init(&sincronizacion, 0 ,0);
	while(1){
		sleep(*(int*)tiempoDeSincronizacion);
		if(flagExit){
			//munmap(mapeadoBlocks,blockCount*blockSize);
			sem_post(&sincronizacion);
			break;
		}
		pthread_mutex_lock(&mutexCopiaBloques);
		memcpy(mapeadoBlocks,copiaBlocks,blockCount*blockSize);
		msync(mapeadoBlocks,blockCount*blockSize,MS_SYNC);
		pthread_mutex_unlock(&mutexCopiaBloques);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * SABOTAJE
 */

void llenarPosicionesSabotaje(){
	char* strPosicionesSabotaje = config_get_string_value(config, "POSICIONES_SABOTAJE");
	char* posiciones 			= string_substring(strPosicionesSabotaje, 1, strlen(strPosicionesSabotaje) - 2);
	char** arrPosiciones		= string_split(posiciones,",");
	posicionesSabotaje 			= list_create();
	for(int i = 0; arrPosiciones[i] != NULL; i++){
		list_add(posicionesSabotaje, arrPosiciones[i]);
	}
	free(arrPosiciones);
	free(posiciones);
}

void sabotaje(int n){
	switch (n) {
		case SIGUSR1:
			sem_post(&semaforoSabotaje);
		break;
	}
}

void* enviarAvisosDeSabotaje(void* data){
	int ficheroSocket     		= strtol((char*)data,NULL,10);
	free(data);
	while(1){
		sem_wait(&semaforoSabotaje);
		if(flagExit){
			close(ficheroSocket);
			break;
		} else {
			char* posicionActual 	= list_get(posicionesSabotaje, contadorPosicion);
			contadorPosicion++;
			if(contadorPosicion == posicionesSabotaje->elements_count) contadorPosicion = 0;
			void* magic 			= malloc(sizeof(int) + strlen(posicionActual) + 1);
			int longitudPosicion 	= strlen(posicionActual);
			int desplazamiento 		= 0;
			agregar_a_serializacion(magic, &desplazamiento, &longitudPosicion, sizeof(int));
			agregar_a_serializacion(magic, &desplazamiento, posicionActual, longitudPosicion + 1);
			send(ficheroSocket, magic, sizeof(int) + strlen(posicionActual) + 1, 0);
			free(posicionActual);
		}
	}
	sem_post(&conexionDiscordiadorLiberada);
	return 0;
}

void esperarHilosMongo(){
	sem_post(&semaforoSabotaje);
	sem_wait(&conexionDiscordiadorLiberada);
	sem_wait(&sincronizacion);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * FSCK
 */

int fsckSuperBloque() {
	// SABOTAJE BLOCK_COUNT
    int cantidadDeBloques 	= obtenerCantidadBloquesSuperbloque();
    int blockSize			= obtenerBlockSize();

    char* pathBlocks     	= rutaPuntoMontaje(strlen("/Blocks.ims"));
    strcat(pathBlocks, "/Blocks.ims");
    int fdBlocks = open(pathBlocks, O_RDWR);
    int seek = lseek(fdBlocks,0,SEEK_END);
    close(fdBlocks);

    int cantidadBloquesLeidos = seek / blockSize;
    if(cantidadDeBloques != cantidadBloquesLeidos) {
    	actualizarCantidadBloquesSuperBloque(cantidadBloquesLeidos);
    	return 1;
    }

    // SABOTAJE BITMAP

    char* bitmap 			= obtenerBitmap();
    char* bitmapNuevo 		= string_repeat('0', cantidadBloquesLeidos);

    t_list* bloques 		= leerBloquesArchivos();
    while(bloques->elements_count != 0){
    	int bloque			= obtenerIntLista(bloques,0,1);
    	bitmapNuevo[bloque] = '1';
    }
	liberarListaBloques(bloques);

	free(pathBlocks);
    if(strcmp(bitmap, bitmapNuevo) != 0){
    	reemplazarBitmap(bitmapNuevo);
    	free(bitmap);
    	free(bitmapNuevo);
    	return 1;
    }
    free(bitmap);
	free(bitmapNuevo);

    return 0;
}

int fsckFiles(){

	char* pathFiles 		= ruteFiles(0);
	t_list* listaArchivos 	= obtenerNombresArchivos(pathFiles);

	int seSaboteo 	= 0;
	for(int i = 0; i < listaArchivos->elements_count; i++){
		if(!seSaboteo){
			char* pathCompleto 	= string_duplicate(pathFiles);
			string_append(&pathCompleto,list_get(listaArchivos,i));
			seSaboteo 			= verificarSabotajeFile(pathCompleto);
			free(pathCompleto);
		}
	}
	while(listaArchivos->elements_count != 0){
		char* archivo = list_remove(listaArchivos, 0);
		free(archivo);
	}
	list_destroy(listaArchivos);
	free(pathFiles);
	return seSaboteo;
}

int obtenerIntLista(t_list* lista, int index, int quitarDeLista){
	char* strBloque = NULL;
	int bloque 		= -1;
	if(quitarDeLista){
		strBloque 	= list_remove(lista,index);
		bloque 		= strtol(strBloque,NULL,10);
		free(strBloque);
	} else {
		strBloque = list_get(lista,index);
		bloque 		= strtol(strBloque,NULL,10);
	}
	return bloque;
}

int verificarSabotajeFile(char* pathArchivo){
	FILE* archivo 	= fopen(pathArchivo,"r+");
	char* md5Nuevo 	= generarMD5(archivo);
	char* md5Viejo 	= obtenerMD5(archivo);
	if(strcmp(md5Nuevo,md5Viejo) == 0){
		free(md5Viejo);
		free(md5Nuevo);
		return 0;
	}
	free(md5Viejo);
	free(md5Nuevo);

	t_list* bloques 				= obtenerBloquesFile(archivo);

	// SABOTAJE BLOCKS

	for(int i = 0; i < bloques->elements_count; i++){
		int bloque				= obtenerIntLista(bloques,i,0);
		printf("Bloque: %d\n",bloque);
		if(sePasaDeTamanio(bloque) || estaVacio(bloque) || ultimoBloqueMovido(i,bloques->elements_count -1, bloque)){
			printf("VAMO A RESTAURAR\n");
			vaciarBloques(bloques,i);
			restaurarArchivo(archivo, i, pathArchivo, bloques);
			return 1;
		}
	}

	// SABOTAJE SIZE
	int sizeArchivo					= obtenerSizeFile(archivo);
	int tamanioBloques 				= obtenerBlockSize();
	int tamanioRestante 			= 0;
	for(int i = 0; i < bloques->elements_count ; i++){
		int bloque					= obtenerIntLista(bloques,i,0);
		tamanioRestante 	       += tamanioDisponibleBlock(bloque);
	}
	int sizeLeido 					= (bloques->elements_count) * tamanioBloques - tamanioRestante;
	int diferenciaSize		 		= sizeLeido - sizeArchivo;
	if(diferenciaSize) {
		actualizarFileSize(archivo,pathArchivo,sizeLeido,0);
		truncarArchivo(archivo, pathArchivo);
		fclose(archivo);
		return 1;
	}

	// SABOTAJE BLOCK_COUNT

	int blockCountLeido 	= bloques->elements_count;
	int blockCountArchivo 	= obtenerCantidadDeBloquesFile(archivo);
	if(blockCountLeido     != blockCountArchivo){
		actualizarBlockCount(archivo,blockCountLeido);
		truncarArchivo(archivo, pathArchivo);
		fclose(archivo);
		return 1;
	}

	return 0;
}

void truncarArchivo(FILE* archivo, char* pathArchivo){
	int seek = desplazamientoLinea(archivo, 5);
	truncate(pathArchivo, seek);

}

void vaciarBloques(t_list* bloques, int index){
	while(bloques->elements_count > index){
		int bloque = obtenerIntLista(bloques,index,1);
		if(!sePasaDeTamanio(bloque)){
			vaciarBloque(bloque);
		}
	}
}

void vaciarBloque(int bloque){
	int tamanioBloque = obtenerBlockSize();
	char* bloqueVacio = string_repeat('\0', tamanioBloque);
	pthread_mutex_lock(&mutexCopiaBloques);
	memcpy(copiaBlocks + bloque*tamanioBloque, bloqueVacio, tamanioBloque);
	pthread_mutex_unlock(&mutexCopiaBloques);
}

int sePasaDeTamanio(int bloque){
	int cantidadBloques = obtenerCantidadBloquesSuperbloque();
	return bloque > cantidadBloques;
}

int estaVacio(int bloque){
	pthread_mutex_lock(&mutexBitmap);
	char* bitmap = obtenerBitmap();
	pthread_mutex_unlock(&mutexBitmap);
	printf("%c\n", bitmap[bloque]);
	int vacio = (bitmap[bloque] == '0');
	free(bitmap);
	return vacio;
}

int ultimoBloqueMovido(int indexBloque, int cantBloques, int bloque){
	int noEsUltimoBloque 		= (indexBloque != cantBloques);
	int tieneTamanioDisponible 	= tamanioDisponibleBlock(bloque) != 0;
	return noEsUltimoBloque && tieneTamanioDisponible;
}

void restaurarArchivo(FILE* archivo, int index, char* pathArchivo, t_list* bloques){
    int sizeBloques = bloques->elements_count;

    t_list* bloquesAux = list_create();
    for(int i = 0; i < bloques->elements_count; i++){
    	char* bloque = malloc(strlen(list_get(bloques, i)) + 1);
    	strcpy(bloque, list_get(bloques, i));
    	list_add(bloquesAux, bloque);
    }

    char* pathArchivoAux = string_duplicate(pathArchivo);

    actualizarBloquesFile(archivo,bloques,pathArchivo);
    int tamanioBloque = obtenerBlockSize();
    int sizePorAgregar = obtenerSizeFile(archivo) - sizeBloques * tamanioBloque;
    actualizarFileSize(archivo,pathArchivo, sizeBloques * tamanioBloque,0);
    char caracterLlenado = obtenerCaracterLlenadoFile(archivo);
    fclose(archivo);
    vaciarBloqueCambiado(bloquesAux, caracterLlenado);
    agregarCaracteresA(pathArchivoAux,sizePorAgregar,1);

    while(bloquesAux->elements_count != 0) {
        char* bloque = list_remove(bloquesAux, 0);
        free(bloque);
    }
    list_destroy(bloquesAux);
}

void vaciarBloqueCambiado(t_list* bloques, char caracterLlenado){
	pthread_mutex_lock(&mutexBitmap);
	char* bitmap = obtenerBitmap();
	for(int i = 0; i < strlen(bitmap); i++){
		if(bitmap[i] == '1' && tieneCaracterLlenado(i,caracterLlenado) && !lista_find(bloques,i)){
			vaciarBloque(i);
			actualizarBitmap(i,'0');
		}
	}
	free(bitmap);
	pthread_mutex_unlock(&mutexBitmap);
}

int lista_find(t_list* bloques, int bloque){
	for(int i = 0; i < bloques->elements_count; i++){
		if(obtenerIntLista(bloques,i,0) == bloque) return 1;
	}
	return 0;
}


int tieneCaracterLlenado(int bloque , char caracterLlenado){
	int tamanioBloque 	= obtenerBlockSize();
	char* strBloque 	= malloc(tamanioBloque);

	pthread_mutex_lock(&mutexCopiaBloques);
	memcpy(strBloque, copiaBlocks + bloque*tamanioBloque, tamanioBloque);
	pthread_mutex_unlock(&mutexCopiaBloques);
	for(int i = 0; i < tamanioBloque; i++){
		if(strBloque[i] != '\0' && strBloque[i] != caracterLlenado){
			return 0;
			break;
		}
	}
	free(strBloque);
	return 1;
}




void cambiarBloqueNoCompleto(FILE* archivo, char* pathArchivo, t_list* bloques){
	int index = -1;
	for(int i = 0; i < bloques->elements_count; i++){
		int bloque = obtenerIntLista(bloques,i,0);
		if(tamanioDisponibleBlock(bloque) != 0) {
			index = i;
			break;
		}
	}
	char* bloque = list_remove(bloques,index);
	list_add(bloques,bloque);
}


t_list* leerBloquesArchivos() {
	t_list* bloques 	= list_create();
	char* pathArchivos  = ruteFiles(0);
	char* pathBitacoras = rutaBitacoras(0);

	obtenerBloquesDeArchivos(bloques,pathArchivos, 1);
	obtenerBloquesDeArchivos(bloques,pathBitacoras, 0);

	free(pathArchivos);
	free(pathBitacoras);

	return bloques;

}

void obtenerBloquesDeArchivos(t_list* bloques, char* pathArchivos, int esFile){
	t_list* listaArchivos 	= obtenerNombresArchivos(pathArchivos);
	for(int i = 0; i < listaArchivos->elements_count; i++){
		char* pathCompleto = string_duplicate(pathArchivos);
		string_append(&pathCompleto,list_get(listaArchivos,i));
		FILE* archivo = fopen(pathCompleto,"r");
		free(pathCompleto);
		t_list* bloquesArchivo = obtenerBloquesFB(archivo,esFile);
		while(bloquesArchivo->elements_count != 0){
			char* strBloque = list_remove(bloquesArchivo, 0);
			list_add(bloques, strBloque);
		}
		list_destroy(bloquesArchivo);
		fclose(archivo);
	}
	while(listaArchivos->elements_count != 0){
		char* archivo = list_remove(listaArchivos, 0);
		free(archivo);
	}
	list_destroy(listaArchivos);
}

t_list* obtenerBloquesFB(FILE* archivo, int esFile){
	if(esFile) return obtenerBloquesFile(archivo);
	else return obtenerBloquesBitacora(archivo);
}

t_list* obtenerNombresArchivos(char* directorio){
	t_list* listaArchivos = list_create();
	DIR *direccion;
	struct dirent *dir;

	direccion = opendir(directorio);
	if (direccion) {
		while ((dir = readdir(direccion)) != NULL) {
			if( strcmp( dir->d_name, "." ) != 0 && strcmp( dir->d_name, ".." ) != 0 ){
				char* cadena = string_duplicate(dir->d_name);
				if(strcmp(cadena,"Bitacoras") != 0) list_add(listaArchivos,cadena);
			}
		}
		closedir(direccion);
	}
	return listaArchivos;
}


void actualizarCantidadBloquesSuperBloque(int cantidadBloques){
	FILE* superBloque = abrirSuperBloque();

	char* strCantidadBloques = string_new();
	string_append_with_format(&strCantidadBloques,"BLOCK_COUNT=%d\n",cantidadBloques);
	actualizarLineaArchivo(superBloque,2,strCantidadBloques,3);

	fclose(superBloque);
}

void reemplazarBitmap(char* bitmap){
	FILE* superBloque = abrirSuperBloque();
	char* bitmapCompleto = string_new();
	string_append_with_format(&bitmapCompleto,"BITMAP=%s\n",bitmap);
	actualizarLineaArchivo(superBloque,3,bitmapCompleto,3);
	fclose(superBloque);
}

void fsck() {
	int seSaboteo = 0;
    seSaboteo = fsckFiles();
    if(!(seSaboteo)) seSaboteo = fsckSuperBloque();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * OBTENER BITACORA
 */

void generarLogBitacora(int tid, int ficheroSocketNuevo){
	char* pathBitacora 		= rutaBitacora(tid);
	char* logBitacora		= string_new();
	char* strBloques		= malloc(0);
	int contadorTamanio		= 0;
	int desplazamiento		= 0;
	FILE* bitacora			= fopen(pathBitacora, "r");
	free(pathBitacora);
	if(bitacora != NULL){
		int blockSize = obtenerBlockSize();
		if(existeTripulante(tid)) waitSemaforoBitacora(tid);
		t_list* bloques 	= obtenerBloquesBitacora(bitacora);
		fclose(bitacora);
		if(existeTripulante(tid)) postSemaforoBitacora(tid);
		if(bloques->elements_count != 0){
			for(int i = 0; i < bloques->elements_count; i++){
				char* strBloqueActual 	= list_get(bloques, i);
				int bloqueActual		= strtol(strBloqueActual, NULL, 10);
				int tamanio 			= -1;
				if(i + i != bloques->elements_count){
					tamanio = blockSize;
				} else {
					tamanio = blockSize - tamanioDisponibleBlock(bloqueActual);
				}
				char* strBloque 		= malloc(tamanio);
				memcpy(strBloque, copiaBlocks + (bloqueActual * blockSize), tamanio);
				contadorTamanio 	+= tamanio;
				strBloques 			= realloc(strBloques, contadorTamanio + 1);
				memcpy(strBloques + desplazamiento, strBloque, tamanio);
				desplazamiento 	+= tamanio;
				free(strBloque);
			}
			liberarListaBloques(bloques);
			char** arrLogs = string_split(strBloques, ".");
			for(int i = 0; arrLogs[i] != NULL; i++){
				if(arrLogs[i+1] != NULL){
					string_append_with_format(&logBitacora, "-%s\n", arrLogs[i]);
				} else {
					string_append(&logBitacora, arrLogs[i]);
				}
			}
			free(strBloques);
			for(int i = 0; arrLogs[i] != NULL; i++){
				free(arrLogs[i]);
				arrLogs[i] = NULL;
			}
			free(arrLogs);
			int longitudLogBitacora = strlen(logBitacora);
			void* magic 			= malloc(sizeof(int) + longitudLogBitacora + 1);
			int desplazamiento2 	= 0;
			agregar_a_serializacion(magic, &desplazamiento2, &longitudLogBitacora, sizeof(int));
			agregar_a_serializacion(magic, &desplazamiento2, logBitacora, longitudLogBitacora + 1);
			send(ficheroSocketNuevo, magic, sizeof(int) + longitudLogBitacora + 1, 0);
			free(magic);
		} else {
			int resultado 		= 0;
			void* magic 		= malloc(sizeof(int));
			int desplazamiento2 = 0;
			agregar_a_serializacion(magic, &desplazamiento2, &resultado, sizeof(int));
			send(ficheroSocketNuevo, magic, sizeof(int), 0);
			free(magic);
		}
	} else {
		int resultado 		= -1;
		void* magic 		= malloc(sizeof(int));
		int desplazamiento2 = 0;
		agregar_a_serializacion(magic, &desplazamiento2, &resultado, sizeof(int));
		send(ficheroSocketNuevo, magic, sizeof(int), 0);
		free(magic);
	}
	free(logBitacora);
	close(ficheroSocketNuevo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * FUNCIONES HILOS
 */

void* generarRecursos(void* data){
	int ficheroSocketNuevo  = strtol((char*)data,NULL,10);
	int* cantidad		 	= (int*)deserializar(ficheroSocketNuevo, sizeof(int), "Cantidad");
	int* longitudRecurso 	= (int*)deserializar(ficheroSocketNuevo, sizeof(int), "Longitud Recurso");
	char* recurso			= (char*)deserializar(ficheroSocketNuevo, *longitudRecurso + 1, "Nombre Recurso");
	int resultado 			= agregarCaracteresA(recurso, *cantidad,0);
	send(ficheroSocketNuevo,&resultado,sizeof(int),0);
	close(ficheroSocketNuevo);
	if(resultado == -1){
		char* log = string_new();
		string_append(&log, "No se pudo agregar el/los recurso/s, disco lleno");
		logearArchivoIMongo(log);
	}
	free(longitudRecurso);
	free(cantidad);
	free(recurso);
	free(data);
	return 0;
}
void* descartarRecursos(void* data){
	int ficheroSocketNuevo	= strtol((char*)data,NULL,10);
	int* cantidad 			= (int*)deserializar(ficheroSocketNuevo, sizeof(int), "Cantidad");
	int* longitudRecurso 	= (int*)deserializar(ficheroSocketNuevo, sizeof(int), "Longitud Recurso");
	char* recurso			= (char*)deserializar(ficheroSocketNuevo, *longitudRecurso + 1, "Nombre Recurso");
	send(ficheroSocketNuevo,cantidad,sizeof(int),0);
	close(ficheroSocketNuevo);
	int resul 				= borrarCaracteresA(recurso, *cantidad);
	free(longitudRecurso);
	free(cantidad);
	free(data);
	if(resul == -1){
		char* log = string_new();
		string_append_with_format(&log, "El archivo %s.ims no existe", recurso);
		logearArchivoIMongo(log);
	}
	if(resul == 1){
		char* log = string_new();
		string_append(&log, "Se quisieron borrar mas caracteres de los existentes");
		logearArchivoIMongo(log);
	}
	free(recurso);
	return 0;
}
void* iniciarBitacora(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	int tid 				=	recibirTID(ficheroSocketNuevo);
	send(ficheroSocketNuevo,&tid,sizeof(int),0);
	close(ficheroSocketNuevo);
	crearArchivoBitacora(tid);
	free(data);
	return 0;
}
void* logearMovimientoHilo(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	int tid 				= recibirTID(ficheroSocketNuevo);
	recibirMovimientoYLoguear(ficheroSocketNuevo, tid);
	send(ficheroSocketNuevo,&tid,sizeof(int),0);
	close(ficheroSocketNuevo);
	free(data);
	return 0;
}
void* logearComienzoDeTareaHilo(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	int tid 		= recibirTID(ficheroSocketNuevo);
	char* nombreTarea = deserializarTarea(ficheroSocketNuevo);
	logearComienzoDeTarea(tid,nombreTarea);
	send(ficheroSocketNuevo,&tid,sizeof(int),0);
	close(ficheroSocketNuevo);
	free(data);
	return 0;
}
void* logearFinalizacionDeTareaHilo(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	int tid 		= recibirTID(ficheroSocketNuevo);
	char* nombreTarea = deserializarTarea(ficheroSocketNuevo);
	send(ficheroSocketNuevo,&tid,sizeof(int),0);
	logearFinalizacionDeTarea(tid,nombreTarea);
	free(data);
	return 0;
}
void* logearCorridaEnPanicoHilo(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	int tid = recibirTID(ficheroSocketNuevo);
	send(ficheroSocketNuevo,&tid,sizeof(int),0);
	logearCorridaEnPanico(tid);
	free(data);
	return 0;
}
void* logearSabotajeResueltoHilo(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	int tid = recibirTID(ficheroSocketNuevo);
	send(ficheroSocketNuevo,&tid,sizeof(int),0);
	logearSabotajeResuelto(tid);
	free(data);
	return 0;
}
void* enviarBitacora(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	int tid 				= recibirTID(ficheroSocketNuevo);
	generarLogBitacora(tid, ficheroSocketNuevo);
	free(data);
	return 0;
}
void* invocarFsck(void* data){
	int ficheroSocketNuevo     = strtol((char*)data,NULL,10);
	int tid 				= 0;
	send(ficheroSocketNuevo,&tid,sizeof(int),0);
	fsck();
	free(data);
	return 0;
}

void logearArchivoIMongo(char* log){
	char* horaActual 	= temporal_get_string_time("%d/%m/%y %H:%M:%S");
	pthread_mutex_lock(&mutexArchivoLogIMongo);
	FILE* archivoLog = fopen("../logIMongoStore.txt", "a+");
	fprintf(archivoLog, "%s:\t%s\n", horaActual, log);
	fclose(archivoLog);
	pthread_mutex_unlock(&mutexArchivoLogIMongo);
	free(horaActual);
	free(log);
}
