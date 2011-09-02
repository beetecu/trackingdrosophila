/*
 * AsignarIdentidades.hpp
 *
 *  Created on: 02/09/2011
 *      Author: chao
 */

#ifndef ASIGNARIDENTIDADES_HPP_
#define ASIGNARIDENTIDADES_HPP_

#include "VideoTracker.hpp"
#include "opencv2/video/tracking.hpp"
#include "Libreria.h"



// various tracking parameters (in seconds)
const double MHI_DURATION = 2;
const double MAX_TIME_DELTA = 0.5;
const double MIN_TIME_DELTA = 0.05;
// number of cyclic frame buffer used for motion detection
// (should, probably, depend on FPS)
const int N = 4;

// ring image buffer
IplImage **buf = 0;
int last = 0;

// temporary images
IplImage* silh; // Imagen con las siluetas
IplImage *mhi = 0; // MHI
IplImage *orient = 0; // orientation
IplImage *mask = 0; // valid orientation mask
IplImage *segmask = 0; // motion segmentation map
CvMemStorage* storage = 0; // temporary storage

void MotionTemplate( IplImage* img, IplImage* dst);

#endif /* ASIGNARIDENTIDADES_HPP_ */
