/*
 * Kalman.cpp
 *
 *  Created on: 18/11/2011
 *      Author: german
 */

#include "Kalman.hpp"

#define NUMBER_OF_MATRIX 6




// Incializar los parámetro del filtro de Kalman para la posición.

CvKalman* initKalman(CvMat** IndexMat,CvPoint coord,float orientacion,float direccion){

	const float Velocidad = 10.599; // pixeles por frame

	// Crear el flitro de Kalman

	CvKalman* Kalman = cvCreateKalman(4,2,0);

	// Inicializar las matrices parámetros para el flitro de Kalman

	const float A[] = {1,0,1,0, 0,1,0,1, 0,0,1,0, 0,0,0,1};


	memcpy( Kalman->transition_matrix->data.fl, A, sizeof(A));

	cvSetIdentity( Kalman->measurement_matrix,cvRealScalar(1) );
	cvSetIdentity( Kalman->process_noise_cov,cvRealScalar(1e-3) );
	cvSetIdentity( Kalman->measurement_noise_cov,cvRealScalar(1e-1) );
	cvSetIdentity( Kalman->error_cov_post,cvRealScalar(1000));

	    // Establecer el estado incial

		float Vx=0;// componente X de la velocidad.
		float Vy=0;// componente Y de la velociad.
		float direccionR; // orientacion en radianes.

		// Calcular las componentes de la velocidad

//		direccionR = (direccion*CV_PI)/180;
		direccionR = (orientacion*CV_PI)/180;

		Vx=Velocidad*cos(direccionR);
		Vy=Velocidad*sin(direccionR);

		// Inicializar el vector de estado

		float initialState[] = {coord.x , coord.y, Vx, Vy};

		CvMat Ma=cvMat(1, 4, CV_32FC1, initialState);
	    copyMat(&Ma, Kalman->state_post);

	    return Kalman;
	}

// Incializar los parámetro del filtro de Kalman para la orientación.

CvKalman* initKalman2(float direccion, float orientacion){

	// Crear el flitro de Kalman

	CvKalman* Kalman = cvCreateKalman(2,1,0);

	// Inicializar las matrices parámetros para el flitro de Kalman

	const float A[] = { 1, 1, 0, 1 };
	const float angulo = 5.2;

	memcpy( Kalman->transition_matrix->data.fl, A, sizeof(A));

	cvSetIdentity( Kalman->measurement_matrix,cvRealScalar(1) );
	cvSetIdentity( Kalman->process_noise_cov,cvRealScalar(1e-3) );
	cvSetIdentity( Kalman->measurement_noise_cov,cvRealScalar(1e-2) );
	cvSetIdentity( Kalman->error_cov_post,cvRealScalar(1000));


	// Establecer el estado incial, calculando el sentido de la velocidad angular.

	 float initialState[] = {orientacion,CalcDirection(direccion,orientacion,angulo)};

     CvMat Ma=cvMat(2, 1, CV_32FC1, initialState);
     copyMat(&Ma, Kalman->state_post);

     return Kalman;
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


//
//float* updateKalmanCorrect(CvKalman* kalman ,CvPoint coordenadas ){
//
//	int Meanx, Meany;
//	CvMat* measurement = cvCreateMat(2,1, CV_32FC1 );
//	Meanx = (int)coordenadas.x;
//	Meany = (int)coordenadas.y;
//	cvmSet(measurement,0,0,Meanx);
//	cvmSet(measurement,1,0,Meany);
//
//	const CvMat* correct= cvKalmanCorrect(kalman, measurement);
//
//	return correct->data.fl;
//}
//
//float* updateKalmanPredict(CvKalman* kalman){
//
//
//	 CvMat* u = cvCreateMat(1,1, CV_32FC1 );
//	 u->data.fl[0]=1;
//
//	 const CvMat* predict = cvKalmanPredict(kalman,0);
//
//	 return predict->data.fl;
//
//
//}




