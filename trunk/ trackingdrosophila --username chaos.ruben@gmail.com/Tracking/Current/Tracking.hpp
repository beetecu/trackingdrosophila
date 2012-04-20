/*
 * Tracking.hpp
 *
 * *  Suposiciones:
 *	Hasta aquí el algoritmo trabaja solo con información espacial, es decir, los datos
 *	del instante t.
 *	En este punto disponemos de la estructura STFrame  ( frameDataIn ) que contiene los datos necesatios del frame t
 *	para poder iniciar el rastreo. Estos son:
	typedef struct {
		int num_frame; //!< Identificación del Frame procesado.
		IplImage* Frame;//!< Imagen fuente de 8 bit de niveles de gris preprocesada.
		IplImage* BGModel;//!< Imagen de 8 bits que contiene el  BackGround Model Dinámico.
		IplImage* IDesvf;//!< Imagen de 32 bits que contiene la Desviación Típica del modelo de fondo dinámico.
		IplImage* FG;  //!< Imagen que contiene el Foreground.
		IplImage* ImKalman;
		STStatFrame * Stats; //!< estadísticas.
		tlcde* Flies; //!< Puntero a lista circular doblemente enlazada (tlcde) con los datos de cada Mosca.

		tlcde* Tracks; //!< Puntero a lista circular doblemente enlazada (tlcde) con cada Trak. Creado por Tracking
	}STFrame;

 *	- En Tracking se mantienen en memoria las estructuras correspondientes a MAX_BUFFER frames ( framesBuff, buffer de datos  ).
 *	Los buffers son colas FIFO. Con esto añadimos la información temporal.
 *	- Asi mismo se crea una cola FIFO para almacenar los Tracks y otra para las
 *	identidades, de modo que éstas sean únicas.

	Tracking realiza las siguientes acciones:

	- Añade nuevo elemento al buffer de datos.
	- asignarIdentidades( lsTracks,frameDataIn->Flies): valida blobs y resuelve las asociaciones usando las predicciones
	  de kalman mediante el algoritmo Hungaro.
	- validarTracks( lsTracks, MAX_TRACKS ) Eliminar falsos tracks en base a la información temporal de t y t+1;
	- Kalman( frameDataIn , Identities, lsTracks):
		-  Crea un track para cada blob.
		-  Inicia un filtro de kalman por cada track.
		-  Genera nueva medida en base a los datos obtenidos de la camara y de asignar identidades.
		-  Filtra de la dirección y resuelve ambiguedad en la orientación.
		-  Actualiza de los parámetros de rastreo: Track y fly (en base a los nuevos datos).
		-  Realiza la predicción para t+1.
	- Aplicación de Heurísticas. Se usa la información temporal del buffer para decidir si eliminar o no tracks.

  *
  *
  *
 /// TIPOS DE TRACKS:

	1) Tracks activos: Serían tracks en estado CAM_CONTROL o KALMAN_CONTROL. Están recibiendo nuevos datos
		a) CAM_CONTROL: Tienen asignacion válida y única
		b) KALMAN_CONTROL: Tienen asignacion válidas pero no única.
	2) Tracks durmiendo: Han perdido el objetivo a rastrear

	Según el origen del track distinguimos:
	1) Tracks válidos: Generados por moscas. LLevan un tiempo rastreando con éxito.
	2) Tracks "posiblemente" válidos. Serían los nuevos tracks.No se sabe con certeza si han sido generados por moscas.
	2) Falsos tracks: Tracks generados por espurios o por "fantasmas (ghost)"

/// TRATAMIENTO DE NUEVOS TRACKS

	Cuando kalman origina un nuevo track t,en t+1 se le busca su primera asignación
	En validarTracks se eliminan los tracks creados en t y sin asignación única en t+1.
	Si durante x frames el track tiene asignaciones correctas, se considerará un track
	válido

	Despertando Tracks:
	DespertarTracks trabaja por un lado con los tracks durmiendo de t y por otro lado con los
 	Tracks nuevos, que no han sido eliminados por validarTrack, es decir, recien creados
 	y "posibles" tracks válidos( =>estan en CAM_CONTROL y EstateCount == 2).//
	Mientras haya tracks durmiendo, cada nuevo track creado en las inmediaciones
 	de un track durmiendo, se intercambiará la id con el track
 	durmiendo mas cercano y los parámetros de su fly serán actualizados con
 	los del track durmiendo y este será eliminado.

/// ELIMIRAR FALSOS TRACKS ///

	Cabe distinguir, en función de las características del blob que originó el falso track, los siguientes tipos:

	tipo 1) Según su tipo de movimiento:
		1.1) Blob Estático o espurio: Un reflejo inmóvil por ejemplo .
		1.2) Blob Móvil: Reflejo creado por una mosca que se mueve por el borde del plato ó un espurio que por
		aparece en t y vuelve a aparecer en t+1 en una posición lo suficientemente cercana.
	tipo 2) Según su duración
		2.1) Espurio o falso blob que aparece en t y desaparece en t+1.
		2.2) Espurios o falsos blobs que permanecen un tiempo inferior a la longitud del buffer.
		2.3) Falsos blobs que permanecen largo tiempo. Sería el caso de una mosca reflejada largo tiempo.

	TRACKS ACTIVOS

	TRACKS DURMIENDO:

	Supocición: Tras aplicar AsignarIdentidades se ha asignado un track ( que no esté durmiendo) a cada blob
  en base a las predicciones de kalman. Entonces Kalman crea un track por cada blob que queda sin asignar .
	Suponemos que se ha creado un falso track para un espurio ( que sobrevivió a la validación ) en t.
	Suponemos que en t+n el track no es eliminado.

  caso 1) No hay ningún blob lo suficientemente cerca como para ser asignado a dicho track.
		=> dicho espurio generaría un track en estado SLEEPING.
		Si no se encuentra dicha asignación en un intervalo de tiempo determinado, el track acabaría
		siendo eliminado en la fase de corrección. En caso contrario, nos encontraríamos con el  caso 2)

	caso 2) un blob que ya estaba siendo rastreado por un track con asignación única se encuentra lo
		suficientemente cerca del falso track como para que el blob también sea asignado a éste nuevo track.//
		=>1: en asignarIdentidades se interpretará que el blob oculta dos flies, y se intentará
		validar para dividir el blob en  2, lo cual sería erróneo. 2: Además, habría dos tracks
		siguiendo al mismo blob.
		Un ejemplo de este caso se da por ejemplo en un track creado por un reflejo. Recordar que asignar identidades no se
		asignan tracks en estado SLEEPING) Justo en el momento en que desaparece el reflejo, el track que lo seguía
		sería asignado al blob que causó el reflejo ya que dicho track estaría en estado CAM_CONTROL
		el reflejo . También se daría en el caso de un espurio que persiste durante dos frames consecutivos.

  Consecuencia: Hay que eliminar el nuevo Track. ( o bien antes de validar o bien usar la validación para detectar si se trata
	de un único blob con una probabilidad suficientemente alta ). Así establecemos flySig = NULL en el track del blob recien creado,
  de modo que sería automáticamente eliminado en validarTracks.

	Solución adoptada:

		- Tratamiento de  Falsos track de tipo 2.1 que dan lugar al caso 1.
		En validarTracks se eliminan los tracks creados en t y sin asignación única en t+1 con lo que el problema del track creado por un
 		espurio de tipo 1 quedaría solucionado. Si el nuevo track tiene asignación válida y única durante 2 frames será considerado un
		"posible" track válido. Si el track permanece un número determinado de frames con asignación única y sin entrar en conflicto con
		otros tracks, es decir, su estado es CAM_CONTROL, será considerado un track válido.



		- Para espurios del tipo 2.2 tambien se pueden dar el caso 1 y 2
		- Para espurios del tipo 2.3 casos 1 y 2

		corregirTracks
		 Aqui van a parar los tracks dormidos que no han podido ser despertados en despertar tracks.
			Despertar tracks trabaja por un lado con los tracks durmiendo de t y por otro lado con los
		 Tracks que no han sido eliminados por validarTrack, es decir, recien creados
		 y "posibles" tracks válidos( =>estan en CAM_CONTROL y EstateCount == 2).
		 Corregir tracks usa el buffer para intentar asignar aquellos tracks que no han sido asignados
		con tracks recien creados por que no cumplian la distancia minima.Se da prioridad a los tracks con
		mayor rango ( < = MAX_TRACKs )
 		transcurridos max buf ( countEstate = maxbuf ) de los nuevos asignamos el más antiguo.
		Si transcurre un periodo determinado sin que aparezcan nuevos traks, es decir, el track ya lleva
	durmiendo mucho tiempo:
			- Si su id es <= MAXTrack, se sigue esperando,
			- En cambio, si su id es mayor, se elimina.
	Aclaración:
 		El error en caso de eliminar un track correcto creado por un espurio de tipo 1 quedaría subsanado posteriormente ya que si en la
		t + 3 se vuelve	a detectar otro blob/espurio en el mismo punto, kalman crearía un nuevo track repitiendose el proceso. Únicamente
		si durante 2 iteraciones consecutivas el track tiene una nueva asignación y ésta es única se considerará que es un "posible" track válido
  	"Posible" ya que no se establecerá dicho track como válido hasta verificar que no se trata de un
 		espurio de tipo 2.

  Para el tipo 2)
		caso 1: El track será automáticamente eliminado si ha nacido hace poco.

		Suponemos ahora que tras validar track tenemos una medida de la validez del track.

		Una vez que se alcance el número máximo de tracks y todos ellos sean válidos, se será mucho más estricto y se dará prioridad
	a los tracks ya creados y verificados.

 		Aquí cabe la posibilidad de que una mosca que no se ha movido durante la detección del plato se mueva mínimamente
		en un instante dado. En este caso el track sería válido.

	Cuando un track pasa a estado sleeping deja su id. Si coincide que es necesrio asignar una nueva id en otro punto en el mismo
 	instante, esta id se asignará al nuevo track y sería erroneo.

 Falsos blobs que permanecen largo tiempo. Sería el caso de una mosca "mirandose al espejo".


 *  Created on: 20/10/2011
 *      Author: chao
 */

#ifndef TRACKING_HPP_
#define TRACKING_HPP_

#include "VideoTracker.hpp"
#include "Libreria.h"
#include "Visualizacion.hpp"

#include "hungarian.hpp"
#include "Kalman.hpp"
#include <opencv2/video/background_segm.hpp>
#include "AsignarIdentidades.hpp"
#include "FlujoOptico.hpp"

#define MAX_TRACKS 14
#define MAX_TIME_SLEPT 200

#define NUMBER_OF_IDENTITIES 100
#define IMAGE_BUFFER_LENGTH 50
#define ULTIMO IMAGE_BUFFER_LENGTH-1
#define PENULTIMO IMAGE_BUFFER_LENGTH-2
#define PRIMERO 0

/*!\brief	- Añade nuevo elemento al buffer de datos.
 - asignarIdentidades( lsTracks,frameDataIn->Flies): valida blobs y resuelve las asociaciones usando las predicciones
   de kalman mediante el algoritmo Hungaro.
 - validarTracks( lsTracks, MAX_TRACKS ) Elimina falsos tracks en base a la información temporal de t y t+1;
	- Kalman( frameDataIn , Identities, lsTracks):
		-  Crea un track para cada blob.
		-  Inicia un filtro de kalman por cada track.
		-  Genera nueva medida en base a los datos obtenidos de la camara y de asignar identidades.
		-  Filtra de la dirección y resuelve ambiguedad en la orientación.
		-  Actualiza de los parámetros de rastreo: Track y fly (en base a los nuevos datos).
		-  Realiza la predicción para t+1.
	- despertarTrack( tlcde* framesBuf, tlcde* lsTracks ):Usa la información temporal del buffer para decidir si despertar o no tracks.
	- corregirTracks( tlcde* framesBuf, tlcde* lsTracks, tlcde* lsIds). Decide si se eliminan o no tracks ( dormidos o no dormidos ) y
	hace las correcciones oportunas en el buffer

 * @param frameDataIn Estructura tipo STFrame que contiene las imagenes y datos del frame de entrada.
 * @param MaxTracks Numero máximo de tracks
 * @param BGModel Modelo de fondo
 * @param Writer Estructura necesaria para grabar partes de la salida
 * @return STFrame FrameDataOut. Devuelve estructura tipo STFrame que contiene las imagenes y datos del frame de salida
			0 Mientras el buffer no esté lleno;
 */

STFrame* Tracking( STFrame* frameDataIn, int MaxTracks,StaticBGModel* BGModel, CvVideoWriter* Writer );

/*! \brief Elimina tracks de espurios y blobs que aparecen en t y desaparecen en t+1.
 * Da una idea de la validez de un track en base al tiempo que esta en estado CAM_CONTROL
 *
 * 	-# Limpieza de tracks:
	- si se ha creado un track en t y en t+1 y :
		- no tiene asignación, es decir, flySig == NULL ( porque no tiene nueva medida )
			o porque sus posibles asignaciones están más alejadas de la maxima distancia permitida) o bien
		- la fly que se ha asignado al nuevo track está a su vez asignada a otro u otros tracks, es decir,
			se ha creado un nuevo track en t y en t+1 un blob que ya estaba siendo rastreado por un
			Track es asignado al nuevo track.
	- se elimina el track ( Se considera un espurio o de un reflejo momentáneo en el borde del plato ).
	-# Establecer validez para cada track: si Estado = CAM_CONTROL y EstadoCount = 200 => Valido = true

 * @param framesBuf
 * @param lsTracks
 * @param identities
 * @param MaxTracks
 *
 * @return el número de tracks que están catalogados como válidos
 */

int validarTracks(tlcde* framesBuf, tlcde* lsTracks, tlcde* identities, int MaxTracks );

/*!\brief Mientras haya tracks durmiendo, cada nuevo track creado en las inmediaciones
 de un track durmiendo, se intercambiará la id con el track durmiendo mas cercano. Los
  parámetros de su fly serán actualizados con los del track durmiendo, que finalmente
  será eliminado.

	-# Comprobar si se han iniciado nuevos tracks. Los nuevos tracks que lleguen aquí
	 son posibles tracks válidos( no han sido eliminados en validar tracks).
	-# Si hay tracks durmiendo escogemos el nuevo track más cercano con un umbral ( MAX_JUMP )
	 hayando la distancia Euclidea entre el origen del nuevo track y el track durmiendo.
	-# Si se cumple la condición reasignamos

 * @param framesBuf
 * @param lsTracks
 * @param lsIds
 */

void despertarTrack( tlcde* framesBuf, tlcde* lsTracks, tlcde* lsIds );

/*!\brief Corregir tracks usa el buffer para intentar reasignar aquellos tracks dormidos que no han
 *  sido asignados con tracks recien creados por que no cumplian la distancia minima.
	Los tracks del 1 al MAX_TRACKS son prioritarios.

    Aqui van a parar los tracks dormidos que no han podido ser despertados en despertar tracks.
	Despertar tracks trabaja por un lado con los tracks durmiendo de t y por otro lado con los
 Tracks que no han sido eliminados por validarTrack, es decir, recien creados
 y "posibles" tracks válidos( =>estan en CAM_CONTROL y EstateCount == 2).
 Si un Track ( con prioridad al de id más baja) lleva dormido max buf ( countEstate = maxbuf )
 de los nuevos asignamos el que lleve más tiempo vivo.
 En caso de no lograr la asignación por que no hay nuevos tracks se abren dos posibilidades:
	 1) Que el track tenga una id > MAX_TRAKS. Estos traks serán eliminados y se limpiará el buffer

	 2) Que su id sea < MAX_TRACK. Estos tracks son prioritarios. Se esperará hasta que tengan una
	 asignación válida.
 Si se ha creado una nueva, y su id es mayor que Max Tracks,  asignar el track sleeping a este blob y
 actualizar flies del buffer sin tener en cuenta la distancia. dejar la id y reasignar.

y este es válido ( OJO con esto. Si no verificamos
		// que es válido se puede asignar un track durmiendo a un track iniciado por un espurio)

 dar prioridad a los tracks con id mas baja.
 * @param framesBuf
 * @param lsTracks
 * @param lsIds
 */
void corregirTracks( tlcde* framesBuf, tlcde* lsTracks, tlcde* lsIds);

void reasignarTracks( tlcde* lsTracks,tlcde* framesBuf, tlcde* lsIds , int nuevo, int viejo);

void AllocateTrackImages( IplImage *I );

void ReleaseDataTrack( );



typedef struct{
	int etiqueta;
	CvScalar color;
}Identity;

#endif /* TRACKING_HPP_ */
