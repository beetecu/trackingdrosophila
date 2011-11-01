/*
 * Tracking.hpp
 *
 *  Created on: 20/10/2011
 *      Author: chao
 */

#ifndef TRACKING_HPP_
#define TRACKING_HPP_

#include "VideoTracker.hpp"
#include "Libreria.h"
#include "hungarian.h"
#include <opencv2/video/background_segm.hpp>
#include "FlujoOptico.hpp"

void Tracking( tlcde* frammeBuf );

void MotionTemplate( IplImage* img, IplImage* dst);

///! brief Realiza la asignación de identidades. El primer parámetro es la lista
///! el segundo parámetro es un flag que indica qué individuos asignar. 1 para los
///! del foreground ( estado dinámico )  y 0 para los del oldforeground ( estado estático )
tlcde* matchingIdentity( tlcde* framesBuf ,int estado );

void AllocateTrackImages( IplImage *I );

void DeallocateTrackIm();

#endif /* TRACKING_HPP_ */
