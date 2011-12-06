/*
 * Tracking.cpp
 *
 *	Hasta aquí el algoritmo trabaja solo con información espacial
 *	En este punto añadimos la información temporal
 *  Created on: 20/10/2011
 *      Author: chao
 */

#include "Tracking.hpp"
#include "Kalman.hpp"


#define NUMBER_OF_MATRIX 6

	IplImage *ImOpFlowX;
	IplImage *ImOpFlowY;
	IplImage *ImagenA;
	IplImage *ImagenB;
	IplImage *IKalman;

	tlcde* Identities = NULL;

void Tracking( tlcde* framesBuf ){

	struct timeval ti; // iniciamos la estructura
	float tiempoParcial;
	STFrame* frameData = NULL;

	bool firstbuf = false;

	static int workPos = -1; // punto de trabajo en el buffer

	// Inicializar
	// creamos una cola Lifo de identidades
	if(!Identities) {
		Identities = ( tlcde * )malloc( sizeof(tlcde ));
		iniciarLcde( Identities );
		CrearIdentidades(Identities);
		//mostrarIds( Identities );
	}
	if( framesBuf->numeroDeElementos < 1) return;
	irAlFinal( framesBuf );
	frameData = ( STFrame* )obtenerActual( framesBuf );

	AllocateTrackImages( frameData->FG );

	hungarian_t prob;



	// Establecemos el estado de los frames que se han ido al oldfg



	/// resolvemos la ambiguedad en la orientación y hacemos una primera
	/// asignación de identidad mediante una plantilla de movimiento.
	gettimeofday(&ti, NULL);
	printf("\t1)Motion Template\n");
	cvZero(frameData->ImMotion);
	MotionTemplate( framesBuf,Identities );//frameData->FG, frameData->ImMotion
	tiempoParcial= obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo total: %5.4g ms\n", tiempoParcial);


	// el rastreo no se inicia hasta que el buffer tenga almenos 25 elementos
	if( framesBuf->numeroDeElementos < 25) return;
	// Cuando el buffer esté por la mitad comienza el rastreo en el segundo frame
	// La posición de trabajo se irá incremetando a la par que el tamaño del buffer
	// asta quedar finalmente situada en el centro.
	if ( framesBuf->numeroDeElementos < IMAGE_BUFFER_LENGTH)
			workPos += 1;

	if(workPos == 0) firstbuf=true;

	////// FILTRO DE KALMAN //////////////

	Kalman(framesBuf,workPos,IKalman);

	///////   FASE DE CORRECCIÓN  /////////
	// esta etapa se encarga de hacer la asignación de identidad definitiva
	// recibe información temporal de los últimos 48 frames ( Longitud_buffer
	// - frame_Inicio - frame_final - (frame_final-1) que son aprox 2 seg )
	//

	//Reasignamos idendidades.
	// enlazamos objetos etiquetados como nuevos con los que estaban parados


	if (workPos == PRIMERO) return;

//	framesBuf = matchingIdentity( framesBuf , 0 );

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



	////////// FASE DE PREDICCIÓN /////////

	// aplicamos kalman a objetos en movimiento

	irAlFinal( framesBuf );
}

void AllocateTrackImages( IplImage *I ) {  // I is just a sample for allocation purposes

        CvSize sz = cvGetSize( I );
        if( !ImOpFlowX ||
        		ImOpFlowX->width != sz.width ||
        		ImOpFlowX->height != sz.height ) {

        		cvReleaseImage( &ImOpFlowX);
        		cvReleaseImage( &ImOpFlowY);
        		cvReleaseImage( &IKalman);
        		cvReleaseImage( &ImagenA);
        		cvReleaseImage( &ImagenB);

        		ImOpFlowX = cvCreateImage( sz,IPL_DEPTH_32F,1 );
        		ImOpFlowY = cvCreateImage(sz,IPL_DEPTH_32F,1 );
        		IKalman = cvCreateImage( sz,8,3 );
        		ImagenA = cvCreateImage( sz ,8,1 );
        		ImagenB = cvCreateImage(sz ,8,1 );

        		cvZero( ImOpFlowX );
        		cvZero( ImOpFlowY );
        		cvZero( IKalman);
        		cvZero( ImagenA );
        		cvZero( ImagenB );
        	}
		cvZero( ImOpFlowX );
		cvZero( ImOpFlowY );
		cvZero(IKalman);
		cvZero( ImagenA );
		cvZero( ImagenB );

}
void ReleaseDataTrack(){
		if(Identities) liberarIdentidades( Identities );
		if (Identities) free( Identities) ;
		cvReleaseImage( &ImOpFlowX);
    		cvReleaseImage( &ImOpFlowY);
    		cvReleaseImage( &ImagenA);
    		cvReleaseImage( &ImagenB);
    		releaseMotionTemplate();
}
