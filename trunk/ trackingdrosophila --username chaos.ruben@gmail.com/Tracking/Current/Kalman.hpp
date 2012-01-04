/*
 * Kalman.hpp
 *
 *  Created on: 18/11/2011
 *      Author: german
 */

#ifndef KALMAN_HPP_
#define KALMAN_HPP_

#include "VideoTracker.hpp"

#define VELOCIDAD  5.3//10.599 // velocidad del blob en pixeles por frame
#define V_ANGULAR 2.6//5.2 // velocidad angular del blob en pixeles por frame
#define NUMBER_OF_MATRIX 6

void Kalman(tlcde* framesBuf,int  workPos );

//!\brief Inicializa los parametros del filtro de Kalman para las coordenadas.
/*!
 * \param IndexMat Matrices que forman parte del filtro de Kalman.
 * \param coord Coordenadas de la posicion del blob.
 * \param orientacion ángulo que corresponde con la orientación del blob.
 *
 * \return La estructura del filtro de Kalman, inicializada con sus correspondientes valores.
 *
 * \note La función tambien establece los valores iniciales para los paramétros del vector de estado,
 * \note en este caso coordenadas X e Y de la posición y componentes Vx y Vy de la velocidad (cte).
 */

CvKalman* initKalman ( );

//!\brief Inicializa los parametros del filtro de Kalman para la orientación - direccion.
/*!
 * \param angulo Dirección del blob.
 * \param orientacion Orientación del blob.
 *
 * \return La estructura del filtro de Kalman, inicializada con sus correspondientes valores.
 *
 * \note La función tambien establece los valores iniciales para los paramétros del vector de estado,
 * \note en este caso la posición angular y la velocidad angular(cte).
 */

CvKalman* initKalman2 ( );

void initKpos(CvKalman* kalman, CvPoint coord,float direccion);

void initkdir(CvKalman* kalman,float direccion,float orientacion);

//!\brief Copia una matriz fuente e otra matriz destino.
/*!
 * \param source Matriz fuente.
 * \param dest Matriz destino.
 */
void copyMat (CvMat* source , CvMat* dest );

//!\brief Calcula el sentido dela velocidad angular, que sirve como parámetro de inicialización
//!para el vector measurement del filtro de Kalman aplicado a la orientación.
/*!
 * \param Direction dirección del blob.
 * \param Orientation orientacion del blob.
 * \param Angulo valor de la velocidad angular, es constante.
 *
 * \return El sentido de la velocidad angular del blob.
 *
 * \note Para determinar el sentido de la velocidad angular se utilizan métodos trigonométricos.
 */

float CalcDirection(float direction,float orientation,float angulo);

//!\brief Diseñar la ROI en función de la estimación y la covarianza del error proporcionadas por el Filtro
//! de Kalman.
/*!
 * \param Matrix Matriz de covarianza del error.
 * \param Predict Matriz que contiene la estimación ( coordenadas).
 *
 * \return La matriz de busqueda.
 */

CvRect ROIKalman(CvMat* Matrix,CvMat* Predict);

//float* updateKalmanCorrect(CvKalman* kalman,CvPoint coordenadas );
//
//float* updateKalmanPredict(CvKalman* kalman);

void DeallocateKalman(  );

#endif /* KALMAN_HPP_ */
