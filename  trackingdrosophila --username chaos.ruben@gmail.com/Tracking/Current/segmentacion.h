/*
 * segmentacion.h
 *
 *  Created on: 29/08/2011
 *      Author: german
 */

#ifndef SEGMENTACION_H_
#define SEGMENTACION_H_

#include "VideoTracker.hpp"
#include "Libreria.h"





void segmentacion(IplImage *Brillo,IplImage *mediana,IplImage *desviacion,IplImage *Foreg,CvRect Segroi);


#endif /* SEGMENTACION_H_ */
