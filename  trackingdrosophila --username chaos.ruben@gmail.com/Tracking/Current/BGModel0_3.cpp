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



	int num_frames = 0;

	double Time = (double)cvGetTickCount();

	/// Acumulamos el fondo para obtener la mediana y la varianza de cada pixel en 20 frames  ////

	printf(" Rellenando matriz de estados...\n");
	while( num_frames < FRAMES_TRAINING ){
		IplImage* frame = cvQueryFrame( t_capture );
		if ( !frame ) {
			error(2);
			break;
		}
		printf("Porcentaje completado: %f % \r",(float) ( num_frames/(FRAMES_TRAINING-1) )*100 );
		if ( (cvWaitKey(10) & 255) == 27 ) break;

		PreProcesado( frame, ImGray, ImMask, false);

		// Inicializamos el fondo con la primera imagen
		if ( num_frames == 0 ){
			cvCopy( ImGray, BG ); // si inicializamos a cero seria la normal tipificada
		}

		accumulateBackground( ImGray, BG );

		num_frames += 1;

		//		Time = (double)cvGetTickCount() - Time;
		//		printf( "Frame  %.1f Seg / Frame \n", Time/(cvGetTickFrequency()*1000000.) );


	}
	//
	//	if ( num_frames < 1000 ) {
	//
	//						} else if ( frameCount == 1000 ) {
	//								createModelsfromStats();
	//						}

	return 1;
}

void accumulateBackground( IplImage* ImGray, IplImage* BGMod) {

	static int first = 1;
	// Estimamos la mediana y la desviación típica
	for (int y = 0; y< ImGray->height; y++){
		// Nos movemos en altura
		uchar* ptr = (uchar*) ( ImGray->imageData + y*ImGray->widthStep);
		uchar* ptr2 = (uchar*) ( BGMod->imageData + y*BGMod->widthStep);
		for (int x= 0; x<ImGray->width; x++){ // Nos movemos en anchura
			// Incrementar o decrementar fondo en una unidad
			if ( ptr[x] < ptr2[x] ) ptr2[x] = ptr2[x]-1;
			if ( ptr[x] > ptr2[x] ) ptr2[x] = ptr2[x]+1;
		}
	}
	cvAbsDiff( ImGray, BGMod, IdiffF);
	if ( first == 1 ){
		cvCopy( IdiffF, IvarF);
		first = 0;
	}
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
}

void UpdateBackground( IplImage * tmp_frame, IplImage* bg_model){

	accumulateBackground( tmp_frame, bg_model );

}

void AllocateImagesBGM( IplImage *I ) {  // I is just a sample for allocation purposes

        CvSize sz = cvGetSize( I );

        IvarF = cvCreateImage( sz, 8, 1 );
        IdiffF = cvCreateImage( sz, 8, 1 );

        cvZero( IvarF );
        cvZero( IdiffF );
        cvZero( IhiF );
        cvZero( IlowF );

        ImGray = cvCreateImage( sz, 8, 1 );
}

void DeallocateImagesBGM() {

        cvReleaseImage( &IvarF );
        cvReleaseImage( &IdiffF );
        cvReleaseImage( &ImGray );
}









