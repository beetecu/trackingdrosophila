/*
 * AsignarIdentidades.cpp
 *
 *  Created on: 02/09/2011
 *      Authors: Germán Macía Vázquez
 *       		 Chao Chao Rubén
 */

#include "AsignarIdentidades.hpp"

using namespace cv;

tlcde* sleep_list = NULL;


int asignarIdentidades(tlcde* lsTraks, tlcde *Flies) {

	CvMat* CoorReal = cvCreateMat(1, 2, CV_32FC1);
	CvMat* Matrix_Asignation = NULL;

	STTrack* Track = NULL;
	STTrack* Track_list = NULL;
	STTrack* TrackActual = NULL;

	STFly* FlyNext = NULL;
	STFly* FlySiguiente = NULL;

	if (!sleep_list) {
		sleep_list = (tlcde *) malloc(sizeof(tlcde));
		if (!sleep_list) {
			error(4);
			exit(1);
		}
	}
	iniciarLcde(sleep_list);

	for (int list = 0; list < lsTraks->numeroDeElementos; list++) {
		Track_list = (STTrack*) obtener(list, lsTraks);
		if (Track_list->Stats->Estado == 0) {
			anyadirAlFinal(Track_list, sleep_list);
			borrarEl(list, lsTraks);
		}
	}

	double
			Hungarian_Matrix[lsTraks->numeroDeElementos][Flies->numeroDeElementos];
	int v = 0;
	int g = 0;
	int ger = 0;
	int indCandidato, indFlie; // posicion de la Fly con mayor porbabilidad.


	if (lsTraks->numeroDeElementos == 0)
		return 0;

	else if (Flies->numeroDeElementos > 0) {

		CvMat* Matrix_Hungarian = cvCreateMat(lsTraks->numeroDeElementos,
				Flies->numeroDeElementos, CV_32FC1); // Matriz de Pesos
		cvZero(Matrix_Hungarian);

		int p = 0;

		for (int d = 0; d < lsTraks->numeroDeElementos; d++) {

			int id = 0;

			Track = (STTrack*) obtener(d, lsTraks);

			for (int f = 0; f < Flies->numeroDeElementos; f++) {

				FlyNext = (STFly*) obtener(f, Flies);

				CoorReal->data.fl[0] = FlyNext->posicion.x;
				CoorReal->data.fl[1] = FlyNext->posicion.y;

				double Peso = PesosKalman(Track->Measurement_noise_cov,
						Track->x_k_Pre, CoorReal);
				Matrix_Hungarian->data.fl[p] = Peso;
				Hungarian_Matrix[d][f] = Peso;
				p++;

			}

		}

		//	Matrix_Asignation=Hungaro(Matrix_Hungarian);

		//	if(Matrix_Hungarian->cols < Matrix_Asignation->cols){
		//
		//	Matrix_Hungarian->rows=Matrix_Asignation->rows;
		//	Matrix_Hungarian->cols=Matrix_Asignation->cols;
		//	int dif= Matrix_Asignation->cols-Matrix_Hungarian->cols;
		//
		//	printf("\n");
		//	for(int l=0;l<Matrix_Hungarian->rows;l++){
		//		printf("\n");
		//		for(int m=0;m<Matrix_Hungarian->cols;m++){
		//			if(m >=Matrix_Hungarian->cols-dif) Matrix_Hungarian->data.fl[g]=0;
		//			printf("\t %f",Matrix_Hungarian->data.fl[g]);
		//			g++;
		//		}
		//	}
		//
		//	}

		if (SHOW_AI_DATA)
			printf("/n********* PESOS*********************************");
		for (int r = 0; r < Matrix_Hungarian->rows; r++) {
			if (SHOW_AI_DATA)
				printf("\n");
			for (int b = 0; b < Matrix_Hungarian->cols; b++) {
				if (SHOW_AI_DATA)
					printf("\t %f", Matrix_Hungarian->data.fl[ger]);
				ger++;
			}
		}

		Matrix_Asignation = Hungaro(Matrix_Hungarian);

		double
				Asignation_Matrix[Matrix_Asignation->rows][Matrix_Asignation->cols];

		if (SHOW_AI_DATA)
			printf("\n********************* ASIGNACION********************");
		for (int l = 0; l < Matrix_Asignation->rows; l++) {
			if (SHOW_AI_DATA)
				printf("\n");
			for (int m = 0; m < Matrix_Asignation->cols; m++) {
				Asignation_Matrix[l][m] = Matrix_Asignation->data.fl[g];
				if (SHOW_AI_DATA)
					printf("\t %f", Matrix_Asignation->data.fl[g]);
				g++;
			}
		}

		// Si la matriz de pesos es cuadrada, asignacion normal 1 es a 1

		if (Matrix_Hungarian->rows == Matrix_Hungarian->cols
				|| Matrix_Hungarian->rows < Matrix_Hungarian->cols) {

			for (int i = 0; i < Matrix_Asignation->rows; i++) {

				for (int j = 0; j < Matrix_Asignation->cols; j++) {

					if (Matrix_Asignation->data.fl[v] == 1) {
						indCandidato = j;

						TrackActual = (STTrack*) obtener(i, lsTraks);
						FlySiguiente = (STFly*) obtener(indCandidato, Flies);

						if (FlySiguiente && TrackActual) {

							anyadirAlFinal(TrackActual, FlySiguiente->Tracks);
							TrackActual->Flysig = FlySiguiente;
							//						anyadirAlFinal(TrackActual,TrackActual->Flysig->Tracks);
						}

					}

					v++;
				}

			}
		}

		if (Matrix_Hungarian->rows > Matrix_Hungarian->cols) {

			int flag;
			int Dif_cols = Matrix_Asignation->cols - Matrix_Hungarian->cols;
			int Add_cols = Matrix_Hungarian->cols + Dif_cols;
			float valor_max;

			for (int n_col = Matrix_Asignation->cols - Dif_cols; n_col
					< Add_cols; n_col++) {
				for (int n_row = 0; n_row < Matrix_Asignation->rows; n_row++) {
					if (Asignation_Matrix[n_row][n_col] == 1) {

						valor_max = 0;
						flag = 0;

						for (int y = 0; y < Matrix_Hungarian->cols; y++) {

							if (Hungarian_Matrix[n_row][y] > 20
									&& Hungarian_Matrix[n_row][y] > valor_max) {

								flag = 1;
								valor_max = Hungarian_Matrix[n_row][y];
								indCandidato = n_row;
								indFlie = y;

							}// if

						}// for

						if (flag == 1) {
							Asignation_Matrix[indCandidato][indFlie] = 1;
							Asignation_Matrix[n_row][n_col] = 0;
							flag = 0;
						} else
							Asignation_Matrix[n_row][n_col] = 0;

					}// if

				}// for
			}// for

			// Asignacion Normal

			g = 0;
			indCandidato = 0;

			if (SHOW_AI_DATA)
				printf("\n");
			for (int l = 0; l < Matrix_Asignation->rows; l++) {
				if (SHOW_AI_DATA)
					printf("\n");
				for (int m = 0; m < Matrix_Asignation->cols; m++) {
					Matrix_Asignation->data.fl[g] = Asignation_Matrix[l][m];
					if (SHOW_AI_DATA)
						printf("\t %f", Matrix_Asignation->data.fl[g]);
					g++;
				}
			}

			for (int i = 0; i < Matrix_Asignation->rows; i++) {

				for (int j = 0; j < Matrix_Asignation->cols; j++) {

					if (Matrix_Asignation->data.fl[v] == 1) {
						indCandidato = j;

						TrackActual = (STTrack*) obtener(i, lsTraks);
						FlySiguiente = (STFly*) obtener(indCandidato, Flies);

						if (FlySiguiente && TrackActual) {

							anyadirAlFinal(TrackActual, FlySiguiente->Tracks);
							TrackActual->Flysig = FlySiguiente;
							//								anyadirAlFinal(TrackActual,TrackActual->Flysig->Tracks);

						}

					}

					v++;
				}

			}

		} // else if

		if (sleep_list->numeroDeElementos > 0) {
			for (int z = 0; z < sleep_list->numeroDeElementos; z++) {
				Track_list = (STTrack*) obtener(z, sleep_list);
				anyadirAlFinal(Track_list, lsTraks);
			}
		}
		return 1;
	}

	return 0;

}

double PesosKalman(const CvMat* Matrix, const CvMat* Predict, CvMat* CordReal) {

	float Matrix_error_cov[] = { Matrix->data.fl[0], Matrix->data.fl[6] };

	float X = CordReal->data.fl[0];
	float Y = CordReal->data.fl[1];
	float EX = Predict->data.fl[0];
	float EY = Predict->data.fl[1];
	//	float VarX = sqrt(Matrix_error_cov[0]);
	//	float VarY = sqrt(Matrix_error_cov[1]);

	float VarX = 10;
	float VarY = 10;

	double ValorX, ValorY;
	double DIVX, DIVY;
	double ProbKalman;

	ValorX = X - EX;
	ValorY = Y - EY;
	DIVX = -((ValorX / VarX) * (ValorX / VarX)) / 2;
	DIVY = -((ValorY / VarY) * (ValorY / VarY)) / 2;

	ProbKalman = exp(-abs(DIVX + DIVY));

	ProbKalman = 100 * ProbKalman;


	if (ProbKalman < 1 || ProbKalman < 0)
		ProbKalman = 0;

	return ProbKalman;

}

void releaseAI() {
	free(sleep_list);
}

// err( Xpred, Xobs ) = ( xpred - xobs )² + ypred - yobs)² + w(phipred - phiobs)²
float funcionError( CvPoint posXk, CvPoint posZk, float phiXk, float phiZk  ){

	static float error;
	static int MaxJump;
	static float phi ;
	static float distancia;
	static float Ax;
	static float Ay;


	MaxJump = obtenerFilterParam( MAX_JUMP );
	Ax = posXk.x - posZk.x ;
	Ay = posXk.y - posZk.y ;
	EUDistance( Ax, Ay, NULL, &distancia );
	if ( distancia > MaxJump ) error = 100000;
	else{
		phi =  phiZk;
		corregirDir( phiXk, &phi );
		error = pow( Ax , 2) + pow( Ay , 2) + pow( phiXk - phi,2 );
	}
	return error;

}

