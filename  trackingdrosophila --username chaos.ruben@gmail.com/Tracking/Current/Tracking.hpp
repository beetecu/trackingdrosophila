/*
 * Tracking.hpp
 *
 *  Created on: 20/10/2011
 *      Author: chao
 */

#ifndef TRACKING_HPP_
#define TRACKING_HPP_

#include "VideoTracker.hpp"
#include "Libreria.h"
#include <opencv2/video/background_segm.hpp>

void Tracking( tlcde* frammeBuf );

void MotionTemplate( IplImage* img, IplImage* dst);

void OpticalFlowLK( IplImage* ImLast, IplImage* velX, IplImage* velY);


#endif /* TRACKING_HPP_ */
