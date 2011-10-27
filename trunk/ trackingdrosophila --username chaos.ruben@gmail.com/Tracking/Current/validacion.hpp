/*
 * validacion.hpp
 *
 *  Created on: 22/09/2011
 *      Author: chao
 */

#ifndef VALIDACION_HPP_
#define VALIDACION_HPP_

#include "VideoTracker.hpp"
#include "segmentacion.hpp"
#include "BGModel.h"

/// Parametros validación
typedef struct {
	int UmbralProb; /// establece cuantas desviaciones tipicas se puede alejar el area del area media sin que se considere inválido el blob
	int UmbralCirc; /// Máxima circularidad a partir de la cual un blob se considerará no válido
	int MaxIncLTHIters; /// Número de iteraciones en las que se incrementará el umbral bajo para dividir una elipse
	int MaxLowTH; /// Máximo valor que alcanzará el umbral mínimo para dividir elipse
	float PxiMin; /// Probabilidad mínima admisible a partir de la cual se deja de aumentar MaxLowTH
	int MaxDecLTHIters; //// Número de iteraciones en las que se incrementará el umbral bajo para aumentar P(xi)
	int MinLowTH; /// Mínimo valor que alcanzará el umbral bajo para aumentar P(xi)
	float UmbralDes;

}ValParams;

tlcde* Validacion(IplImage *Imagen, STFrame* FrameData, SHModel* SH,CvRect Segroi,BGModelParams* BGParams, ValParams* VParams,IplImage* Mask);
void setValParams( ValParams* Params);
void setBGModParams( BGModelParams* Params);
double CalcProbTotal(tlcde* Lista,SHModel* SH,ValParams* VParams,STFly* FlyData);
double CalcProbMosca( SHModel* SH , STFly* Flie );
double CalcCircul( STFly* Flie);
int CalcProbUmbral( SHModel* SH ,ValParams* VParams,STFly* Flie);
#endif /* VALIDACION_HPP_ */
