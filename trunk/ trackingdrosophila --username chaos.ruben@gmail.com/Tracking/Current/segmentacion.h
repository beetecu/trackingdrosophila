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

#define PI 3.14159265

//CvMemStorage* storage = 0;
	IplImage *IDif = 0;
	IplImage *IDifm = 0;
	IplImage *pesos = 0;
	IplImage *FGTemp = 0;
	typedef struct {
		IplImage* BGModel;  ///BackGround Model
		IplImage* IDesv;
		IplImage* OldFG; ///OldForeGround ( objetos estáticos )
		IplImage* FGTemp; /// Imagen a segmentar y validar
		IplImage* FG;  ///Foreground ( objetos en movimiento )
		IplImage* ImFMask; /// Mascara del plato
		IplImage* ImRois;
		IplImage* ImMotion;
	}STCapas;

void  segmentacion(IplImage *Brillo,STCapas* Capa,CvRect Segroi);


#endif /* SEGMENTACION_H_ */
