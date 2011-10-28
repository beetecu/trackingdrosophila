/*
 * Tracking.cpp
 *
 *  Created on: 20/10/2011
 *      Author: chao
 */

#include "Tracking.hpp"

void Tracking( tlcde* framesBuf ){

	struct timeval ti, tf, tif, tff; // iniciamos la estructura
	int tiempoParcial;
	STFrame* frameData = NULL;
	STFly* flyData = NULL;
	static int workPos = 0; // punto de trabajo en el buffer
	/// TRACKING
	IplImage *ImOpFlowX;
	IplImage *ImOpFlowY;


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
	irAl( workPos, framesBuf);
	// cargar datos del frame
	frameData = ( STFrame* )obtenerActual( framesBuf );
	gettimeofday(&ti, NULL);

	framesBuf = matchingIdentity( framesBuf , 0 );


	cvZero(frameData->ImMotion);
	if ( SHOW_MOTION_TEMPLATE == 1){
		MotionTemplate( frameData->FG, frameData->ImMotion);
	}
	ImOpFlowX = cvCreateImage( cvGetSize( frameData->FG ) ,IPL_DEPTH_32F,1 );
	ImOpFlowY = cvCreateImage(cvGetSize( frameData->FG ) ,IPL_DEPTH_32F,1 );

	cvZero( ImOpFlowX );
	cvZero( ImOpFlowY );
//		OpticalFlowLK( frameData->FG, ImOpFlowX, ImOpFlowY );
	if (SHOW_OPTICAL_FLOW == 1){
	cvShowImage( "Flujo Optico X", ImOpFlowX );
	cvShowImage( "Flujo Optico Y", ImOpFlowY);
	}
	gettimeofday(&tf, NULL);
	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
											(tf.tv_usec - ti.tv_usec)/1000.0;
	printf("Tracking: %5.4g ms\n", tiempoParcial);

	cvReleaseImage( &ImOpFlowX );
	cvReleaseImage( &ImOpFlowY );
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
