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

//CvMemStorage* storage = 0;
	IplImage *IDif = 0;
	IplImage *IDifm = 0;
	IplImage *pesos = 0;
	IplImage *FGTemp = 0;

void segmentacion(IplImage *Brillo,IplImage *mediana,IplImage *desviacion,IplImage *Foreg,CvRect Segroi);


#endif /* SEGMENTACION_H_ */
