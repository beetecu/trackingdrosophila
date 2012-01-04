/*
 * Kalman.cpp
 *
 *  Created on: 18/11/2011
 *      Author: german
 */

#include"Tracking.hpp"
#include "Kalman.hpp"

// Parametros de Kalman para la linealidad ( Coordenadas)
CvKalman* kalman = NULL; // Estructra de kalman para la linealizad
CvMat* state = NULL;
CvMat* measurement = NULL;
CvMat* process_noise = NULL;

CvRandState rng;

// Parametros de Kalman para la orientación

CvKalman* kalman_2=NULL;// Estructura de kalman para la orientacion.
CvMat* state_2 = NULL;
CvMat* measurement_2 = NULL;
CvMat* process_noise_2 = NULL;

CvRandState rng2;

CvMat* indexMat[NUMBER_OF_MATRIX]; // Matrices para usar el filtro de Kalman

void Kalman(tlcde* framesBuf,int  workPos ){

	STFrame* frameData = NULL;
	STFly* flyData = NULL;
	tlcde* flies;

	// acceder al punto de trabajo

	irAl( workPos , framesBuf);

	// cargar datos del frame

	frameData = ( STFrame* )obtenerActual( framesBuf );

	flies=frameData->Flies;



	////// INICIALIZAR FILTROS DE KALMAN//////
	if(!kalman){
		state=cvCreateMat(4,1,CV_32FC1);
		measurement = cvCreateMat( 2, 1, CV_32FC1 );
		process_noise = cvCreateMat(4, 1, CV_32FC1);

	if(!kalman_2){
		state_2=cvCreateMat(2,1,CV_32FC1);
		measurement_2 = cvCreateMat( 1, 1, CV_32FC1 );
		process_noise_2 = cvCreateMat(2, 1, CV_32FC1);
	}

	cvZero(frameData->ImKalman);

	//Predicción y correción de cada blob
	for(int flypos=0;flypos < flies->numeroDeElementos;flypos++){

		kalman = initKalman( ); // FK Lineal para las coodenadas
		kalman_2 = initKalman2( );// FK para la orientación

		// obtener blob
		flyData=(STFly*)obtener(flypos,flies);

		// iniciar para predecir la posición del blob
		initKpos( kalman,flyData->posicion,flyData->orientacion);
		state->data.fl[0]=flyData->posicion.x;
		state->data.fl[1]=flyData->posicion.y;
		// iniciar para predecir la orientación del blob
		initkdir( kalman_2,flyData->direccion,flyData->orientacion);
		state_2->data.fl[0]=flyData->orientacion;



		/////////////////// PREDICCION //////////////////////

		const CvMat* yk = cvKalmanPredict( kalman, 0 ); // Predicción coordenadas

		const CvMat* yk_2 = cvKalmanPredict( kalman_2, 0 ); // Predicción orientacion

		printf("\n");

		cvRandSetRange(&rng,0,sqrt(kalman->measurement_noise_cov->data.fl[0]),0);
		cvRand( &rng, measurement );

		cvRandSetRange(&rng2,0,sqrt(kalman_2->measurement_noise_cov->data.fl[0]),0);
		cvRand( &rng2, measurement_2 );

		cvMatMulAdd(kalman->measurement_matrix,state,measurement,measurement);

		cvMatMulAdd(kalman_2->measurement_matrix,state_2,measurement_2,measurement_2);

		if(workPos > 1){

			////////////////////// CORRECCION ////////////////////////

			cvKalmanCorrect( kalman,measurement);

			cvKalmanCorrect( kalman_2,measurement_2);

			cvRandSetRange(&rng,0,sqrt(kalman->process_noise_cov->data.fl[0]),0);
			cvRand( &rng,process_noise );

			cvRandSetRange(&rng2,0,sqrt(kalman_2->process_noise_cov->data.fl[0]),0);
			cvRand( &rng2,process_noise_2 );

			cvMatMulAdd(kalman->transition_matrix,state,process_noise,state);

			cvMatMulAdd(kalman_2->transition_matrix,state_2,process_noise_2,state_2);


			// Diseñar la ROI en función de la estimación y la covarianza del error proporcionadas por el Filtro
			// de Kalman.

			CvRect Kalman_ROI = ROIKalman(kalman->error_cov_pre,kalman->state_pre);


		////////////////////// VISUALIZAR RESULTADOS KALMAN///////////////////////////////

		if(SHOW_KALMAN_DATA){

			printf("\n***********************FRAMEBUF %d BLOB NUMERO %d ************************",workPos,flypos);

			printf("\n Dirección : %f ,  Orientación : %f y Coordenadas: ( %d , %d ) ",flyData->direccion,flyData->orientacion,flyData->posicion.x,flyData->posicion.y);

			printf("\n\n Real State Coordenadas: ( %f y %f )",state->data.fl[0],state->data.fl[1]);
			printf("\n Predicted State Coordenadas: (%f y %f )",yk->data.fl[0],yk->data.fl[1]);
			printf("\n Observed State: ( %f y %f )",measurement->data.fl[0],measurement->data.fl[1]);


			printf("\n\n Real State Orientación : %f",state_2->data.fl[0]);
			printf("\n Predicted State Orientation: %f",yk_2->data.fl[0]);
			printf("\n Observer State:  %f", measurement_2->data.fl[0]);

			printf("\n\n Coordenadas corrección ( %f y %f )",kalman->state_post->data.fl[0],kalman->state_post->data.fl[1]);
			printf("\n Orientacion Corrección %f",kalman_2->state_post->data.fl[0]);
			printf("\n\n");


		}
		if(SHOW_KALMAN){
			///////////////////// DIBUJAR COOREDENADAS DE KALMAN /////////

			cvCircle(frameData->ImKalman,cvPoint(cvRound(measurement->data.fl[0]),cvRound(measurement->data.fl[1])),4,CVX_GREEN,1,8);

			cvCircle(frameData->ImKalman,cvPoint(cvRound(yk->data.fl[0]),cvRound(yk->data.fl[1])),5,CVX_RED,1,8);

			cvCircle(frameData->ImKalman,cvPoint(cvRound(state->data.fl[0]),cvRound(state->data.fl[1])),3,CVX_WHITE,1,8);

			// Dibujar la ROI
//
//			CvPoint pt1;
//			CvPoint pt2;
//
//			pt1.x = Kalman_ROI.x;
//			pt1.y = Kalman_ROI.y;
//			pt2.x = pt1.x + Kalman_ROI.width;
//			pt2.y = pt1.y + Kalman_ROI.height;
//
//			cvRectangle(IKalman,pt1,pt2,CVX_BLUE,1);
//			cvShowImage("Kalman",IKalman);
		}

		}//FOR
	}
}
}
// Incializar los parámetro del filtro de Kalman para la posición.

CvKalman* initKalman( ){

	// Crear el flitro de Kalman

	CvKalman* Kalman = cvCreateKalman(4,2,0);

	// Inicializar las matrices parámetros para el flitro de Kalman

	const float A[] = {1,0,1,0, 0,1,0,1, 0,0,1,0, 0,0,0,1};


	memcpy( Kalman->transition_matrix->data.fl, A, sizeof(A));

	cvSetIdentity( Kalman->measurement_matrix,cvRealScalar(1) );
	cvSetIdentity( Kalman->process_noise_cov,cvRealScalar(1e-3) );
	cvSetIdentity( Kalman->measurement_noise_cov,cvRealScalar(1e-1) );
	cvSetIdentity( Kalman->error_cov_post,cvRealScalar(100));
	cvSet( Kalman->error_cov_pre, cvScalar(1) );


	return Kalman;
	}

// Incializar los parámetro del filtro de Kalman para la orientación.

CvKalman* initKalman2( ){

	// Crear el flitro de Kalman

	CvKalman* Kalman = cvCreateKalman(2,1,0);

	// Inicializar las matrices parámetros para el flitro de Kalman

	const float A[] = { 1, 1, 0, 1 };

	memcpy( Kalman->transition_matrix->data.fl, A, sizeof(A));

	cvSetIdentity( Kalman->measurement_matrix,cvRealScalar(1) );
	cvSetIdentity( Kalman->process_noise_cov,cvRealScalar(1e-3) );
	cvSetIdentity( Kalman->measurement_noise_cov,cvRealScalar(1e-2) );
	cvSetIdentity( Kalman->error_cov_post,cvRealScalar(1000));

	return Kalman;
}

void initKpos(CvKalman* kalman, CvPoint coord, float direccion){ // iniciar para predecir la posición del blob flypos

// Establecer el estado incial

	float Vx=0;// componente X de la velocidad.
	float Vy=0;// componente Y de la velociad.
	float direccionR; // orientacion en radianes.

	// Calcular las componentes de la velocidad

//		direccionR = (flyData->direccion*CV_PI)/180;
	if ( direccion >=0 && direccion < 180) direccion = 180 - direccion;
	else direccion = (360-direccion)+180;

	direccionR = (direccion*CV_PI)/180;
	Vx=-VELOCIDAD*cos(direccionR);
	Vy=-VELOCIDAD*sin(direccionR);

	// Inicializar el vector de estado

	float initialState[] = {coord.x , coord.y, Vx, Vy};

	CvMat Ma=cvMat(1, 4, CV_32FC1, initialState);
	copyMat(&Ma, kalman->state_post);
	cvZero(state);
	cvZero(measurement);

}

void initkdir(CvKalman* kalman,float direccion,float orientacion){ // iniciar para predecir la direccion del blob

	// Establecer el estado incial, calculando el sentido de la velocidad angular.

	float initialState[] = {orientacion,CalcDirection(direccion,orientacion,V_ANGULAR )};

    CvMat Ma=cvMat(2, 1, CV_32FC1, initialState);
    copyMat(&Ma, kalman->state_post);
    cvZero(state_2);
    cvZero(measurement_2);

}


void copyMat (CvMat* source, CvMat* dest){

	int i,j;

	for (i=0; i<source->rows; i++)
		for (j=0; j<source->cols;j++)
			dest->data.fl[i*source->cols+j]=source->data.fl[i*source->cols+j];

}

// Funcion para calcular el sentido de la velocidad angular y por tanto de la orientación

float CalcDirection(float direction,float orientation,float angulo){

	float angle = 0;// angulo con su sentido que devuelve la función.
	float alpha,beta;
	bool signo=false;// sentido de la dirección angular, false = resta angulo, true = suma angulo.

	// Si ORIENTACION pertenece al 1º Cuadrante

	if(0 < orientation && orientation < 90){

		// Si direccion pretenece al Cuadrante 1

		if(0 <= direction && direction <= 90){

			if( orientation > direction) signo = false;

			else signo = true;
		}

		// Si dirección pertene al Cuadrante 2

		if(90 <= direction && direction <= 180) signo = true;

		// Si dirección pertenece al Cuadrante 3

		if(180 < direction && direction < 270){

			alpha = direction - orientation;
			beta = (360 - alpha);

			if(alpha > beta) signo = false;
			else signo = true;
		}

		// Si dirección pertenece al Cuadrante 4

		if(270 <= direction && direction < 360) signo = false; // Direccion C 4

		}

	//Si ORIENTACION pertenece al 2º Cuadrante

	if(90 < orientation && orientation < 180){

		// Si dirección pertenece al Cuadrante 1

		if(0 <= direction && direction <= 90 ) signo = false;

		// Si dirección pertenece al Cuadrante 2

		if(90 < direction && direction < 180){

			if( orientation > direction) signo = true;
			else signo = false;
		}

		// Si dirección pertenece al Cuadrante 3

		if(180 <= direction && direction <= 270) signo = true;

		// Si dirección pertenece al Cuadrante 4

		if(270 < direction && direction < 360){

			beta = (360 - direction) + orientation;
			alpha = 360 - beta;
			if(alpha > beta) signo = false;
			else signo = true;
		}


	}

	// Si ORIENTACION pertenece al 3º Cuadrante

	if(180 < orientation && orientation < 270){

		// Si dirección pertenece al Cuadrante 1

		if(0 <= direction && direction < 90){


			alpha = orientation - direction;
			beta= 360 - alpha;

			if(alpha > beta) signo = true;
			else signo = false;
		}

		// Si dirección pertenece al Cuadrante 2

		if(90 <= direction && direction  <= 180) signo = false;

		// Si dirección pertenece al Cuadrante 3

		if(180 < direction && direction < 270){

			if(orientation > direction) signo = false;
			else signo = true;
		}

		// Si dirección pertenece al Cuadrante 4

		if(270 <= direction && direction < 360 ) signo = true;

	}


	// Si ORIENTACION pertenece al 4º Cuadrante

	if(270 < orientation && orientation < 360){

		// Si dirección pertenece al Cuadrante 1

		if(0 <= direction && direction <= 90) signo = true;

		// Si dirección pertenece al Cuadrante 2

		if(90 < direction && direction < 180){

			beta = (360 - orientation) + direction;
			alpha = 360 - beta;

			if (alpha > beta) signo = true;
			else signo = false;
		}

		// Si dirección pertenece al Cuadrante 3

		if(180 <= direction && direction <= 270) signo = false;

		// Si dirección pertenece al Cuadrante 4

		if(270 < direction && direction < 360){

			if(orientation > direction) signo = false;
			else signo = true;
		}

	}

	// Si signo = false se produce la resta de ángulo, si signo = true la suma de ángulo.

	if( !signo) angle = -angulo;
	else angle = angulo;

	return angle;

} // Fin de la Funcion


CvRect ROIKalman(CvMat* Matrix,CvMat* predict){

	float Matrix_Kalman_Cov[] = {Matrix->data.fl[0], Matrix->data.fl[1], Matrix->data.fl[4], Matrix->data.fl[5]};

	float Valor_X,Valor_Y;

//	CvMat* MC=cvCreateMat(2,2,CV_32FC1);
//	CvMat* Diag=cvCreateMat(2,2,CV_32FC1);
//	CvMat* R=cvCreateMat(2,2,CV_32FC1);
//	CvMat* Rt=cvCreateMat(2,2,CV_32FC1);
//
//	MC->data.fl[0]=Matrix->data.fl[0];
//	MC->data.fl[1]=Matrix->data.fl[1];
//	MC->data.fl[2]=Matrix->data.fl[4];
//	MC->data.fl[3]=Matrix->data.fl[5];
//
//	//	cvEigenVV(MC,R,Rt,2);// Hallar los EigenVectores
//	cvSVD(MC,Diag,R,Rt,0); // Hallar los EigenValores, MATRIX_C=R*Diagonal*RT
//
//	printf("\n");
//	for(int i=0;i < R->cols*R->rows;i++){
//		printf("\n %f",R->data.fl[i]);
//	}

	CvRect ROI;

	Valor_X = sqrt(Matrix_Kalman_Cov[0]);
	Valor_Y = sqrt(Matrix_Kalman_Cov[3]);

	ROI.x=(predict->data.fl[0]-Valor_X);
	ROI.y=(predict->data.fl[1]-Valor_Y);
	ROI.width=2*Valor_X;
	ROI.height=2*Valor_Y;

	return ROI;

}

/// Limpia de la memoria las imagenes usadas durante la ejecución
void DeallocateKalman(  ){

	if(kalman){
		cvReleaseKalman(&kalman);
		cvReleaseMat(&state);
		cvReleaseMat( &measurement );
		cvReleaseMat(&process_noise);
	}
	if(kalman_2)	{
		cvReleaseKalman(&kalman_2);
		cvReleaseMat(&state_2);
		cvReleaseMat(&measurement_2 );
		cvReleaseMat(&process_noise_2);
	}
}



