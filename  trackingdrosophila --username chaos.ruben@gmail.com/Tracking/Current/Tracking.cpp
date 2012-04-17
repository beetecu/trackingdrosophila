/*
 * Tracking.cpp
 *
 * 		ALGORITMO DE RASTREO

 *  Created on: 20/10/2011
 *      Authors: Rubén Chao Chao.
 *      		 German Macía Vázquez.
 */

#include "Tracking.hpp"

tlcde* Identities = NULL;
tlcde* lsTracks = NULL;

STFrame* Tracking( tlcde** framesBuff,STFrame* frameDataIn, int MaxTracks ){

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
	// Asignar identidades.
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

//	validarTracks( lsTracks, MAX_TRACKS );

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

	// SI BUFFER LLENO
	if( framesBuf->numeroDeElementos == IMAGE_BUFFER_LENGTH ){

		irAlPrincipio( framesBuf );
		// Establecer en base a diversos parámetros la validez del track.
		//
		// Comprobar si se reinicia un track  o se elimina
		// si existen tracks durmiendo durante un tiempo x ( mirar contador)
		// y durante el tiempo del buffer no ha aparecido ninguna asignación con una probabilidad aceptable
		// pero se ha creado un nuevo track en algún punto, y este es válido ( OJO con esto. Si no verificamos
		// que es válido se puede asignar un track durmiendo a un track iniciado por un espurio)
		// eliminar el nuevo ( trackdead) y asignar la fly que rastreaba
		// el track eliminado al track durmiendo con prioridad al más cercano.
		// Reasignar etiquetas.

		// Si hay 2 tracks siguiendo a una fly en movimiento durante x frames.
		//	1) Comprobar si se ha creado recientemente un nuevo track.
		// 	   si es así, de los dos tracks, asignamos el más reciente a la fly del  nuevo track y lo eliminamos y reasignamos id
		// 	2) si el nuevo track es más antiguo que cualquiera de los dos o bien no se ha creado recientemente uno nuevo, lo eliminamos
		//  y reasignamos ids.

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
// 1)si se ha creado un track en t y en t+1:
//		a) no tiene asignación, es decir, flySig == NULL ( porque no tiene nueva medida )
//			o porque sus posibles asignaciones están más alejadas de la maxima distancia permitida) o bien
//		b) la fly que se ha asignado al nuevo track está a su vez asignada a otro u otros tracks, es decir,
//		 	se ha creado un nuevo track en t y en t+1 un blob que ya estaba siendo rastreado por un
//			Track es asignado al nuevo track.
//   - se elimina el track ( Se considera un espurio o de un reflejo momentáneo en el borde del plato ).

//	Aclaraciones:
//	Supocicion:se habria creado un track para un supuesto espurio ( que sobrevivió a la validación ) en t.
//	En t+1, dicho espurio desaparece.
//	Suponemos que en t+1 el track no es eliminado.
//  caso 1) No hay ningún blob lo suficientemente cerca como para ser asignado a dicho track.
//		=> dicho espurio generaría un track en estado SLEEPING que estaría esperando a una asignación válida.
//		Si no se encuentra dicha asignación en un intervalo de tiempo determinado, el track acabaría
//		siendo eliminado en la fase de corrección. En caso contrario, nos encontraríamos con el  caso b)
//	caso 2) un blob que ya estaba siendo rastreado por un track con asignación única se encuentra lo
//		suficientemente cerca del falso track como para que el blob también sea asignado a éste nuevo track.
//		=> en asignarIdentidades se interpretará que el blob oculta dos flies, y se intentará
//		validar para dividir el blob en  2, lo cual sería erróneo. De no ser eliminado,	habría dos tracks
//		siguiendo al mismo blob.
//  Consecuencia: Hay que eliminar el nuevo Track. ( o bien antes de validar o bien usar la validación para detectar si se trata
//	de un único blob con una probabilidad suficientemente alta ). Así establecemos flySig = NULL en el track del blob recien creado,
//  de modo que sería automáticamente eliminado en validarTracks.
//	El error en caso de eliminar un track correcto quedaría subsanado posteriormente ya que si en la siguiente iteración se vuelve
//	a detectar otro blob/espurio en el mismo punto, kalman crearía un nuevo track repitiendose el proceso. Únicamente si durante 2
//	iteraciones consecutivas el track tiene una nueva asignación y ésta es única se considerará que es un "posible" track válido.

//  "Posible" ya que no se establecerá dicho track como válido hasta que se verifique que no se trata de un
//  espurio que permanece varios frames o bien de un reflejo (movil o inmovil). Este caso se trata en la parte de Heurísticas usando
// la información temporal del buffer.

void validarTracks( tlcde* lsTracks, int MaxTracks ){

	STTrack* Track = NULL;
	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
			// obtener Track
			Track = (STTrack*)obtener(i, lsTracks);
			if( falsoTrack( Track ) ) deadTrack( lsTracks, i );
	}
	return;
}

int falsoTrack( STTrack* Track ){


	if( Track->Estado == CAM_CONTROL && Track->EstadoCount == 1 ){
		if(!Track->Flysig || Track->Flysig->Tracks->numeroDeElementos > 1 ){
			return 1;
		}
		return 0;
	}
	else return 0;

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


