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

	// Parametros de Kalman para la linealidad ( Coordenadas)

	CvKalman* kalman = NULL; // Estructra de kalman para la linealizad
	CvMat* state = NULL;
	CvMat* measurement = NULL;
	CvMat* process_noise = NULL;

	CvPoint coordenadas; // Centro del blob.
	CvPoint coordReal;

	CvRandState rng;

	// Parametros de Kalman para la orientación

	CvKalman* kalman_2=NULL;// Estructura de kalman para la orientacion.
	CvMat* state_2 = NULL;
	CvMat* measurement_2 = NULL;
	CvMat* process_noise_2 = NULL;

	CvRandState rng2;

	float direccion; // Dirección del blob
	float orientacion; // Orietación del blob

	CvRect flieRoi;// Roi del blob.

	CvMat* indexMat[NUMBER_OF_MATRIX]; // Matrices para usar el filtro de Kalman




void Tracking( tlcde* framesBuf ){

	struct timeval ti; // iniciamos la estructura
	float tiempoParcial;
	STFrame* frameData = NULL;
	STFly* flyData = NULL;
	tlcde* flies;

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

	// acceder al punto de trabajo
	irAl( workPos , framesBuf);
	// cargar datos del frame
	frameData = ( STFrame* )obtenerActual( framesBuf );
	gettimeofday(&ti, NULL);


	flies=frameData->Flies;

	for(int flypos=0;flypos < flies->numeroDeElementos;flypos++){

	flyData=(STFly*)obtener(flypos,flies);

	coordenadas=flyData->posicion;
	direccion=flyData->direccion;
	orientacion=flyData->orientacion;


	coordenadas.x=flyData->posicion.x;
	coordenadas.y=flyData->posicion.y;

	flieRoi=flyData->Roi;


	////// INICIALIZAR FILTROS DE KALMAN//////

	kalman = initKalman(indexMat,coordenadas,orientacion,direccion); // FK Lineal para las coodenadas

	kalman_2 = initKalman2(direccion,orientacion);// FK para la orientación

	state=cvCreateMat(4,1,CV_32FC1);
	measurement = cvCreateMat( 2, 1, CV_32FC1 );
	process_noise = cvCreateMat(4, 1, CV_32FC1);

	state_2=cvCreateMat(2,1,CV_32FC1);
	measurement_2 = cvCreateMat( 1, 1, CV_32FC1 );
	process_noise_2 = cvCreateMat(2, 1, CV_32FC1);


	cvZero(state);
	cvZero(state_2);
	cvZero(measurement);
	cvZero(measurement_2);

	state->data.fl[0]=coordenadas.x;
	state->data.fl[1]=coordenadas.y;


	state_2->data.fl[0]=orientacion;


	/////////////////// PREDICCION //////////////////////

//	predict = updateKalmanPredict(kalman);

	const CvMat* yk = cvKalmanPredict( kalman, 0 ); // Predicción coordenadas

	const CvMat* yk_2 = cvKalmanPredict( kalman_2, 0 ); // Predicción orientacion


	cvRandSetRange(&rng,0,sqrt(kalman->measurement_noise_cov->data.fl[0]),0);
	cvRand( &rng, measurement );

	cvRandSetRange(&rng2,0,sqrt(kalman_2->measurement_noise_cov->data.fl[0]),0);
	cvRand( &rng2, measurement_2 );

	cvMatMulAdd(kalman->measurement_matrix,state,measurement,measurement);

	cvMatMulAdd(kalman_2->measurement_matrix,state_2,measurement_2,measurement_2);


	if(workPos > 1){

	////////////////////// CORRECCION ////////////////////////

//	correct = updateKalmanCorrect(kalman ,measurement);

	cvKalmanCorrect( kalman,measurement);

	cvKalmanCorrect( kalman_2,measurement_2);

	cvRandSetRange(&rng,0,sqrt(kalman->process_noise_cov->data.fl[0]),0);
	cvRand( &rng,process_noise );

	cvRandSetRange(&rng2,0,sqrt(kalman_2->process_noise_cov->data.fl[0]),0);
	cvRand( &rng2,process_noise_2 );

	cvMatMulAdd(kalman->transition_matrix,state,process_noise,state);

	cvMatMulAdd(kalman_2->transition_matrix,state_2,process_noise_2,state_2);


	////////////////////// VISUALIZAR RESULTADOS KALMAN///////////////////////////////

	if(SHOW_KALMAN_RESULT){

	printf("\n***********************FRAMEBUF %d BLOB NUMERO %d ************************",workPos,flypos);

	printf("\n Dirección : %f ,  Orientación : %f y Coordenadas: ( %d , %d ) ",direccion,orientacion,coordenadas.x,coordenadas.y);

	printf("\n\n Real State Coordenadas: ( %f y %f )",state->data.fl[0],state->data.fl[1]);
	printf("\n Predicted State Coordenadas: (%f y %f )",yk->data.fl[0],yk->data.fl[1]);
	printf("\n Observed State: ( %f y %f )",measurement->data.fl[0],measurement->data.fl[1]);


	printf("\n\n Real State Orientación : %f",state_2->data.fl[0]);
	printf("\n Predicted State Orientation: %f",yk_2->data.fl[0]);
	printf("\n Observer State:  %f", measurement_2->data.fl[0]);

	printf("\n\n Coordenadas corrección ( %f y %f )",kalman->state_post->data.fl[0],kalman->state_post->data.fl[1]);
	printf("\n Orientacion Corrección %f",kalman_2->state_post->data.fl[0]);
	printf("\n");

	///////////////////// DIBUJAR COOREDENADAS DE KALMAN /////////

		cvCircle(IKalman,cvPoint(cvRound(measurement->data.fl[0]),cvRound(measurement->data.fl[1])),4,CVX_GREEN,1,8);
		cvShowImage( "Kalman", IKalman );

		cvCircle(IKalman,cvPoint(cvRound(yk->data.fl[0]),cvRound(yk->data.fl[1])),5,CVX_RED,1,8);
		cvShowImage( "Kalman", IKalman );

		cvCircle(IKalman,cvPoint(cvRound(state->data.fl[0]),cvRound(state->data.fl[1])),3,CVX_WHITE,1,8);
		cvShowImage( "Kalman", IKalman );


	}

	}

	cvReleaseKalman(&kalman);
	cvReleaseKalman(&kalman_2);

	}//FOR

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
