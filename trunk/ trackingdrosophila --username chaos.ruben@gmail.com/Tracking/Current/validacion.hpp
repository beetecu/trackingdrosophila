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

void Validacion(IplImage *Imagen, STCapas* Capa, SHModel SH, CvRect Segroi);

#endif /* VALIDACION_HPP_ */
