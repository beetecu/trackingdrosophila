/*
 * segmentacion.cpp
 *
 *  Created on: 29/08/2011
 *      Author: german
 */

#include "segmentacion.h"


void segmentacion(IplImage *Brillo,IplImage *mediana,IplImage *desviacion,IplImage *Foreg,CvRect Segroi){


	cvNamedWindow("Segmentacion",1);

	IplImage *IDif=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1); // imagen diferencia abs(I(pi)-u(p(i))
	IplImage *IDifm=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1);// IDif en punto flotante
	IplImage *pesos=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1);//Imagen resultado wi ( pesos)
	IplImage *FGTemp=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1);// Imagen copia del foreground

	cvCopy(Foreg,FGTemp);

	//Crear storage y secuencia de los contornos

	CvMemStorage* storage = cvCreateMemStorage();
	CvSeq* first_contour=NULL;

	cvAbsDiff(Brillo,mediana,IDif);// |I(p)-u(p)|/0(p)
	cvConvertScale(IDif ,IDifm,1,0);// A float
	cvDiv(IDifm,desviacion,pesos);// Calcular wi

	//Buscamos los contornos de las moscas en movimiento en el foreground

	int Nc = cvFindContours(FGTemp,storage,&first_contour,sizeof(CvContour),CV_RETR_EXTERNAL);
	printf( "\n Total Contornos Detectados: %d\n", Nc );

	for( CvSeq *c=first_contour; c!=NULL; c=c->h_next) {

		float z=0;
		float mul=0;
		float u=0;
		CvScalar w; // valor del brillo pixel w
		CvScalar p; // valor del peso del pixel p
		CvScalar covar; // valor de los pixels dela matrix covarianza

		CvRect rect=cvBoundingRect(c,0); // Hallar los rectangulos para establecer las ROIs
		Segroi=cvRect(rect.x,rect.y,rect.width,rect.height);

		cvSetImageROI( FGTemp, Segroi );
		cvSetImageROI( pesos,Segroi);

		float matrix_p[pesos->roi->height][pesos->roi->width];// Alamcena el brillo de cada pix
															// del contorno float matrix_dif[pesos->roi->height][pesos->roi->width];

		float matrix_w[pesos->roi->height][pesos->roi->width];//Almacena el valor wi

		float matrix_dif[pesos->roi->height][pesos->roi->width]; // pi-u
		float matrix_tras[pesos->roi->height][pesos->roi->width];// (pi-u)T
		float matrix_mult[pesos->roi->height][pesos->roi->height];// (pi-u)(pi-u)T


		//Crear las matrices como estructuras CvMat

		CvMat *matrix=cvCreateMat(pesos->roi->height,pesos->roi->width,CV_32FC1);// matrix_p
		CvMat *traspuesta=cvCreateMat(pesos->roi->width,pesos->roi->height,CV_32FC1);//matrix_tras
		CvMat *Mult=cvCreateMat(matrix->rows,traspuesta->cols,CV_32FC1);//matrix_mult
		CvMat *matrix_wi=cvCreateMat(pesos->roi->height,pesos->roi->width,CV_32FC1);//matrix_w
		CvMat *MC=cvCreateMat(Mult->rows,matrix_wi->cols,CV_32FC1);//Matriz resultado

		float matrix_covar[MC->rows][MC->cols];
		CvMat *MATRIX_COVAR=cvCreateMat(Mult->rows,matrix_wi->cols,CV_32FC1);

		// Hallar Z y u

			for (int x= 0; x<pesos->roi->height; x++){ // Recorrer la Roi
				for (int y= 0; y<pesos->roi->width; y++){

					w=cvGet2D(pesos,x,y); // Sacar el valor del brillo de cada pixel
					p=cvGet2D(FGTemp,x,y);//Sacar e valor del peso de cada pixel

					z=z + w.val[0]; // Sumatorio Z, Parametro Z
					mul=mul + (w.val[0]*p.val[0]);//Sumatorio w * p

					matrix_p[x][y]=p.val[0]; // Almacenar el valor del brillo del pixel en la
										   //matriz

					matrix_w[x][y]=w.val[0];// Almacena el valor wi del pixel en la matriz
				}

			}

			u=mul/z; // Parametro u
			//printf("\n El parametro u = %f",u);

			//Bucles para meter las matrices en las estructuras CvMat

			for (int x = 0; x< pesos->roi->height; x++){ // Recorrer la ROI
					for (int y= 0; y< pesos->roi->width; y++){

						matrix_dif[x][y] =(matrix_p[x][y]-u);
						matrix_tras[y][x]=matrix_dif[x][y];
						matrix_mult[x][y]=NULL;

						 float elemento_wi=matrix_w[x][y];
						*((float*)CV_MAT_ELEM_PTR( *matrix_wi, x, y ))=elemento_wi;

						float elemento=matrix_dif[x][y];
						*((float*)CV_MAT_ELEM_PTR( *matrix, x, y ))=elemento;

						float elemento_tras=matrix_tras[y][x];
						*((float*)CV_MAT_ELEM_PTR( *traspuesta, y, x ))=elemento_tras;

					}
			}

			//Multiplicar matrices

			cvGEMM(matrix,traspuesta,1,NULL,0,Mult,0);// (p-u)*(p-u)T = Mult

			cvGEMM(Mult,matrix_wi,1,NULL,0,MC,0);// wi*(p-u)*(p-u)T = MC

			// Calcular la matriz de covarianza

			for(int x=0;x<MC->rows;x++){
				for(int y=0;y<MC->cols;y++){

					covar=cvGet2D(MC,x,y);
					matrix_covar[x][y]=covar.val[0]/z;

					 float elemento_covar=matrix_covar[x][y]; // Matriz covarianza como CvMat
					 *((float*)CV_MAT_ELEM_PTR( *MATRIX_COVAR, x, y ))=elemento_covar;
				}
			}


			cvResetImageROI(FGTemp);
			cvResetImageROI(pesos);

			cvReleaseMat(&matrix);
			cvReleaseMat(&traspuesta);
			cvReleaseMat(&Mult);
			cvReleaseMat(&matrix_wi);
			cvReleaseMat(&MC);
			cvReleaseMat(&MATRIX_COVAR);

	} // Fin del for de Contornos

			cvReleaseImage(&IDif);
			cvReleaseImage(&IDifm);
			cvReleaseImage(&pesos);

} // Fin de la funcion segementacion
