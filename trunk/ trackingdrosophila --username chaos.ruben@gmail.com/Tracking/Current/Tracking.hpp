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

#define MAX_TRACKS 14
#define NUMBER_OF_IDENTITIES 100
#define IMAGE_BUFFER_LENGTH 50
#define ULTIMO IMAGE_BUFFER_LENGTH-1
#define PENULTIMO IMAGE_BUFFER_LENGTH-2
#define PRIMERO 0
//!\brief
//!
//!	- Añade nuevo elemento al buffer de datos.
//! - asignarIdentidades( lsTracks,frameDataIn->Flies): valida blobs y resuelve las asociaciones usando las predicciones
//!   de kalman mediante el algoritmo Hungaro.
//! - validarTracks( lsTracks, MAX_TRACKS ) Eliminar falsos tracks en base a la información temporal de t y t+1;
//!	- Kalman( frameDataIn , Identities, lsTracks):
//!		-  Crea un track para cada blob.
//!		-  Inicia un filtro de kalman por cada track.
//!		-  Genera nueva medida en base a los datos obtenidos de la camara y de asignar identidades.
//!		-  Filtra de la dirección y resuelve ambiguedad en la orientación.
//!		-  Actualiza de los parámetros de rastreo: Track y fly (en base a los nuevos datos).
//!		-  Realiza la predicción para t+1.
//!	- Heurísticas. Se usa la información temporal del buffer para decidir si eliminar o no tracks.
/*!
	  \param framesBuff : lista lineal doblemente enlazada que se usará como buffer de datos.
						 Almacena estructuras de tipo STFrame
	  \param FrameDataIn: estructura tipo STFrame que contiene las imagenes y datos del frame de entrada.

	   \return  STFrame FrameDataOut. Devuelve estructura tipo STFrame que contiene las imagenes y datos del frame de salida
				0 Mientras el buffer no esté lleno;
		*/
STFrame* Tracking( STFrame* frameDataIn, int MaxTracks,StaticBGModel* BGModel, CvVideoWriter* Writer );

void validarTracks( tlcde* lsTracks, tlcde* identities, int MaxTracks );


void AllocateTrackImages( IplImage *I );

void ReleaseDataTrack( );



typedef struct{
	int etiqueta;
	CvScalar color;
}Identity;

#endif /* TRACKING_HPP_ */
