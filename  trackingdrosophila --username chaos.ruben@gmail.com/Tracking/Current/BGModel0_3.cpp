/*!
 * BGModel.cpp
 *
 * Modelado de fondo mediante distribución Gaussiana
 *
 * Recibe un puntero tipo cv_Capture a la secuencia a modelar
 * Devuelve un puntero IplImage al modelo de fondo
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */

#include "BGModel.h"

int initBGGModel( CvCapture* t_capture, IplImage* BG, IplImage* ImMask){



	AllocateImagesBGM( BG );

	if ( !t_capture ) {
		error (1);
		return -1;
	}

	accumulateBackground( t_capture, BG, ImMask);

//
//	if ( num_frames < 1000 ) {
//
//						} else if ( frameCount == 1000 ) {
//								createModelsfromStats();
//						}
	DeallocateImagesBGM();
return 1;
}

void accumulateBackground( CvCapture* t_cap, IplImage* BGMod, IplImage* Mask) {


    int num_frames = 0;

	cvConvertScale( BGMod, Iscratch,1,0); // a float

	double Time = (double)cvGetTickCount();

	/// Acumulamos el fondo para obtener la mediana y la varianza de cada pixel en 20 frames  ////



	printf(" Rellenando matriz de estados...\n");
	while( num_frames < FRAMES_TRAINING ){
		IplImage* frame = cvQueryFrame( t_cap );
		if ( !frame ) {
			error(2);
			break;
		}
		printf(" Porcentaje completado: %f \r",(float) num_frames/(FRAMES_TRAINING-1) );
		if ( (cvWaitKey(10) & 255) == 27 ) break;

		PreProcesado( frame, ImGray, Mask, false);

	// Convertimos a float
		cvConvertScale( ImGray, Iscratch,1,0 );
		cvConvertScale( BGMod, Iscratch2,1,0 );
	// Invertimos la mascara para operar solo en el plato.
//		invertirBW( Mask  );
	// Inicializamos el fondo con la primera imagen
		if ( num_frames == 0 ){
			cvCopy( Iscratch, Iscratch2 ); // si inicializamos a cero seria la normal tipificada

		}
		cvShowImage("BG", Iscratch);
				cvWaitKey();
	// Estimamos la mediana y la varianza
		for (int y = 0; y< Iscratch->height; y++){
			// Nos movemos en altura
			        uchar* ptr = (uchar*) ( Iscratch->imageData + y*Iscratch->widthStep);
					uchar* ptr2 = (uchar*) ( Iscratch2->imageData + y*Iscratch2->widthStep);

					for (int x= 0; x<Iscratch->width; x++){ // Nos movemos en anchura
						// Incrementar o decrementar fondo en una unidad
						if ( ptr[x] < ptr2[x] ) ptr2[x] = ptr2[x]-1;
						if ( ptr[x] > ptr2[x] ) ptr2[x] = ptr2[x]+1;
					}
		}
		cvAbsDiff( Iscratch, Iscratch2, IdiffF);
		if ( num_frames == 0 )  cvCopy( IdiffF, IvarF);
		else {
			for (int y = 0; y< IdiffF->height; y++){
						// Nos movemos en altura
								uchar* ptr3 = (uchar*) ( IdiffF->imageData + y*IdiffF->widthStep);
								uchar* ptr4 = (uchar*) ( IvarF->imageData + y*IvarF->widthStep);
								for (int x= 0; x<IdiffF->width; x++){ // Nos movemos en anchura
									// Incrementar o decrementar fondo en una unidad
									if ( ptr3[x] < ptr4[x] ) ptr4[x] = ptr4[x]-1;
									if ( ptr3[x] > ptr4[x] ) ptr4[x] = ptr4[x]+1;
								}
			}
		}
		num_frames += 1;
		Time = (double)cvGetTickCount() - Time;
		printf( "Frame  %.1f Seg / Frame \n", Time/(cvGetTickFrequency()*1000000.) );

		//Obtenemos el vector recorriendo la matriz primero en profundidad con el bucle interno
	}
}



void AllocateImagesBGM( IplImage *I ) {  // I is just a sample for allocation purposes

        CvSize sz = cvGetSize( I );


        IvarF = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
        IdiffF = cvCreateImage( sz, IPL_DEPTH_32F, 1 );



        cvZero( IvarF );
        cvZero( IdiffF );

        cvZero( IhiF );
        cvZero( IlowF );


        Iscratch = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
        Iscratch2 = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
        ImGray = cvCreateImage( sz, 8, 1 );

        cvZero(Iscratch);
        cvZero(Iscratch2);
}

void DeallocateImagesBGM() {

        cvReleaseImage( &IvarF );
        cvReleaseImage( &IdiffF );


        cvReleaseImage( &Iscratch );
        cvReleaseImage( &Iscratch2 );
        cvReleaseImage( &ImGray );
}









