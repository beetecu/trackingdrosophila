/*
 * segmentacion.cpp
 *
 *  Created on: 29/08/2011
 *      Author: german
 */

#include "segmentacion.h"

void segmentacion(IplImage *Brillo,IplImage *mediana,IplImage *desviacion){

	cvNamedWindow("Segmentacion",1);
	IplImage *IDif=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1); // imagen diferencia abs(I(pi)-u(p(i))
	IplImage *IDifm=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1);// IDif en punto flotante
	IplImage *pesos=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1);//Imagen resultado wi ( pesos)

	cvAbsDiff(Brillo,mediana,IDif);
	cvConvertScale(IDif ,IDifm,1,0);// A float
	cvDiv(IDifm,desviacion,pesos);

	cvShowImage("Segmentacion",pesos);


}
