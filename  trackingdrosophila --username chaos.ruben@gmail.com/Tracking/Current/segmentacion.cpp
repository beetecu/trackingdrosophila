/*
 * segmentacion.cpp
 *
 *  Created on: 29/08/2011
 *      Author: german
 */

#include "segmentacion.h"

void segmentacion(IplImage *Brillo,IplImage *mediana,IplImage *desviacion,IplImage *Foreg,CvRect Segroi){

	int edge_thresh = 1; // Por si se utiliza Canny


	cvNamedWindow("Segmentacion",1);

	IplImage *IDif=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1); // imagen diferencia abs(I(pi)-u(p(i))
	IplImage *IDifm=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1);// IDif en punto flotante
	IplImage *pesos=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1);//Imagen resultado wi ( pesos)
	IplImage *FGTemp=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1);

	cvCopy(Foreg,FGTemp);

	//Crear storage y secuencia de los contornos

	CvMemStorage* storage = cvCreateMemStorage();
	CvSeq* first_contour=NULL;

	cvAbsDiff(Brillo,mediana,IDif);
	cvConvertScale(IDif ,IDifm,1,0);// A float
	cvDiv(IDifm,desviacion,pesos);

	//cvShowImage("Segmentacion",pesos);

	//cvCanny(Foreg, Foreg, (float)edge_thresh, (float)edge_thresh*3, 5);
	//cvShowImage("Segmentacion",Foreg);

	//Buscamos los contornos de las moscas en movimiento en el foreground

	int Nc = cvFindContours(FGTemp,storage,&first_contour,sizeof(CvContour),CV_RETR_EXTERNAL);
	printf( "\n Total Contornos Detectados: %d\n", Nc );

	for( CvSeq *c=first_contour; c!=NULL; c=c->h_next) {

		float z=0;
		float mul=0;
		float u=0;
		CvScalar w;
		CvScalar p;



		CvRect rect=cvBoundingRect(c,0); // Hallar los rectangulos para establecer las ROIs
		Segroi=cvRect(rect.x,rect.y,rect.width,rect.height);

		cvSetImageROI( FGTemp, Segroi );
		cvSetImageROI( pesos,Segroi);

		//cvShowImage("Segmentacion",FGTemp);

		// Hallar Z y u

			for (int y = 0; y<pesos->roi->height; y++){ // Recorrer la Roi
				for (int x= 0; x<pesos->roi->width; x++){

					w=cvGet2D(pesos,y,x); // Sacar el valor del peso de cada pixel
					p=cvGet2D(FGTemp,y,x);

					z=z + w.val[0]; // Sumatorio Z, Parametro Z
					mul=mul + w.val[0]*p.val[0];//Sumatorio w * p
				}

			}

			u=mul/z; // Parametro u
			printf("\n El parametro u = %f",u);

			cvResetImageROI(FGTemp);
			cvResetImageROI(pesos);

	} // Fin del for de Contornos

} // Fin de la funcion segementacion
