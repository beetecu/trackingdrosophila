/*
 * Plato.cpp
 *
 * Recibe una imagen de un canal en niveles de gris
 * Detecta el plato y crea una máscara: pone a 255 los pixeles que estén fuera
 *
 * Devuelve 0 en caso de no encontrar un radio válido
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */

#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <stdio.h>
#include <math.h>

//#include "Tracking0_2.h"

using namespace cv;
using namespace std;

void MascaraPlato(CvCapture* t_capture, IplImage* Cap,
				int* centro_x, int* centro_y, int* radio){

	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* circles = NULL;
	IplImage* Im;

// LOCALIZACION

	int num_frames = 0;

//	t_capture = cvCaptureFromAVI( "Drosophila.avi" );
	if ( !t_capture ) {
		fprintf( stderr, "ERROR: capture is NULL \n" );
		getchar();
		return ;
	}

	// Obtención del centro de máximo radio del plato en 50 frames
	printf("Localizando plato... ");
//	GlobalTime = (double)cvGetTickCount() - GlobalTime;
//	printf( " %.1f\n", GlobalTime/(cvGetTickFrequency()*1000.) );

	while (num_frames < 50) {

		IplImage* frame = cvQueryFrame( t_capture );
		if ( !frame ) {
			fprintf( stderr, "ERROR: frame is null...\n" );
			getchar();
			break;
		}
		if ( (cvWaitKey(10) & 255) == 27 ) break;

		Im = cvCreateImage(cvSize(frame->width,frame->height),8,1);
//
		// Imagen a un canal de niveles de gris
		cvCvtColor( frame , Im, CV_BGR2GRAY);
		// Filtrado gaussiano 5x
		cvSmooth(Im,Im,CV_GAUSSIAN,5,0,0,0);

		circles = cvHoughCircles(Im, storage,CV_HOUGH_GRADIENT,4,5,100,300, 150,
				cvRound( Im->height/2 ));
		int i;
		for (i = 0; i < circles->total; i++)
		{

			// Excluimos los radios que se salgan de la imagen
			 float* p = (float*)cvGetSeqElem( circles, i );
			 if(  (  cvRound( p[0]) < cvRound( p[2] )  ) ||
				  ( ( Im->width - cvRound( p[0]) ) < cvRound( p[2] )  ) ||
				  (  cvRound( p[1] )  < cvRound( p[2] ) ) ||
				  ( ( Im->height - cvRound( p[1]) ) < cvRound( p[2] ) )
				) continue;

			 // Buscamos el de mayor radio de entre todos los frames;
			 if (  *radio < cvRound( p[2] ) ){
				 *radio = cvRound( p[2] );
				 *centro_x = cvRound( p[0] );
				 *centro_y = cvRound( p[1] );
			 }
		}
		num_frames +=1;
		cvClearMemStorage( storage);
	}
	cvReleaseMemStorage( &storage );

	printf("	Plato localizado : ");
	printf("Centro x : %d , Centro y %d : , Radio: %d \n"
			,*centro_x,*centro_y, *radio);

	// Creacion de mascara

	printf("Creando máscara del plato... ");
//	GlobalTime = (double)cvGetTickCount() - GlobalTime;
//	printf( " %.1f\n", GlobalTime/(cvGetTickFrequency()*1000.) );
	for (int y = 0; y< Cap->height; y++){
		//puntero para acceder a los datos de la imagen
		uchar* ptr = (uchar*) ( Cap->imageData + y*Cap->widthStep);
		// Los pixeles fuera del plato se ponen a 0;
		for (int x= 0; x<Cap->width; x++){
			if ( sqrt( pow( abs( x - *centro_x ) ,2 )
					+ pow( abs( y - *centro_y ), 2 ) )
					> *radio) ptr[x] = 255;
			else	ptr[x] = 0;
		}
	}
	return;
}








