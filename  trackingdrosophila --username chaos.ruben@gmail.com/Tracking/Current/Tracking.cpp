/*
 * Tracking.cpp
 *
 *  Created on: 20/10/2011
 *      Author: chao
 */

#include "Tracking.hpp"

	IplImage *ImOpFlowX;
	IplImage *ImOpFlowY;
	IplImage *ImagenA;
	IplImage *ImagenB;

void Tracking( tlcde* framesBuf ){

	struct timeval ti, tf, tif, tff; // iniciamos la estructura
	int tiempoParcial;
	STFrame* frameData = NULL;
	STFly* flyData = NULL;
	static int workPos = 0; // punto de trabajo en el buffer
	/// TRACKING


	frameData = ( STFrame* )obtenerActual( framesBuf );

	AllocateTrackImages( frameData->FG );

	hungarian_t prob;

	// el rastreo no se inicia hasta que el buffer tenga almenos 3 elementos
	if( framesBuf->numeroDeElementos < 3) return;
	// Mientras no se llene el buffer, solo se incrementa la posición de trabajo
	// cada dos frames (cuado el numero de elementos es impar ).
	// el buffer tiene un numero de elementos impar. El punto de trabajo será el
	// centro.
	if( ( framesBuf->numeroDeElementos < IMAGE_BUFFER_LENGTH)&&
			( !(framesBuf->numeroDeElementos%2) )) return;
	workPos = (framesBuf->numeroDeElementos + 1)/2 -1;

	// acceder al punto de trabajo
	irAl( ULTIMO , framesBuf);
	// cargar datos del frame
	frameData = ( STFrame* )obtenerActual( framesBuf );
	gettimeofday(&ti, NULL);

	framesBuf = matchingIdentity( framesBuf , 0 );


	cvZero(frameData->ImMotion);
	if ( SHOW_MOTION_TEMPLATE == 1){
		MotionTemplate( frameData->FG, frameData->ImMotion);
	}


	if (SHOW_OPTICAL_FLOW == 1){
		irAlFinal( framesBuf);
		frameData = ( STFrame* )obtenerActual( framesBuf );
		cvCopy(frameData->FG,ImagenB);
		//cvCvtColor( frameData->Frame, ImagenB, CV_BGR2GRAY);
		irAlAnterior( framesBuf);
		frameData = ( STFrame* )obtenerActual( framesBuf );
		cvCopy(frameData->FG,ImagenA);
	//	cvCvtColor( frameData->Frame, ImagenA, CV_BGR2GRAY);
	//	LKOptFlow( frameData->FG, ImOpFlowX, ImOpFlowY );
		PLKOptFlow( ImagenA, ImagenB, ImOpFlowX );
	cvShowImage( "Flujo Optico X", ImOpFlowX );
	cvShowImage( "Flujo Optico Y", ImOpFlowY);
	}

	gettimeofday(&tf, NULL);
	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
											(tf.tv_usec - ti.tv_usec)/1000.0;
	printf("Tracking: %5.4g ms\n", tiempoParcial);


	irAlFinal( framesBuf );
}

tlcde* matchingIdentity( tlcde* framesBuf ,int estado ){

	tlcde* FliesFrActual;
	tlcde* FliesFrSig;

	if ( estado == 1){

	}
	else{

	}
}

void AllocateTrackImages( IplImage *I ) {  // I is just a sample for allocation purposes

        CvSize sz = cvGetSize( I );
        if( !ImOpFlowX ||
        		ImOpFlowX->width != sz.width ||
        		ImOpFlowX->height != sz.height ) {

        		cvReleaseImage( &ImOpFlowX);
        		cvReleaseImage( &ImOpFlowY);
        		cvReleaseImage( &ImagenA);
        		cvReleaseImage( &ImagenB);

        		ImOpFlowX = cvCreateImage( sz,IPL_DEPTH_32F,1 );
        		ImOpFlowY = cvCreateImage(sz ,IPL_DEPTH_32F,1 );
        		ImagenA = cvCreateImage( sz ,8,1 );
        		ImagenB = cvCreateImage(sz ,8,1 );

        		cvZero( ImOpFlowX );
        		cvZero( ImOpFlowY );
        		cvZero( ImagenA );
        		cvZero( ImagenB );

        	}
		cvZero( ImOpFlowX );
		cvZero( ImOpFlowY );
		cvZero( ImagenA );
		cvZero( ImagenB );

}
void DeallocateTrackIm(){
		cvReleaseImage( &ImOpFlowX);
    		cvReleaseImage( &ImOpFlowY);
    		cvReleaseImage( &ImagenA);
    		cvReleaseImage( &ImagenB);
}
