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

STFrame* Tracking( tlcde** framesBuff,STFrame* frameDataIn ){

	tlcde* framesBuf;
	STFrame* frameDataOut = NULL; // frame de salida

#ifdef MEDIR_TIEMPOS
	struct timeval ti,tif; // iniciamos la estructura
	float tiempoParcial;
	gettimeofday(&tif, NULL);
	printf("\n2)Tracking:\n");
	gettimeofday(&ti, NULL);
	printf("\t1)Asignación de identidades\n");
#endif


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

	////////////// AÑADIR AL BUFFER /////////////
	anyadirAlFinal( frameDataIn, framesBuf);
//	MotionTemplate( framesBuf,Identities );

	////////////// ASIGNACIÓN DE IDENTIDADES ///////////

	//APLICAR EL METODO DE OPTIMIZACION HUNGARO A LA MATRIZ DE PESOS
	// Asignar identidades y orientación.
	// resolver las asociaciones usando las predicciones de kalman mediante el algoritmo Hungaro
	// Si varias dan a la misma etiquetarla como 0. Enlazar flies.
	// Se trabaja en las posiciones frame MAX_BUFFER - 1 y MAX_BUFFER -2.

	asignarIdentidades( lsTracks,frameDataIn->Flies);

#ifdef MEDIR_TIEMPOS
	tiempoParcial= obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo: %5.4g ms\n", tiempoParcial);
	gettimeofday(&ti, NULL);
	printf("\t2)Filtro de Kalman\n");
#endif

	/// ELIMIRAR FALSOS TRACKS ///
	// únicamente serán validos aquellos tracks que los primeros instantes tengan asignaciones válidas
	// y unicas. Si no es así se considera un trackDead:

	// 1)si se ha creado un track en t y en t+1 :
	//		a) no tiene asignación, es decir, flySig == NULL ( porque no tiene nueva medida
	//			o porque sus posibles asignaciones están más alejadas de la maxima distancia permitida) o bien
	//		b) la fly que se ha asignado al nuevo track está a su vez asignada a otro u otros tracks.
	//   - se elimina el track ( Se considera un espurio o de un reflejo momentáneo en el borde del plato ).
	//   El error en caso de eliminar un track correcto quedaría subsanado posteriormente ya que si en la siguiente iteración se vuelve
	//	 a detectar, kalman crearía un nuevo track. Únicamente si durante 2  iteraciones consecutivas el track
	//	 tiene una nueva asignación y única se considera un posible track válido, asi si:

	// 2)En caso de tener asignaciones válidas y únicas durante los primeros instantes:
	//		a) Se puede tratar de un reflejo en el borde del plato de una mosca ( móvil ó inmovil )
	//
	//		b) Puede ser una mosca que no se ha movido durante la detección del plato y se mueve mínimamente
	//			en un instante dado. En este caso el track sería válido.
	//		¿como distinguir el caso a) del b) ?
	//
	/////////////// FILTRO DE KALMAN //////////////
	// El filtro de kalman trabaja en la posicion MAX_BUFFER -1. Ultimo elemento anyadido.

	Kalman( frameDataIn , Identities, lsTracks);

#ifdef MEDIR_TIEMPOS
	tiempoParcial= obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo: %5.4g ms\n", tiempoParcial);
#endif

	///////   FASE DE CORRECCIÓN. APLICACION DE HEURISTICAS  /////////
	// esta etapa se encarga de hacer la asignación de identidad definitiva
	// Se mantienen en memoria los últimos MAX_BUFFER_LENGHT/fps segundos de video
	// recibe información temporal de los últimos MAX_BUFFER_LENGHT frames ( Longitud_buffer)
	// y corrige y elimina tracks
	// La posición de trabajo será la 0. Este frame será el de salida.
	// una vez procesado.
	// Reasignamos idendidades.
	// enlazamos objetos etiquetados como nuevos con los que estaban parados

	// SI BUFFER LLENO
	if( framesBuf->numeroDeElementos == IMAGE_BUFFER_LENGTH ){

		irAlPrincipio( framesBuf );
		// Comprobar si se reinicia un track  o se elimina
		// si existen tracks durmiendo durante un tiempo x ( mirar contador)
		// y durante el tiempo del buffer no ha aparecido ninguna asignación con una probabilidad aceptable
		// pero se ha creado un nuevo track en algún punto,
		// eliminar el nuevo ( trackdead) y asignar la fly que rastreaba
		// el track eliminado al track durmiendo con prioridad al al más cercano.
		// Reasignar etiquetas.

		// Para los tracks que están durmiendo, si llevan durmiendo durante x frames eliminarlos
		// y reasignar las etiquetas de los que quedan durmiendo
//		for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
//						if ( Track->FlyActual == NULL )
//							ReInitTrack( Track, Fly, 1);
//		}
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
		printf("Tracking correcto.Tiempo total %5.4g ms\n", tiempoParcial);
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
	DeallocateKalman( lsTracks );
	free(lsTracks);

}

