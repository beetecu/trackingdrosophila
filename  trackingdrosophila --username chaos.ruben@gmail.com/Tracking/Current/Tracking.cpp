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
TrackingParams* trackParams = NULL;

STFrame* Tracking( STFrame* frameDataIn, int MaxTracks,StaticBGModel* BGModel, int FPS ){


	STFrame* frameDataOut = NULL; // frame de salida

#ifdef MEDIR_TIEMPOS
	struct timeval ti,tif; // iniciamos la estructura
	float tiempoParcial;
	gettimeofday(&tif, NULL);
	printf("\n2)Tracking:\n");
	gettimeofday(&ti, NULL);
	printf("\t1)Asignación de identidades\n");
#endif



	////////////// AÑADIR AL BUFFER /////////////
	anyadirAlFinal( frameDataIn, framesBuf);
//	MotionTemplate( framesBuf,Identities );

	////////////// ASIGNACIÓN DE IDENTIDADES ///////////

	//APLICAR EL METODO DE OPTIMIZACION HUNGARO A LA MATRIZ DE PESOS
	// Asignar identidades.
	// resolver las asociaciones usando las predicciones de kalman mediante el algoritmo Hungaro
	// Si varias dan a la misma etiquetarla como 0. Enlazar flies.
	// Se trabaja en las posiciones frame MAX_BUFFER - 1 y MAX_BUFFER -2.
	if( frameDataIn->num_frame == 424 ){
						printf("hola");
					}
	if( frameDataIn->num_frame == 433 ){
							printf("hola");
						}
	if( frameDataIn->num_frame == 1424 ){
						printf("hola");
					}
	if( frameDataIn->num_frame == 1456 ){
							printf("hola");
						}
	if( frameDataIn->num_frame == 1456 ){
								printf("hola");
							}
	asignarIdentidades( lsTracks,frameDataIn->Flies);
	ordenarTracks( lsTracks );
#ifdef MEDIR_TIEMPOS
	tiempoParcial= obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo: %5.4g ms\n", tiempoParcial);
	gettimeofday(&ti, NULL);
	fprintf(stderr,"\t2)Filtro de Kalman\n");
#endif

	/////////////// ELIMIRAR FALSOS TRACKS ///
	// únicamente serán validos aquellos tracks que los primeros instantes tengan asignaciones válidas
	// y unicas. Si no es así se considera un trackDead:
	frameDataIn->numTracks = validarTracks( framesBuf, lsTracks,Identities, trackParams->MaxBlobs, frameDataIn->num_frame );

	/////////////// FILTRO DE KALMAN //////////////
	// El filtro de kalman trabaja en la posicion MAX_BUFFER -1. Ultimo elemento anyadido.

	Kalman( frameDataIn , Identities, lsTracks, trackParams);

	frameDataIn->Tracks = lsTracks;

#ifdef MEDIR_TIEMPOS
	tiempoParcial= obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo: %5.4g ms\n", tiempoParcial);
#endif

	///////   FASE DE CORRECCIÓN. APLICACION DE HEURISTICAS  /////////

	despertarTrack(framesBuf, lsTracks, Identities );
	ordenarTracks( lsTracks );

	// SI BUFFER LLENO
	if( framesBuf->numeroDeElementos == trackParams->MaxBuffer ){

		corregirTracks(framesBuf,  lsTracks, Identities );


	////////// LIBERAR MEMORIA  ////////////
		frameDataOut = (STFrame*)liberarPrimero( framesBuf ) ;
		if(!frameDataOut){error(7); exit(1);}

		VisualizarEl( framesBuf, PENULTIMO, BGModel );

#ifdef MEDIR_TIEMPOS
		tiempoParcial = obtenerTiempo( tif , NULL);
		printf("Tracking correcto.Tiempo total %5.4g ms\n", tiempoParcial);
#endif
		return frameDataOut;
	}
	else {
		VerEstadoBuffer( frameDataIn->Frame, framesBuf->numeroDeElementos, trackParams->MaxBuffer);
		VisualizarEl( framesBuf, PENULTIMO, BGModel );
#ifdef MEDIR_TIEMPOS
		tiempoParcial = obtenerTiempo( tif , NULL);
		printf("Tracking correcto.Tiempo total %5.4g ms\n", tiempoParcial);
#endif
		return 0;
	}
}


int validarTracks(tlcde* framesBuf, tlcde* lsTracks, tlcde* identities, int MaxTracks, int numFrame ){

	STTrack* Track = NULL;
	int valCount = 0;

	if( lsTracks->numeroDeElementos < 1) return 0;
	irAlPrincipio( lsTracks);
	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
			// obtener Track
			Track = (STTrack*)obtenerActual( lsTracks);
			// eliminar los tracks creados en t y sin asignación en t+1
			if( falsoTrack( Track, numFrame ) ){
				dejarId( Track, identities );
				reasignarTracks(lsTracks, framesBuf, identities, NULL, i);
				deadTrack( lsTracks, i );
			}
			// validar tracks.
			if( (Track->Stats->Estado == CAM_CONTROL)&&(Track->Stats->EstadoCount == 200 ) ){
				Track->validez = true;
				valCount = valCount +1;
			}
			else Track->validez = false;
			irAlSiguiente( lsTracks);
	}
	return valCount;

}

int falsoTrack( STTrack* Track, int numFrame ){

	int tiempoVivo = numFrame - Track->Stats->InitTime ;

	if( tiempoVivo == 1 ){
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
	if( lsTracks->numeroDeElementos <= trackParams->MaxBlobs  ) return;
	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
		// obtener Track
		NewTrack = (STTrack*)obtener(i, lsTracks);
		// comprobar si se han iniciado nuevos tracks. Los tracks que lleguen aqui
		// son posibles tracks válidos( no han sido eliminados en validar tracks), es decir,
		// llevan vivos 1 frames y tienen asignación válida
		tiempoVivo = frameData1->num_frame - NewTrack->Stats->InitTime ;
		if( tiempoVivo == 1 ){ //deteccion de nuevos tracks posiblemente válidos
			for(int j = 0;j < lsTracks->numeroDeElementos ; j++){
				// si hay tracks durmiendo escogemos el nuevo track más cercano con un umbral
				SleepingTrack = (STTrack*)obtener(j, lsTracks);
				if( SleepingTrack->Stats->Estado == SLEEPING  ){ // && SleepingTrack->id <= trackParams->MaxBlobs) ??si hay tracks durmiento
					// una vez se haya confirmado la validez de los tracks restringimos mas.

					// nos quedamos con el más cercano al punto donde se inicio el nuevo track
					a = NewTrack->Stats->InitPos.x - SleepingTrack->x_k_Pos->data.fl[0];
					b = NewTrack->Stats->InitPos.y - SleepingTrack->x_k_Pos->data.fl[1];
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
//		if( SleepingTrack->Stats->Estado == SLEEPING &&
//						SleepingTrack->id <= trackParams->MaxBlobs &&
//						SleepingTrack->EstadoCount == MAX_BUFFER )

		if( SleepingTrack->Stats->Estado == SLEEPING &&
				SleepingTrack->Stats->EstadoCount == trackParams->MaxBuffer-1 ){
			// intentamos asignarle un nuevo track
			// damos prioridad a los que tengan la menor etiqueta frente a los más altos,
				// que es lo  mismo que escojer el nuevo track más antiguo
			int ultimo = 0;

			for( int j = 0;j < lsTracks->numeroDeElementos ; j++){
				// de los nuevos asignamos el más antiguo
				NewTrack = (STTrack*)obtener(j, lsTracks);
				tiempoVivo = frameData1->num_frame - NewTrack->Stats->InitTime;
				if( tiempoVivo < trackParams->MaxBuffer && tiempoVivo > ultimo ){
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
			if(	SleepingTrack->id <= trackParams->MaxBlobs ) continue;
			// en cambio si su etiqueta es mayor, si durante un tiempo no aparecen nuevos tracks, eliminarlo
			// , dejar su id y eliminar los datos de la fly que creó.
			else {
				dejarId( SleepingTrack, lsIds );
				reasignarTracks(lsTracks, framesBuf, lsIds, -1, i);
				deadTrack( lsTracks, i );
			}
		}
		// limpieza

		//comprobar si hay dos tracks siguiendo a una fly durante un periodo mas o menos largo ( del buffer). Si es así,
		// si uno de ellos es un nuevo track  y su id es mayor de MAXTracks, eliminarlo, si su id es menor o igual
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
 * Sustituye el track antiguo por el nuevo, actualiza los datos del nuevo en base a los del antiguo.
 * si no hay trak nuevo:
 * Elimina todo rastro del track antiguo incluyendo el blob que rastreaba. Este caso es de tracks que seguian a blobs con una
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
	if(nuevo != -1) NewTrack= (STTrack*)obtener(nuevo, lsTracks);// Limpiar datos del track y su blob
	if(NewTrack == NULL){ // en caso de que no haya nuevo track, se trata de eliminar el antiguo y su fly
		posInit = (framesBuf->numeroDeElementos-1) -  NewTrack->Stats->FrameCount  ;
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
	else{ // se actualiza el nuevo con los datos del antiguo y se continua con el nuevo
		// obtener posición desde la que se corrige la id
		posInit = framesBuf->numeroDeElementos -  NewTrack->Stats->FrameCount ;
		// corregir buffer
		if(posInit < 0) posInit = 0;
		// Usar la etiqueta, no fly sig, ya que sino se pisaría la etiqueta 0 ( KAM_CONTROL )
		for( int i = posInit; i < framesBuf->numeroDeElementos; i++ ){
			// obtener frame
			frameData1 = (STFrame*)obtener(i, framesBuf);
			for( int j = 0;j < frameData1->Flies->numeroDeElementos ; j++){
				Fly = (STFly*)obtener(j, frameData1->Flies);
				if( Fly->etiqueta == NewTrack->id ){
					Fly->etiqueta = SleepingTrack->id;
					Fly->Color = SleepingTrack->Color;
					Fly->dstTotal = SleepingTrack->Stats->dstTotal + Fly->dstTotal;
				}
			}
		}
		// ACTUALIZAR TRACK
		NewTrack->id = SleepingTrack->id;
		NewTrack->Color = SleepingTrack->Color;
		NewTrack->Stats->dstTotal = SleepingTrack->Stats->dstTotal + NewTrack->Stats->dstTotal;
		NewTrack->Stats->FrameCount = SleepingTrack->Stats->FrameCount + NewTrack->Stats->FrameCount;
		NewTrack->Stats->InitTime = SleepingTrack->Stats->InitTime;
		NewTrack->Stats->InitPos = SleepingTrack->Stats->InitPos;
		NewTrack->Stats->TimeBlobOff = SleepingTrack->Stats->TimeBlobOff + NewTrack->Stats->TimeBlobOff;
		NewTrack->Stats->TimeBlobOn = SleepingTrack->Stats->TimeBlobOn + NewTrack->Stats->TimeBlobOn;
	}
}

void ordenarTracks( tlcde* lsTracks ){

	STTrack* Track1;
	STTrack* Track2;
	int permutacion = 1;

	while( permutacion){
		permutacion = 0;
		for( int i = 1; i < lsTracks->numeroDeElementos; i++){
			Track1 = (STTrack*)obtener(i-1, lsTracks);
			Track2 = (STTrack*)obtener(i, lsTracks);
			if( Track1->id > Track2->id   ){
				permutacion = 1;
				Track1 = (STTrack*)borrarEl(i-1, lsTracks); // borra el elemento i-1. Actual apunta al sig (i)
				insertar( (STTrack*)Track1,lsTracks); // se inserta el track 1 a continuación del actual(i)
			}
		}
	}
}

void SetTrackingParams(  ){

    //init parameters
	config_t cfg;
	config_setting_t *setting;
	char settingName[30];
	char configFile[30];
	char settingFather[30];

	int EXITO;
	int DEFAULT = false;

	// Reservar memoria
	if( !trackParams){
		trackParams = ( TrackingParams *) malloc( sizeof( TrackingParams) );
		if(!trackParams) {error(4); return;}
	}

	fprintf(stderr, "\nCargando parámetros para Tracking...");
	config_init(&cfg);

	sprintf( configFile, "config.cfg");
	sprintf( settingFather,"TrackingParams" );

	 /* Leer archivo. si hay un error, informar y cargar configuración por defecto */
	if(! config_read_file(&cfg, configFile))
	{
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
				config_error_line(&cfg), config_error_text(&cfg));

		fprintf(stderr, "Error al acceder al fichero de configuración %s .\n"
						" Estableciendo valores por defecto.\n"
						,configFile);
		DEFAULT = true;
	}
	else
	{
		setting = config_lookup(&cfg, settingFather);
		/* Si no se se encuentra la setting o bien existe la variable hijo Auto y ésta es true, se establecen TODOS los valores por defecto.*/
		if(setting != NULL)
		{
			sprintf(settingName,"Auto");
			/* Obtener el valor */
			EXITO = config_setting_lookup_bool ( setting, settingName, &DEFAULT);
			if(!EXITO) DEFAULT = true;
			else if( EXITO && DEFAULT ) fprintf(stderr, "\n Opción Auto activada para el campo %s.\n"
												" Estableciendo valores por defecto.\n",settingFather);
			else if( EXITO && !DEFAULT) fprintf(stderr, "\n Opción Auto desactivada para el campo %s.\n"
												" Estableciendo valores del fichero de configuración.\n",settingFather);
		}
		else {
			DEFAULT = true;
			fprintf(stderr, "Error.No se ha podido leer el campo %s.\n"
							" Estableciendo valores por defecto.\n",settingFather);
		}
	}

	if( DEFAULT ) SetDefaultTrackParams(  );
	/* Valores leídos del fichero de configuración. Algunos valores puedes ser establecidos por defecto si se indica
	 * expresamente en el fichero de configuración. Si el valor es erroneo o no se encuentra la variable, se establecerán
	 * a los valores por defecto.
	 */
	else{
		 /* Get the store name. */
		sprintf(settingName,"MaxBlobs");
		if(! config_setting_lookup_int ( setting, settingName, &trackParams->MaxBlobs )  ){
			trackParams->MaxBlobs = 10;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,trackParams->MaxBlobs );

		}

		sprintf(settingName,"ShowKalmanData");
				if(! config_setting_lookup_bool ( setting, settingName, &trackParams->ShowKalmanData )  ){
					trackParams->ShowKalmanData = false;
					fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
									"Establecer por defecto a %d \n",settingName,trackParams->ShowKalmanData );

		}

		sprintf(settingName,"MaxTimeSlept");
		if(! config_setting_lookup_int ( setting, settingName, &trackParams->MaxTimeSlept  )  ){
			trackParams->MaxTimeSlept = 200;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,trackParams->MaxTimeSlept );

		}

		sprintf(settingName,"MaxBuffer");
		if(! config_setting_lookup_int ( setting, settingName, &trackParams->MaxBuffer )  ){
			trackParams->MaxBuffer = 50;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,trackParams->MaxBuffer);
		}

		sprintf(settingName,"NumberOfIdentities");
		if(! config_setting_lookup_int ( setting, settingName, &trackParams->NumberOfIdentities )  ){
			trackParams->NumberOfIdentities = 100;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,trackParams->NumberOfIdentities);
		}

		sprintf(settingName,"PeriodoVelMed");
		if(! config_setting_lookup_int ( setting, settingName, &trackParams->PeriodoVelMed )  ){
			trackParams->PeriodoVelMed = 15 ;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,trackParams->PeriodoVelMed );
		}

		sprintf(settingName,"MediumActivityTh");
		if(! config_setting_lookup_float ( setting, settingName, &trackParams->MediumActivityTh )  ){
			trackParams->MediumActivityTh	= 2.0 ;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %0.1f \n",settingName,trackParams->MediumActivityTh);

		}

		sprintf(settingName,"LowActivityTh");
		if(! config_setting_lookup_float ( setting, settingName, &trackParams->LowActivityTh )  ){
			trackParams->LowActivityTh	= 0.5 ;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %0.1f \n",settingName,trackParams->LowActivityTh);

		}

		sprintf(settingName,"NullActivityTh");
		if(! config_setting_lookup_float ( setting, settingName, &trackParams->NullActivityTh )  ){
			trackParams->NullActivityTh	= 0.2 ;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %0.1f \n",settingName,trackParams->NullActivityTh);
		}

	}

	SetPrivateTrackParams(  );

	ShowTrackParams( settingFather );
	config_destroy(&cfg);
}

void SetDefaultTrackParams(   ){

	trackParams->ShowKalmanData = false;
	trackParams->MaxBlobs = 10;
	trackParams->MaxTimeSlept = 200;
	trackParams->MaxBuffer = 50;
	trackParams->NumberOfIdentities = 100;
	trackParams->PeriodoVelMed = 15 ;
	trackParams->MediumActivityTh	= 2 ;
	trackParams->LowActivityTh	= 0.5 ;
	trackParams->NullActivityTh	= 0.2 ;
}

void ShowTrackParams( char* Campo ){

	printf(" \nVariables para el campo %s : \n", Campo);
	printf(" -MaxBlobs = %d \n", trackParams->MaxBlobs);
	printf(" -MaxTimeSlept = %d \n",trackParams->MaxTimeSlept);
	printf(" -MaxBuffer = %d \n", trackParams->MaxBuffer );
	printf(" -NumberOfIdentities = %d \n",  trackParams->NumberOfIdentities);
	printf(" -PeriodoVelMed= %d \n", trackParams->PeriodoVelMed );
	printf(" -MediumActivityTh = %0.3f \n", trackParams->MediumActivityTh );
	printf(" -LowActivityTh  = %0.3f \n",  trackParams->LowActivityTh );
	printf(" -NullActivityTh  = %0.3f \n", trackParams->NullActivityTh );

}

void SetPrivateTrackParams(  ){

	/////////////// Inicializar /////////////////

	// cola Lifo de identidades
	if(!Identities) {
		Identities = ( tlcde * )malloc( sizeof(tlcde ));
		if( !Identities ) {error(4);exit(1);}
		iniciarLcde( Identities );
		CrearIdentidades(Identities, trackParams->NumberOfIdentities);
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
	free( trackParams);
	releaseAI();

}


