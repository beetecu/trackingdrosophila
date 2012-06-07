/*
 * Tracking.hpp
 *
 *  ****************  TRACKING ****************
 *



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

#define ULTIMO 2
#define PENULTIMO 1

#define BORRAR 0
#define RE_ETIQUETAR 1
#define UPDATE_STATS 2
#define ONLY_UPDATE_IDS 3

void SetTrackingParams( ConvUnits* calParams );

void SetDefaultTrackParams(   );

void SetPrivateTrackParams( ConvUnits* calParams );

void ShowTrackParams( char* Campo );


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
 * @param Frames por segundo
 * @return STFrame FrameDataOut. Devuelve estructura tipo STFrame que contiene las imagenes y datos del frame de salida
			0 Mientras el buffer no esté lleno;
 */

STFrame* Tracking( STFrame* frameDataIn, int MaxTracks,StaticBGModel* BGModel, int FPS );

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

int validarTracks(tlcde* framesBuf, tlcde* lsTracks, tlcde* identities, int MaxTracks, int numFrame );

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

void reasignarTracks( tlcde* lsTracks,tlcde* framesBuf, tlcde* lsIds , int nuevo, int viejo, int Accion );

/*!brief Ordena la lista tracks de forma creciente
 *
 * @param lsTracks
 */
void ordenarTracks( tlcde* lsTracks );

void SetTrackingParams( int FPS, float mmTOpixel );

void AllocateTrackImages( IplImage *I );

void ReleaseDataTrack( );



typedef struct{
	int etiqueta;
	CvScalar color;
}Identity;

#endif /* TRACKING_HPP_ */
