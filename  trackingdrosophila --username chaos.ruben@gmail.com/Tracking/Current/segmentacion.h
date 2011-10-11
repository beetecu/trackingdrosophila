/*
 * segmentacion.h
 *
 *  Created on: 29/08/2011
 *      Author: german
 */

#ifndef SEGMENTACION_H_
#define SEGMENTACION_H_

#include "VideoTracker.hpp"





int  segmentacion(IplImage *Brillo,STCapas* Capa, CvRect Roi,STFlies** FlieTemp);



#endif /* SEGMENTACION_H_ */
