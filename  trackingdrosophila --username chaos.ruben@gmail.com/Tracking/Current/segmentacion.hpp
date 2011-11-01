/*
 * segmentacion.hpp
 *
 *  Created on: 29/08/2011
 *      Author: german
 */

#ifndef SEGMENTACION_H_
#define SEGMENTACION_H_

#include "VideoTracker.hpp"
#include "Libreria.h"

tlcde* segmentacion( IplImage *Brillo, STFrame* FrameData ,CvRect Roi, IplImage* Mask );

void CreateDataSegm( IplImage* Brillo );

void ReleaseDataSegm( );

void establecerROIS( STFrame* FrameData, IplImage* Brillo,CvRect Roi );

void resetearROIS( STFrame* FrameData, IplImage* Brillo );

#endif /* SEGMENTACION_H_ */
