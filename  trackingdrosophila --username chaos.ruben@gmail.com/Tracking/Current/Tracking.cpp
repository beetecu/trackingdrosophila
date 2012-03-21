/*
 * Tracking.cpp
 *
 *	Hasta aquí el algoritmo trabaja solo con información espacial
 *	En este punto añadimos la información temporal
 *  Se mantienen en memoria las estructuras correspondientes a MAX_BUFFER frames
 *	( buffer de datos  ) e IMAGE_BUFFER_LENGHT ( buffer de imagenes ) .
 *	Los buffers son colas FIFO
 *
 *  Created on: 20/10/2011
 *      Author: chao
 */

#include "Tracking.hpp"

tlcde* Identities = NULL;
tlcde* lsTracks = NULL;
CvMat* Matrix_Hungarian = NULL;

STFrame* Tracking( tlcde** framesBuff,STFrame* frameDataIn ){

	tlcde* framesBuf;
	STFrame* frameDataOut = NULL; // frame de salida
	STFrame* frameData = NULL; // frame t. penultimo frame del buffer.
	STFrame* frameDataSig = NULL; // frame t + 1 (ultimo frame del buffer.

#ifdef MEDIR_TIEMPOS
	struct timeval ti,tif; // iniciamos la estructura
	float tiempoParcial;
	gettimeofday(&tif, NULL);
#endif
	printf("\n2)Tracking:\n");

	/////////////// Inicializar /////////////////
	framesBuf = *framesBuff;

	// cola Lifo de identidades
	if(!Identities) {
		Identities = ( tlcde * )malloc( sizeof(tlcde ));
		if( !Identities ) {error(4);exit(1);}
		iniciarLcde( Identities );
		CrearIdentidades(Identities);
		//mostrarIds( Identities );
	}

	//buffer de Imagenes y datos.
	if(!*framesBuff){
		framesBuf = ( tlcde * )malloc( sizeof(tlcde));
		if( !framesBuf ) {error(4);exit(1);}
		iniciarLcde( framesBuf );
		*framesBuff = framesBuf;
	}

	//Tracks
	if(!lsTracks){
		lsTracks = ( tlcde * )malloc( sizeof(tlcde ));
		if( !lsTracks ) {error(4);exit(1);}
		iniciarLcde( lsTracks );
	}

	//Imagenes
//	AllocateTrackImages( frameDataIn->FG );

	////////////// AÑADIR AL BUFFER /////////////
	anyadirAlFinal( frameDataIn, framesBuf);
	MotionTemplate( framesBuf,Identities );
	if( framesBuf->numeroDeElementos < 2  )	return 0;

#ifdef MEDIR_TIEMPOS
	gettimeofday(&ti, NULL);
#endif

	////////////// ASIGNACIÓN DE IDENTIDADES ///////////

	//APLICAR EL METODO DE OPTIMIZACION HUNGARO A LA MATRIZ DE PESOS
	// Asignar identidades y orientación.
	// resolver las asociaciones usando las predicciones de kalman mediante el algoritmo Hungaro
	// Si varias dan a la misma etiquetarla como 0. Enlazar flies.
	// Se trabaja en las posiciones frame MAX_BUFFER - 1 y MAX_BUFFER -2.
	printf("\t1)Asignación de identidades\n");
	if( Matrix_Hungarian ){
//		Hungaro(Matrix_Hungarian);
//		asignarIdentidades( lsTracks, frameData->Fliesanterior,frameDataSig->Flies , Identities);

		cvReleaseMat(&Matrix_Hungarian);
	}
	cvZero(frameDataIn->ImMotion);


#ifdef MEDIR_TIEMPOS
	tiempoParcial= obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo total: %5.4g ms\n", tiempoParcial);
#endif



#ifdef MEDIR_TIEMPOS
	gettimeofday(&ti, NULL);
#endif

	/////////////// FILTRO DE KALMAN //////////////
	// El filtro de kalman trabaja en la posicion MAX_BUFFER -1. Ultimo elemento anyadido.

	// cargar datos del frame
	frameData = ( STFrame* ) obtener(framesBuf->numeroDeElementos-2, framesBuf);
	frameDataSig = ( STFrame* ) obtener(framesBuf->numeroDeElementos-1, framesBuf);
	// Aplicar kalman
//	Kalman2( frameDataIn, Identities, lsTracks);
	Matrix_Hungarian = Kalman(frameData,frameDataSig,Identities, lsTracks ); // Nos devuelve la matriz de pesos


#ifdef MEDIR_TIEMPOS
	tiempoParcial= obtenerTiempo( ti, 0);
	printf("\t\t- Filtrado correcto.Tiempo total: %5.4g ms\n", tiempoParcial);
#endif

	///////   FASE DE CORRECCIÓN. HEURISTICAS  /////////
	// esta etapa se encarga de hacer la asignación de identidad definitiva
	// recibe información temporal de los últimos 51 frames ( Longitud_buffer)
	// La posición de trabajo será la 0. Este frame será el de salida.
	//
	//Reasignamos idendidades.
	// enlazamos objetos etiquetados como nuevos con los que estaban parados

	// SI BUFFER LLENO
	if( framesBuf->numeroDeElementos == IMAGE_BUFFER_LENGTH ){

		irAlPrincipio( framesBuf );

		////////// LIBERAR MEMORIA  ////////////
		frameDataOut = (STFrame*)liberarPrimero( framesBuf ) ;
		if(!frameDataOut){error(7); exit(1);}

#ifdef MEDIR_TIEMPOS
		tiempoParcial = obtenerTiempo( tif , NULL);
		printf("Tracking correcto.Tiempo total %5.4g ms\n", tiempoParcial);
#endif

		return frameDataOut;
	}
	else {
#ifdef MEDIR_TIEMPOS
		tiempoParcial = obtenerTiempo( tif , NULL);
		printf("Llenando buffer.Tiempo total %5.4g ms\n", tiempoParcial);
#endif
		return 0;
	}
}

void ReleaseDataTrack( tlcde* FramesBuf ){
	if(Identities){
		liberarIdentidades( Identities );
		free( Identities) ;
	}
	if(FramesBuf) {
		liberarBuffer( FramesBuf );
		free( FramesBuf);
	}
//	cvReleaseImage( &ImOpFlowX);
//	cvReleaseImage( &ImOpFlowY);
//	cvReleaseImage( &ImagenA);
//	cvReleaseImage( &ImagenB);
//	releaseMotionTemplate();
	DeallocateKalman( lsTracks );
	free(lsTracks);
	if(Matrix_Hungarian) cvReleaseMat(&Matrix_Hungarian);

}

