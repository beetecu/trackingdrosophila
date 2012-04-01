/*
 * Kalman.hpp
 *
 *  Created on: 18/11/2011
 *      Author: german
 */

#ifndef KALMAN_HPP_
#define KALMAN_HPP_

#include "VideoTracker.hpp"
#include "Tracking.hpp"

#define VELOCIDAD  5.3//10.599 // velocidad del blob en pixeles por frame
#define V_ANGULAR 2.6//5.2 // velocidad angular del blob en pixeles por frame
#define NUMBER_OF_MATRIX 6

typedef struct{
	int id;
	CvKalman* kalman ; // Estructura de kalman para la linealizada

    CvMat* x_k_Pre_; // Predicción de k hecha en k-1
    CvMat* P_k_Pre_; // incertidumbre en la predicción en k-1

    const CvMat* x_k_Pre; // Predicción de k+1 hecha en k
    const CvMat* P_k_Pre; // incertidumbre en la predicción

	const CvMat* x_k_Pos; // Posición tras corrección
	const CvMat* P_k_Pos; // Incertidumbre en la posición tras corrección

	CvMat* Measurement_noise; // V->N(0,R) : Incertidumbre en la medida. Lo suponemos de media 0.
	CvMat* Measurement_noise_cov; // R
	CvMat* Medida;	// Valor real ( medido )
	CvMat* z_k; // Zk = H Medida + V. Valor observado

	tlcde* listFlyNext;
	STFly* Flysig; // Fly asignada en t+1. Obtenemos de aqui la nueva medida
	STFly* FlyActual;

}STTrack;

CvMat* Kalman(STFrame* frameData,STFrame* frameData_sig,tlcde* lsIds,tlcde* lsTracks);

void Kalman2(STFrame* frameData,tlcde* lsIds,tlcde* lsTracks);

void initNewsTracks( STFrame* frameData, tlcde* lsTracks );

STTrack* initTrack( STFly* Fly ,tlcde* ids, float fps );

CvKalman* initKalman( STFly* fly, float dt );

int deadTrack( tlcde* Tracks, int id );

void generarZ_k( STTrack* Track);

void generarMedida(  STTrack* Track, STFly* Fly );

void generarMedida2(  STTrack* Track, STFly* Fly  );

int generarPhiZk( float phiXk , float Vx, float Vy , float* phiZk, float* R_phiZk);

void kalmanControl( STTrack* Track );

void visualizarKalman( STFrame* frameData, tlcde* lsTracks);

void showKalmanData( STTrack *Track);

void DeallocateKalman( tlcde* lista );

void liberarTracks( tlcde* lista);

void copyMat (CvMat* source, CvMat* dest);

void Kalman2(STFrame* frameData,STFrame* frameData_sig, tlcde* lsIds,tlcde* lsTracks);

#endif /* KALMAN_HPP_ */
