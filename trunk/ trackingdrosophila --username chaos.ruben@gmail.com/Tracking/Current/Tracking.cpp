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

	/// ELIMIRAR FALSOS TRACKS ///
	// únicamente serán validos aquellos tracks que los primeros instantes tengan asignaciones válidas
	// y unicas. Si no es así se considera un trackDead:

	frameDataIn->numTracks = validarTracks( lsTracks,Identities, MAX_TRACKS );


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

	frameDataIn->Tracks = lsTracks;

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

//	despertarTrack(framesBuf, lsTracks );

	// SI BUFFER LLENO
	if( framesBuf->numeroDeElementos == IMAGE_BUFFER_LENGTH ){

		irAlPrincipio( framesBuf );
		// Establecer en base a diversos parámetros la validez del track.
		//
		// Comprobar si se reinicia un track  o se elimina
		// Si existen tracks durmiendo durante un tiempo x ( mirar contador)
		// y  se ha creado un nuevo track en algún punto, y este es válido ( OJO con esto. Si no verificamos
		// que es válido se puede asignar un track durmiendo a un track iniciado por un espurio)
		// eliminar el nuevo ( trackdead) y asignar la fly que rastreaba
		// el track eliminado al track durmiendo con prioridad al más cercano.
		// Reasignar etiquetas.

//		corregirTracks(framesBuf,  lsTracks, Identities );

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

/// ELIMIRAR FALSOS TRACKS ///

//	Supocicion: Tras aplicar AsignarIdentidades se ha asignado un track ( que no esté durmiendo) a cada blob
//  en base a las predicciones de kalman. Entonces Kalman crea un track por cada blob que queda sin asignar .
//	Suponemos que se ha creado un falso track para un espurio ( que sobrevivió a la validación ) en t.
//	Suponemos que en t+1 el track no es eliminado.

//  caso 1) No hay ningún blob lo suficientemente cerca como para ser asignado a dicho track.
//		=> dicho espurio generaría un track en estado SLEEPING.
//		Si no se encuentra dicha asignación en un intervalo de tiempo determinado, el track acabaría
//		siendo eliminado en la fase de corrección. En caso contrario, nos encontraríamos con el  caso b)

//	caso 2) un blob que ya estaba siendo rastreado por un track con asignación única se encuentra lo
//		suficientemente cerca del falso track como para que el blob también sea asignado a éste nuevo track.//
//		=>1: en asignarIdentidades se interpretará que el blob oculta dos flies, y se intentará
//		validar para dividir el blob en  2, lo cual sería erróneo. 2: Además, habría dos tracks
//		siguiendo al mismo blob.
//		Un ejemplo de este caso se da por ejemplo en un track creado por un reflejo. Recordar que asignar identidades no se
//		asignan tracks en estado SLEEPING) Justo en el momento en que desaparece el reflejo, el track que lo seguía
//		sería asignado al blob que causó el reflejo ya que dicho track estaría en estado CAM_CONTROL
//		el reflejo . También se daría en el caso de un espurio que persiste durante dos frames consecutivos.

//  Consecuencia: Hay que eliminar el nuevo Track. ( o bien antes de validar o bien usar la validación para detectar si se trata
//	de un único blob con una probabilidad suficientemente alta ). Así establecemos flySig = NULL en el track del blob recien creado,
//  de modo que sería automáticamente eliminado en validarTracks.

//	Solución adoptada:
//	Cabe distinguir entre dos tipos de espurios.
//	1) Espurio que aparece en t y desaparece en t+1
//	2) Espurio que permanece 2 o más frames. Aquí podemos distinguir:
//		2.1) Estático: Un reflejo inmóvil por ejemplo o espurio que permanece más de 2 frames.
//		2.2) Móvil: Reflejo creado por una mosca que se mueve por el borde del plato.//
//
//	El caso 1) se presentará con espurios de tipo 1 y el caso 2 con espurios de tipo 2

//	Solución para espurios de tipo 1)
//		En validar tracks se eliminan los tracks creados en t y sin asignación única en t+1 con lo que el problema del track creado por un
// 		espurio de tipo 1 quedaría solucionado. Si el nuevo track tiene asignación válida y única durante 2 frames será considerado un
//		"posible" track válido. Si el track permanece un número determinado de frames con asignación única y sin entrar en conflicto con
//		otros tracks, es decir, su estado es CAM_CONTROL, será considerado un track válido.
//
//	Aclaración:
// 		El error en caso de eliminar un track correcto creado por un espurio de tipo 1 quedaría subsanado posteriormente ya que si en la
//		t + 3 se vuelve	a detectar otro blob/espurio en el mismo punto, kalman crearía un nuevo track repitiendose el proceso. Únicamente
//		si durante 2 iteraciones consecutivas el track tiene una nueva asignación y ésta es única se considerará que es un "posible" track válido
//  	"Posible" ya que no se establecerá dicho track como válido hasta verificar que no se trata de un
// 		espurio de tipo 2.
//  Para el tipo 2) Para espurios del tipo 2 tambien se pueden dar el caso 1 y 2.
//		caso 1: El track será automáticamente eliminado si ha nacido hace poco.

//		Suponemos ahora que tras validar track tenemos una medida de la validez del track.

//	Una vez que se alcance el número máximo de tracks y todos ellos sean válidos, se será mucho más estricto y se dará prioridad
//	a los tracks ya creados y verificados.




// 		Aquí cabe la posibilidad de que una mosca que no se ha movido durante la detección del plato se mueva mínimamente
//		en un instante dado. En este caso el track sería válido.

//	Cuando un track pasa a estado sleeping deja su id. Si coincide que es necesrio asignar una nueva id en otro punto en el mismo
// 	instante, esta id se asignará al nuevo track y sería erroneo.






// 1)si se ha creado un track en t y en t+1:
//		a) no tiene asignación, es decir, flySig == NULL ( porque no tiene nueva medida )
//			o porque sus posibles asignaciones están más alejadas de la maxima distancia permitida) o bien
//		b) la fly que se ha asignado al nuevo track está a su vez asignada a otro u otros tracks, es decir,
//		 	se ha creado un nuevo track en t y en t+1 un blob que ya estaba siendo rastreado por un
//			Track es asignado al nuevo track.
//   - se elimina el track ( Se considera un espurio o de un reflejo momentáneo en el borde del plato ).
int validarTracks( tlcde* lsTracks, tlcde* identities, int MaxTracks ){

	STTrack* Track = NULL;
	int valCount = 0;

	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
			// obtener Track
			Track = (STTrack*)obtener(i, lsTracks);
			// eliminar los tracks creados en t y sin asignación en t+1
			if( falsoTrack( Track ) ){
				dejarId( Track, identities );
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

// estamos al comienzo del buffer.
// mientras haya tracks durmiendo, cada nuevo track creado en las inmediaciones
// de un track durmiendo, se intercambiará la id con el track
// durmiendo mas cercano y los parámetros de su fly serán actualizados con
// los del track durmiendo y este será eliminado.
void despertarTrack( tlcde* framesBuf, tlcde* lsTracks ){

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


	if( framesBuf->numeroDeElementos < 3 ) return;
	frameData1 = (STFrame*)obtener(framesBuf->numeroDeElementos-1, framesBuf);
//	frameData0 = (STFrame*)obtener(0, framesBuf);

	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
		// obtener Track
		NewTrack = (STTrack*)obtener(i, lsTracks);
		// comprobar si se han iniciado nuevos tracks. Los tracks que lleguen aqui
		// son posibles tracks válidos( no han sido eliminados en validar tracks)
		if( NewTrack->EstadoCount == 2 ){ //deteccion de nuevos tracks posiblemente válidos
			for(int j = 0;j < lsTracks->numeroDeElementos ; j++){
				// si hay tracks durmiendo escogemos el nuevo track más cercano con un umbral y
				// en primer lugar aquellos cuya id sea menor?? //  (que el número máximo de tracks ??.)
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
			//esperamos a que se verifique si es válida.??
			if ( menorDistancia <= MAX_JUMP) reasignarTracks( lsTracks,i,masCercano, framesBuf );
		}
	}
}
// Aqui van a parar los tracks dormidos que no han podido ser despertados en despertar tracks.
//	Despertar tracks trabaja por un lado con los tracks durmiendo de t y por otro lado con los
// Tracks que no han sido eliminados por validarTrack, es decir, recien creados
// o "posibles" tracks válidos( =>estan en CAM_CONTROL y EstateCount == 2).
// Corregir tracks usa el buffer para intentar asignar
// aquellos tracks que no han sido asignados con tracks recien creados por que no cumplian la distancia minima.
// transcurridos max buf ( countEstate = maxbuf ) de los nuevos asignamos el más antiguo.
// 	Si se ha creado una nueva, y su id es mayor que Max Tracks,  asignar el track sleeping a este blob y
// actualizar flies del buffer sin tener en cuenta la distancia. dejar la id y reasignar.
//
// dar prioridad a los tracks con id mas baja.
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

	// ordenar la lista lsTracks por ids
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
				reasignarTracks( lsTracks, masAntiguo , i , framesBuf);
				continue;
				// si se ha conseguido asignar continuar al siguiente
			}
			//. Si no es que no hay nuevos tracks. En este caso
			// si su etiqueta es menor que maxTracks, no hacer nada. Esperar a que aparezca el nuevo track.
			if(	SleepingTrack->id <= MAX_TRACKS ) continue;
			// en cambio si su etiqueta es mayor, si durante un tiempo no aparecen nuevos tracks, eliminarlo
			// y dejar su id
			else {
				tiempoVivo = frameData1->num_frame - SleepingTrack->InitTime;
				if (tiempoVivo > MAX_TIME_SLEPT ){
					dejarId( SleepingTrack, lsIds );
					deadTrack( lsTracks, i );
				}
			}
		}


	}


}

// comprobar si hay dos tracks siguiendo a una fly durante un periodo mas o menos largo ( del buffer). Si es así, si uno de ellos es un nuevo track
// track y su id es mayor de MAXTracks,

void reasignarTracks( tlcde* lsTracks, int nuevo, int viejo,tlcde* framesBuf){

	// obtener posicion del buffer desde la que hay que corregir.


	// actualizar distancias

	// actualizar origen
	// actualizar numero de frame
	// actualizar tiempos de inicio y posición de inicio.



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


