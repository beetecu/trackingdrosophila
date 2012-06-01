/*
 * AsignarIdentidades.hpp
 *
 *  Created on: 02/09/2011
 *      Author: chao
 */

#ifndef ASIGNARIDENTIDADES_HPP_
#define ASIGNARIDENTIDADES_HPP_

#include "VideoTracker.hpp"
#include "Tracking.hpp"
#include "Kalman.hpp"
#include "Libreria.h"
#include "opencv2/video/tracking.hpp"


int asignarIdentidades( tlcde* lsTraks , tlcde *Flies);

double PesosKalman(const CvMat* Matrix,const CvMat* Predict,CvMat* CordReal);

float funcionError( CvPoint posXk, CvPoint posZk, float phiXk, float phiZk  );

void releaseAI();

#endif /* ASIGNARIDENTIDADES_HPP_ */
