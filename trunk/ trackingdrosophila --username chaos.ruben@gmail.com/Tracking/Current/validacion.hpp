/*
 * validacion.hpp
 *
 *  Created on: 22/09/2011
 *      Author: chao
 */

#ifndef VALIDACION_HPP_
#define VALIDACION_HPP_

#include "VideoTracker.hpp"
#include "segmentacion.h"
#include "BGModel.h"

/// Parametros validación
typedef struct {
	float UmbralProb; /// establece cuantas desviaciones tipicas se puede alejar el area del area media sin que se considere inválido el blob
	float UmbralCirc; /// Máxima circularidad a partir de la cual un blob se considerará no válido
	int MaxIncLTHIters; /// Número de iteraciones en las que se incrementará el umbral bajo para dividir una elipse
	int MaxLowTH; /// Máximo valor que alcanzará el umbral mínimo para dividir elipse
	float PxiMin; /// Probabilidad mínima admisible a partir de la cual se deja de aumentar MaxLowTH
	int MaxDecLTHIters; //// Número de iteraciones en las que se incrementará el umbral bajo para aumentar P(xi)
	int MinLowTH; /// Mínimo valor que alcanzará el umbral bajo para aumentar P(xi)

}ValParams;

void Validacion(IplImage *Imagen, STCapas* Capa, SHModel* SH, STFlies* Flie, CvRect Segroi,BGModelParams* BGParams, ValParams* VParams);
void setValParams( ValParams* Params);
void setBGModParams( BGModelParams* Params);
double CalcProbMosca( SHModel* SH , STFlies* Flie );
double CalcCircul( STFlies* Flie);
int CalcProbUmbral( SHModel* SH ,ValParams* VParams);
#endif /* VALIDACION_HPP_ */
