/*
 * ShapeModel.hpp
 *
 *  Created on: 19/09/2011
 *      Author: chao
 */

#ifndef SHAPEMODEL_HPP_
#define SHAPEMODEL_HPP_

#include "VideoTracker.hpp"
#include "Libreria.h"
#include <BlobResult.h>
#include <Blob.h>

#define SM_FRAMES_TRAINING 50

void ShapeModel( CvCapture* g_capture, int* FlyAreaMed, int* FlyAreaDes,IplImage* ImMask, CvRect ROI );

#endif /* SHAPEMODEL_HPP_ */
