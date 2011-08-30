/*
 * segmentacion.cpp
 *
 *  Created on: 29/08/2011
 *      Author: german
 */

#include "segmentacion.h"

void segmentacion(IplImage *Brillo,IplImage *mediana,IplImage *desviacion){

	cvNamedWindow("Segmentacion",1);
	IplImage *IDif=cvCloneImage(mediana); // imagen diferencia abs(I(pi)-u(p(i))
	IplImage *IDifm=cvCloneImage(mediana);// IDif en punto flotante
	IplImage *pesos=cvCloneImage(mediana);//Imagen resultado wi ( pesos)

	cvAbsDiff(Brillo,mediana,IDif);
	cvConvertScale(IDif ,IDifm,1,0);// A float
	cvDiv(IDifm,desviacion,pesos);

	cvShowImage("Segmentacion",IDifm);


}
