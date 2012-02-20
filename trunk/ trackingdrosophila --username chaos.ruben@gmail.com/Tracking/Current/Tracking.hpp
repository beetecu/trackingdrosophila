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
#include "hungarian.hpp"
#include <opencv2/video/background_segm.hpp>
#include "AsignarIdentidades.hpp"
#include "FlujoOptico.hpp"


void Tracking( STFrame* frameDataIn, tlcde** framesBuff );

void AllocateTrackImages( IplImage *I );

void ReleaseDataTrack();

void crearMascara( IplImage* Frame, IplImage* FG,IplImage* mascara);

#endif /* TRACKING_HPP_ */
