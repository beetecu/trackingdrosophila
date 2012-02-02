/*
 * Tracking.cpp
 *
 *	Hasta aquí el algoritmo trabaja solo con información espacial
 *	En este punto añadimos la información temporal
 *  Se mantienen en memoria las estructuras correspondientes a MAX_BUFFER frames
 *	( buffer de datos  ) e IMAGE_BUFFER_LENGHT ( buffer de imagenes ) .
 *	Los buffers son colas FIFO
 *
 *  Created on: 20/10/2011
 *      Author: chao
 */

#include "Tracking.hpp"
#include "Kalman.hpp"



	IplImage *ImOpFlowX;
	IplImage *ImOpFlowY;
	IplImage *ImagenA;
	IplImage *ImagenB;


	tlcde* Identities = NULL;

void Tracking( STFrame* frameDataIn, tlcde** framesBuff ){

	struct timeval ti,tif; // iniciamos la estructura
	float tiempoParcial;
	tlcde* framesBuf;

	static int workPos = -1; // punto de trabajo en el buffer
	gettimeofday(&tif, NULL);

	printf("\n2)Tracking:\n");

	// Inicializar:
	framesBuf = *framesBuff;
	// Imagenes
	AllocateTrackImages( frameDataIn->FG );
	// cola Lifo de identidades
	if(!Identities) {
		Identities = ( tlcde * )malloc( sizeof(tlcde ));
		iniciarLcde( Identities );
		CrearIdentidades(Identities);
		//mostrarIds( Identities );
	}
	//buffer de Imagenes y datos.
	if(!*framesBuff){
		framesBuf = ( tlcde * )malloc( sizeof(tlcde));
		if( !framesBuf ) {error(4);return;}
		iniciarLcde( framesBuf );
		*framesBuff = framesBuf;
	}

	////////// AÑADIR Frame de entrada AL BUFFER /////
	anyadirAlFinal( frameDataIn, framesBuf);
	if( framesBuf->numeroDeElementos < 1) return;

	irAlFinal( framesBuf );
	frameDataIn = ( STFrame* )obtenerActual( framesBuf );

	hungarian_t prob;

	// Asignar identidades y orientación. Establecer estado (fg o oldFG)

	gettimeofday(&ti, NULL);
	printf("\t1)Motion Template\n");
	cvZero(frameDataIn->ImMotion);

	MotionTemplate( framesBuf,Identities );

	tiempoParcial= obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo total: %5.4g ms\n", tiempoParcial);

	// el rastreo no se inicia hasta que el buffer tenga almenos 25 elementos
	if( framesBuf->numeroDeElementos < 25) return;
	// Cuando el buffer esté por la mitad comienza el rastreo en el primer frame
	// La posición de trabajo se irá incremetando a la par que el tamaño del buffer
	// asta quedar finalmente situada en el centro.
	if ( framesBuf->numeroDeElementos < IMAGE_BUFFER_LENGTH) workPos += 1;

	////// FILTRO DE KALMAN //////////////
	gettimeofday(&ti, NULL);
	printf("\t2)Filtro de Kalman\n");

	Kalman(framesBuf,workPos );

	tiempoParcial= obtenerTiempo( ti, 0);
	printf("\t\t- Filtrado correcto.Tiempo total: %5.4g ms\n", tiempoParcial);
	///////   FASE DE CORRECCIÓN  /////////
	// esta etapa se encarga de hacer la asignación de identidad definitiva
	// recibe información temporal de los últimos 48 frames ( Longitud_buffer
	// - frame_Inicio - frame_final - (frame_final-1) que son aprox 2 seg )
	//

	//Reasignamos idendidades.
	// enlazamos objetos etiquetados como nuevos con los que estaban parados

	if (workPos == PRIMERO) return ;

//	framesBuf = matchingIdentity( framesBuf , 0 );

	if (SHOW_OPTICAL_FLOW == 1){
		irAlFinal( framesBuf);
		frameDataIn = ( STFrame* )obtenerActual( framesBuf );
		cvCopy(frameDataIn->FG,ImagenB);
		//cvCvtColor( frameDataIn->Frame, ImagenB, CV_BGR2GRAY);
		irAlAnterior( framesBuf);
		frameDataIn = ( STFrame* )obtenerActual( framesBuf );
		cvCopy(frameDataIn->FG,ImagenA);
	//	cvCvtColor( frameDataIn->Frame, ImagenA, CV_BGR2GRAY);
	//	LKOptFlow( frameDataIn->FG, ImOpFlowX, ImOpFlowY );
		PLKOptFlow( ImagenA, ImagenB, ImOpFlowX );
	cvShowImage( "Flujo Optico X", ImOpFlowX );
	cvShowImage( "Flujo Optico Y", ImOpFlowY);
	}

	tiempoParcial = obtenerTiempo( tif , NULL);
	printf("Tracking correcto.Tiempo total %5.4g ms\n", tiempoParcial);

	irAlFinal( framesBuf );

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
        		ImOpFlowY = cvCreateImage(sz,IPL_DEPTH_32F,1 );

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
void ReleaseDataTrack(){
		if(Identities) liberarIdentidades( Identities );
		if (Identities) free( Identities) ;
		cvReleaseImage( &ImOpFlowX);
    		cvReleaseImage( &ImOpFlowY);
    		cvReleaseImage( &ImagenA);
    		cvReleaseImage( &ImagenB);
    		releaseMotionTemplate();
    		DeallocateKalman(  );

}
