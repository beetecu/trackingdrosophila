/*
 * AsignarIdentidades.hpp
 *
 *  Created on: 02/09/2011
 *      Author: chao
 */

#ifndef ASIGNARIDENTIDADES_HPP_
#define ASIGNARIDENTIDADES_HPP_

#include "VideoTracker.hpp"
#include "opencv2/video/tracking.hpp"
#include "Libreria.h"



typedef struct{
	int etiqueta;
	CvScalar color;
}Identity;

void MotionTemplate( tlcde* framesBuf,tlcde* Etiquetas );

///! brief Realiza la asignación de identidades. El primer parámetro es la lista
///! el segundo parámetro es un flag que indica qué individuos asignar. 1 para los
///! del foreground ( estado dinámico )  y 0 para los del oldforeground ( estado estático )
STFly* matchingIdentity( tlcde* framesBuf ,tlcde* Etiquetas, CvRect MotionRoi, double angle );

void EUDistance( CvPoint posicion1, CvPoint posicion2, float* direccion, float* distancia );

void CrearIdentidades(tlcde* Etiquetas);

static Scalar randomColor(RNG& rng);

void liberarIdentidades(tlcde* lista);

void asignarNuevaId( STFly* fly, tlcde* identities);

void dejarId( STFly* fly, tlcde* identities );

void mostrarIds( tlcde* Ids);

void enlazarFlies( STFly* flyAnterior, STFly* flyActual, tlcde* ids = NULL );

void SetTita( STFly* flyAnterior,STFly* flyActual, double angle );

void allocateMotionTemplate( IplImage* im);

void releaseMotionTemplate();

void TrackbarSliderMHI(  int pos );

void TrackbarSliderDMin(  int pos );

void TrackbarSliderDMax(  int pos );

#endif /* ASIGNARIDENTIDADES_HPP_ */
