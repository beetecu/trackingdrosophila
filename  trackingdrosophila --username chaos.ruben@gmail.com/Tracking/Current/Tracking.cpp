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
tlcde* framesBuf = NULL;

STFrame* Tracking( STFrame* frameDataIn, int MaxTracks,StaticBGModel* BGModel, CvVideoWriter* Writer ){


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

	// cola Lifo de identidades
	if(!Identities) {
		Identities = ( tlcde * )malloc( sizeof(tlcde ));
		if( !Identities ) {error(4);exit(1);}
		iniciarLcde( Identities );
		CrearIdentidades(Identities);
		//mostrarIds( Identities );
	}
	//buffer de Imagenes y datos.
	if(!framesBuf){
		framesBuf = ( tlcde * )malloc( sizeof(tlcde));
		if( !framesBuf ) {error(4);exit(1);}
		iniciarLcde( framesBuf );
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
	if( frameDataIn->num_frame == 185 ){
						printf("hola");
					}
	if( frameDataIn->num_frame == 210 ){
						printf("hola");
					}
	if( frameDataIn->num_frame == 219 ){
							printf("hola");
						}
	asignarIdentidades( lsTracks,frameDataIn->Flies);

#ifdef MEDIR_TIEMPOS
	tiempoParcial= obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo: %5.4g ms\n", tiempoParcial);
	gettimeofday(&ti, NULL);
	printf("\t2)Filtro de Kalman\n");
#endif

	/////////////// ELIMIRAR FALSOS TRACKS ///
	// únicamente serán validos aquellos tracks que los primeros instantes tengan asignaciones válidas
	// y unicas. Si no es así se considera un trackDead:
	frameDataIn->numTracks = validarTracks( framesBuf, lsTracks,Identities, MAX_TRACKS );

	/////////////// FILTRO DE KALMAN //////////////
	// El filtro de kalman trabaja en la posicion MAX_BUFFER -1. Ultimo elemento anyadido.

	Kalman( frameDataIn , Identities, lsTracks);

	frameDataIn->Tracks = lsTracks;

#ifdef MEDIR_TIEMPOS
	tiempoParcial= obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo: %5.4g ms\n", tiempoParcial);
#endif

	///////   FASE DE CORRECCIÓN. APLICACION DE HEURISTICAS  /////////

	despertarTrack(framesBuf, lsTracks, Identities );

	// SI BUFFER LLENO
	if( framesBuf->numeroDeElementos == IMAGE_BUFFER_LENGTH ){

		corregirTracks(framesBuf,  lsTracks, Identities );

	////////// LIBERAR MEMORIA  ////////////
		frameDataOut = (STFrame*)liberarPrimero( framesBuf ) ;
		if(!frameDataOut){error(7); exit(1);}

		if(!SHOW_WINDOW) VisualizarEl( framesBuf, PENULTIMO, BGModel, Writer );

#ifdef MEDIR_TIEMPOS
		tiempoParcial = obtenerTiempo( tif , NULL);
		printf("Tracking correcto.Tiempo total %5.4g ms\n", tiempoParcial);
#endif
		return frameDataOut;
	}
	else {
		VerEstadoBuffer( frameDataIn->Frame, framesBuf->numeroDeElementos, IMAGE_BUFFER_LENGTH);
		if(!SHOW_WINDOW) VisualizarEl( framesBuf, PENULTIMO, BGModel, Writer );
#ifdef MEDIR_TIEMPOS
		tiempoParcial = obtenerTiempo( tif , NULL);
		printf("Tracking correcto.Tiempo total %5.4g ms\n", tiempoParcial);
#endif
		return 0;
	}
}


int validarTracks(tlcde* framesBuf, tlcde* lsTracks, tlcde* identities, int MaxTracks ){

	STTrack* Track = NULL;
	int valCount = 0;

	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
			// obtener Track
			Track = (STTrack*)obtener(i, lsTracks);
			// eliminar los tracks creados en t y sin asignación en t+1
			if( falsoTrack( Track ) ){
				dejarId( Track, identities );
				reasignarTracks(lsTracks, framesBuf, identities, NULL, i);
				deadTrack( lsTracks, i );
			}
			// validar tracks.
			if( (Track->Estado == CAM_CONTROL)&&(Track->EstadoCount == 200 ) ){
				Track->validez = true;
				valCount = valCount +1;
			}
			else Track->validez = false;
	}
	return valCount;

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


void despertarTrack( tlcde* framesBuf, tlcde* lsTracks, tlcde* lsIds ){

	STFrame* frameData0;
	STFrame* frameData1;
	STTrack* NewTrack;
	STTrack* SleepingTrack;

	float distancia;	// almacena la distancia entre el track nuevo y los que duermen.
	float menorDistancia = 100000; // almacena el valor de la menor distancia
	int masCercano; // almacena la posición de la lista del track nuevo más cercano al track durmiendo
	float direccion;
	int a; // para hayar la distancia
	int b; // idem
	int tiempoVivo;

	if( framesBuf->numeroDeElementos < 3 ) return;
	frameData1 = (STFrame*)obtener(framesBuf->numeroDeElementos-1, framesBuf);
//	frameData0 = (STFrame*)obtener(0, framesBuf);

	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
		// obtener Track
		NewTrack = (STTrack*)obtener(i, lsTracks);
		// comprobar si se han iniciado nuevos tracks. Los tracks que lleguen aqui
		// son posibles tracks válidos( no han sido eliminados en validar tracks), es decir,
		// lleva vivos 1 frames
		tiempoVivo = frameData1->num_frame - NewTrack->InitTime ;
		if( tiempoVivo == 1 ){ //deteccion de nuevos tracks posiblemente válidos
			for(int j = 0;j < lsTracks->numeroDeElementos ; j++){
				// si hay tracks durmiendo escogemos el nuevo track más cercano con un umbral
				SleepingTrack = (STTrack*)obtener(j, lsTracks);
				if( SleepingTrack->Estado == SLEEPING  ){ // && SleepingTrack->id <= MAX_TRACKS) ??si hay tracks durmiento
					// una vez se haya confirmado la validez de los tracks restringimos mas.

					// nos quedamos con el más cercano al punto donde se inicio el nuevo track
					a = NewTrack->InitPos.x - SleepingTrack->x_k_Pos->data.fl[0];
					b = NewTrack->InitPos.y - SleepingTrack->x_k_Pos->data.fl[1];
					EUDistance( a,b,&direccion, &distancia);
					if( (distancia < menorDistancia) && (distancia < MAX_JUMP) ){
						masCercano = j;
						menorDistancia = distancia;
					}
				}
			}
			//Si se cumple la condición reasignamos
			if ( menorDistancia <= MAX_JUMP){
				dejarId( NewTrack, lsIds );
				reasignarTracks( lsTracks, framesBuf,lsIds,i,masCercano );
				deadTrack( lsTracks, masCercano );
			}
		}
	}
}

void corregirTracks( tlcde* framesBuf, tlcde* lsTracks, tlcde* lsIds){

	STFrame* frameData0;
	STFrame* frameData1;
	STTrack* NewTrack;
	STTrack* SleepingTrack;

	int masAntiguo ;
	int tiempoVivo;
	// posición (tiempo) desde el que se van a examinar nuevos tracks
					// si fuese 0 se pisaría a despertarTrack
	frameData1 = (STFrame*)obtener(framesBuf->numeroDeElementos-1, framesBuf);


	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
		// obtener Track// de los durmiendo, buscamos asignar en primer lugar el de etiqueta más baja.
		// damos prioridad a los menores o iguales a maxTrack, como están en orden, recorremos la lista
		// en orden
		SleepingTrack = (STTrack*)obtener(i, lsTracks);
//		if( SleepingTrack->Estado == SLEEPING &&
//						SleepingTrack->id <= MAX_TRACKS &&
//						SleepingTrack->EstadoCount == MAX_BUFFER )

		if( SleepingTrack->Estado == SLEEPING &&
				SleepingTrack->EstadoCount == IMAGE_BUFFER_LENGTH-1 ){
			// intentamos asignarle un nuevo track
			// damos prioridad a los que tengan la menor etiqueta frente a los más altos,
					// que es lo  mismo que escojer el nuevo track más antiguo
			int ultimo = 0;

			for( int j = 0;j < lsTracks->numeroDeElementos ; j++){
				// de los nuevos asignamos el más antiguo
				NewTrack = (STTrack*)obtener(j, lsTracks);
				tiempoVivo = frameData1->num_frame - NewTrack->InitTime;
				if( tiempoVivo < IMAGE_BUFFER_LENGTH && tiempoVivo > ultimo ){
					masAntiguo = j;
					ultimo = tiempoVivo;
				}
			}
			if( ultimo != 0){
				dejarId( NewTrack, lsIds );
				reasignarTracks( lsTracks, framesBuf,lsIds, masAntiguo , i );
				deadTrack( lsTracks, i );
				continue;
				// si se ha conseguido asignar continuar al siguiente
			}
			//. Si no es que no hay nuevos tracks. En este caso
			// si su etiqueta es menor que maxTracks, no hacer nada. Esperar a que aparezca el nuevo track.
			// Nunca será eliminado un track de este tipo. Se intentará reasignar al más adecuado en cuanto
			// sea posible
			if(	SleepingTrack->id <= MAX_TRACKS ) continue;
			// en cambio si su etiqueta es mayor, si durante un tiempo no aparecen nuevos tracks, eliminarlo
			// , dejar su id y eliminar los datos de la fly que creó.
			else {
				dejarId( SleepingTrack, lsIds );
				reasignarTracks(lsTracks, framesBuf, lsIds, NULL, i);
				deadTrack( lsTracks, i );
			}
		}
		// limpieza

		//comprobar si hay dos tracks siguiendo a una fly durante un periodo mas o menos largo ( del buffer). Si es así,
		// si uno de ellos es un nuevo track track y su id es mayor de MAXTracks, eliminarlo, si su id es menor o igual
		// , poner el más jóven en estado SLEEP.

		// eliminar tracks que,  aunque sean válidos, tengan una id alta
		// y permanezcan largo tiempo inmóviles
		//
		// Si hay 2 tracks siguiendo a una fly en movimiento durante x frames.
		//	1) Comprobar si se ha creado recientemente un nuevo track.
		// 	   si es así, de los dos tracks, asignamos el más reciente a la fly del  nuevo track y lo eliminamos y reasignamos id
		// 	2) si el nuevo track es más antiguo que cualquiera de los dos o bien no se ha creado recientemente uno nuevo, lo eliminamos
		//  y reasignamos ids.


	}


}


/*!\brief Si recibe track nuevo:
 * Sustituye el track antiguo por el nuevo, actualiza los datos y del nuevo en base a los del antiguo.
 * si no hay trak nuevo:
 * Elimina todo rastro del track antiguo incluyendo el blob que ratreaba. Este caso es de tracks que seguian a blobs con una
 * alta probabilidad de ser falsos.
 *
 *
 *
 * @param lsTracks
 * @param nuevo
 * @param viejo
 * @param framesBuf
 */
void reasignarTracks( tlcde* lsTracks,tlcde* framesBuf, tlcde* lsIds , int nuevo, int viejo){

	// obtener posicion del buffer desde la que hay que corregir.
	STFrame* frameData1 = NULL;
	STTrack* NewTrack = NULL;
	STTrack* SleepingTrack = NULL;
	STFly* Fly = NULL;
	tlcde* FliesFrame = NULL;

	int posInit; // posición del buffer desde la que se inicia la correción

	// obtener Track//
	SleepingTrack = (STTrack*)obtener(viejo, lsTracks);
	if(nuevo) NewTrack= (STTrack*)obtener(nuevo, lsTracks);// Limpiar datos del track y su blob
	if(!NewTrack){
		posInit = framesBuf->numeroDeElementos - (SleepingTrack->FrameCount + 1);
		if(posInit < 0) posInit = 0;
		for( int i = posInit; i < framesBuf->numeroDeElementos; i++ ){
				// obtener frame
				frameData1 = (STFrame*)obtener(i, framesBuf);
				for( int j = 0;j < frameData1->Flies->numeroDeElementos ; j++){
					Fly = (STFly*)obtener(j, frameData1->Flies);
					if( Fly->etiqueta == SleepingTrack->id ){
						borrarEl(j,frameData1->Flies);
						if (Fly->Tracks ) free(Fly->Tracks);
						if (Fly->Stats) free(Fly->Stats);
						free(Fly); // borrar el área de datos del elemento eliminado
					}
				}
		}

	}
	else{
		// obtener posición desde la que se corrige la id
		posInit = framesBuf->numeroDeElementos - NewTrack->FrameCount;
		// corregir buffer
		// Usar la etiqueta, no fly sig, ya que sino se pisaría la etiqueta 0 ( KAM_CONTROL )
		for( int i = posInit; i < framesBuf->numeroDeElementos; i++ ){
			// obtener frame
			frameData1 = (STFrame*)obtener(i, framesBuf);
			for( int j = 0;j < frameData1->Flies->numeroDeElementos ; j++){
				Fly = (STFly*)obtener(j, frameData1->Flies);
				if( Fly->etiqueta == NewTrack->id ){
					Fly->etiqueta = SleepingTrack->id;
					Fly->Color = SleepingTrack->Color;
					Fly->dstTotal = SleepingTrack->dstTotal + Fly->dstTotal;
				}
			}
		}
		// ACTUALIZAR TRACK
		NewTrack->id = SleepingTrack->id;
		NewTrack->Color = SleepingTrack->Color;
		NewTrack->dstTotal = SleepingTrack->dstTotal + NewTrack->dstTotal;
		NewTrack->FrameCount = SleepingTrack->FrameCount + NewTrack->FrameCount;
		NewTrack->InitTime = SleepingTrack->InitTime;
		NewTrack->InitPos = SleepingTrack->InitPos;
	}
	// ELIMINAR SLEEPINGTRACK



}
void ReleaseDataTrack(  ){

	if(Identities){
		liberarIdentidades( Identities );
		free( Identities) ;
	}
	if(framesBuf) {
		liberarBuffer( framesBuf );
		free( framesBuf);
	}
	DeallocateKalman( lsTracks );
	free(lsTracks);

}


