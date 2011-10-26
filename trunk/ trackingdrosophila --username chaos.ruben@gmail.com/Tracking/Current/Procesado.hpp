/*
 * Procesado.hpp
 *
 *  Created on: 20/10/2011
 *      Author: chao
 */

#ifndef PROCESADO_HPP_
#define PROCESADO_HPP_

#include "VideoTracker.hpp"
#include "Libreria.h"
#include "BGModel.h"
#include "segmentacion.hpp"


void Procesado( IplImage* frame,tlcde* framesBuf, StaticBGModel* BGModel,STFlat* Flat, SHModel* Shape);

void Procesado2( IplImage* frame,tlcde* framesBuf, StaticBGModel* BGModel,STFlat* Flat,SHModel* Shape );

void InitNewFrameData(IplImage* I, STFrame *FrameData );

void putBGModelParams( BGModelParams* Params);

#endif /* PROCESADO_HPP_ */
